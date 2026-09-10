#include "llviewerprecompiledheaders.h"
#include "fsdaeexporter.h"

#include "llvolume.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llvovolume.h"
#include "llviewerobject.h"
#include "llviewerjointattachment.h"
#include "llviewercontrol.h"
#include "llfilepicker.h"
#include "lldirpicker.h"
#include "llagent.h"
#include "llmeshrepository.h"
#include "llnotificationsutil.h"
#include "llprocess.h"
#include "llfile.h"
#include "lltimer.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"
#include "lltexturecache.h"
#include "llappviewer.h"
#include "llcallbacklist.h"
#include "llimage.h"
#include "llimagej2c.h"
#include "llimagepng.h"
#include "fscommon.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <map>
#include <set>
#include <algorithm>

//=============================================================================
// Public API
//=============================================================================

// Collects an object and all of its child prims into nodes.
static void collectObjectTree(LLViewerObject* obj, std::vector<LLViewerObject*>& nodes)
{
    if (!obj) return;
    nodes.push_back(obj);
    const LLViewerObject::const_child_list_t& children = obj->getChildren();
    for (LLViewerObject::const_child_list_t::const_iterator it = children.begin(); it != children.end(); ++it)
        collectObjectTree(it->get(), nodes);
}

//=============================================================================
// Async texture saving
//=============================================================================

class FSTextureExportJob;

// Reads the stored texture bytes of one texture from the local texture cache.
// The reader leaves the responder image format open so the cache creates the
// correct decoder (J2C on server-downloaded textures at full resolution).
class FSTextureCacheReadResponder : public LLTextureCache::ReadResponder
{
    LOG_CLASS(FSTextureCacheReadResponder);
public:
    FSTextureCacheReadResponder(const LLUUID& id, const std::string& out_path, FSTextureExportJob* job)
        : mId(id), mOutPath(out_path), mJob(job)
    {
    }

protected:
    void completed(bool success) override;

private:
    LLUUID mId;
    std::string mOutPath;
    FSTextureExportJob* mJob;
};

// Shared job (one per viewer session): textures are requested as meshes are
// exported. For each texture we wait until the viewer has fully fetched it
// (confirmed via the viewer-texture loaded callback), then read the stored
// bytes from the local texture cache and write a PNG. If the cache read or
// the J2C decode fails (e.g. texture only ever downloaded at a coarse
// discard), we fall back to the decoded raw image the viewer is holding.
// The job self-deletes once every requested texture has either been saved or
// failed, so a summary alert always appears.
class FSTextureExportJob
{
public:
    // Returns the decoded raw image the viewer was holding, if any. Not
    // consumed: the raw is shared by every output path that references the same
    // texture UUID (same texture, several Material_N.png copies), and is
    // released (refcounted) once the last of those paths has finished.
    LLImageRaw* getFallbackRaw(const LLUUID& id)
    {
        std::map<LLUUID, LLPointer<LLImageRaw>>::iterator it = mFallbackRaw.find(id);
        if (it == mFallbackRaw.end())
            return NULL;
        return it->second.get();
    }

    void releaseFallbackRaw(const LLUUID& id)
    {
        std::map<LLUUID, S32>::iterator ref = mFallbackRefs.find(id);
        if (ref == mFallbackRefs.end())
            return;
        if (--ref->second > 0)
            return;
        std::map<LLUUID, LLPointer<LLImageRaw>>::iterator it = mFallbackRaw.find(id);
        if (it != mFallbackRaw.end())
            mFallbackRaw.erase(it);
        mFallbackRefs.erase(ref);
    }
    void addRequest(const LLUUID& id, const std::string& out_path)
    {
        // Key every request by its output path, NOT by texture UUID: the same
        // texture is often shared between linked items (e.g. sleeve texture
        // used by two attachment parts), and each of those parts needs its own
        // copy of the PNG next to its own DAE.
        if (id.isNull() || mPending.count(out_path) || mDone.count(out_path))
            return;

        mPending[out_path] = id;
        mPendingByUuid[id].push_back(out_path);
        ++mRemaining;
        ensureWatchdog();
        LL_INFOS("DAEExport") << "Queued texture " << id.asString() << " -> " << out_path << LL_ENDL;

        LLViewerFetchedTexture* imagep = LLViewerTextureManager::getFetchedTexture(id);
        if (!imagep)
            return;

        // Wait until the texture is fully loaded before touching the cache.
        imagep->setBoostLevel(LLViewerTexture::BOOST_MAX_LEVEL);
        imagep->forceToSaveRawImage(0);
        imagep->setLoadedCallback(FSTextureExportJob::onTextureLoaded,
                                  0, true, false, this, &mCallbackTextureList);
    }

    void start()
    {
        // Requests are issued eagerly in addRequest(), so per-mesh exports can
        // contribute textures at any time. If nothing was queued, close out
        // immediately (running any pending .blend conversions) so a previous
        // job is never kept alive by an empty batch.
        if (mRemaining <= 0 && !mFinished)
            finish();
        else
            ensureWatchdog();
    }

    // Registers a DAE whose .blend conversion should run only after every
    // texture it references has been written. Blender reads the PNGs by name;
    // importing while they are still being fetched would open the DAE with
    // missing textures (race condition). The conversion runs in finish().
    void addBlendAfterTextures(const std::string& dae_path)
    {
        mPendingBlendDae.push_back(dae_path);
    }

    // Watchdog: textures that never fetch (e.g. an unrendered avatar on the
    // other side of the region) would otherwise leave the job hanging forever,
    // so after a grace period they are salvaged from cache, then force-failed.
    // The popup always appears and the retained job is freed once the last
    // outstanding cache read completes.
    void ensureWatchdog()
    {
        if (mWatchdogStarted)
            return;
        mWatchdogStarted = true;
        mWatchdogTimer.setTimerExpirySec(SOFT_SALVAGE_SEC);
        gIdleCallbacks.addFunction(&FSTextureExportJob::watchdog, this);
    }

    static void watchdog(void* userdata)
    {
        FSTextureExportJob* job = (FSTextureExportJob*)userdata;
        if (!job)
            return;
        if (job->mFinished)
        {
            gIdleCallbacks.deleteFunction(&FSTextureExportJob::watchdog, job);
            return;
        }
        if (job->mWatchdogTimer.hasExpired())
        {
            job->onWatchdogExpire();
            if (job->mRemaining <= 0)
                job->finish();
        }
    }

    void onWatchdogExpire()
    {
        if (!mDidSoftSalvage)
        {
            // First expiry: try a direct cache read for every texture whose
            // fetch callback never arrived; better to save from disk than fail.
            mDidSoftSalvage = true;
            mWatchdogTimer.setTimerExpirySec(SALVAGE_GRACE_SEC);
            std::vector<LLUUID> ids;
            for (auto const& kv : mPending)
                ids.push_back(kv.second);
            for (const LLUUID& id : ids)
                fetchTextureFromCache(id, true, NULL);
            return;
        }

        // Grace period over: anything still pending never made it anywhere.
        // Fail it out so the job always terminates with a result popup.
        std::vector<LLUUID> ids;
        for (auto const& kv : mPending)
            ids.push_back(kv.second);
        for (const LLUUID& id : ids)
        {
            std::vector<std::string> paths;
            std::map<LLUUID, std::vector<std::string>>::iterator it = mPendingByUuid.find(id);
            if (it != mPendingByUuid.end())
                paths = it->second;
            for (const std::string& out_path : paths)
            {
                std::map<std::string, LLUUID>::iterator pit = mPending.find(out_path);
                if (pit == mPending.end())
                    continue;
                mPending.erase(pit);
                mDone.insert(out_path);
                ++mFailed;
                LL_WARNS("DAEExport") << "Texture export timed out, skipped: " << out_path
                                      << " (" << id.asString() << ")" << LL_ENDL;
                --mRemaining;
            }
            mPendingByUuid.erase(id);
        }
    }

    void onTextureDone(bool success, LLImageFormatted* image, const std::string& out_path, LLImageRaw* fallback_raw)
    {
        std::string saved = "no";
        if (success && image && image->getDataSize() > 0 && image->getData())
        {
            LLPointer<LLImageRaw> raw = new LLImageRaw;
            image->updateData();
            if (image->decode(raw, 1e6F) && raw->getData() && raw->getWidth() > 0 && raw->getHeight() > 0)
            {
                if (writeRawAsPng(raw, out_path))
                    saved = "cache";
            }
        }
        if (saved == "no" && fallback_raw && fallback_raw->getData() && fallback_raw->getWidth() > 0)
        {
            if (writeRawAsPng(fallback_raw, out_path))
                saved = "raw";
        }

        if (saved != "no")
        {
            ++mSaved;
            LL_INFOS("DAEExport") << "Texture saved (source=" << saved << "): " << out_path << LL_ENDL;
        }
        else
        {
            ++mFailed;
            LL_WARNS("DAEExport") << "Texture export failed: " << out_path
                                  << " cache_ok=" << success
                                  << " bytes=" << (image ? image->getDataSize() : 0)
                                  << " raw_fallback=" << (fallback_raw ? (S32)fallback_raw->getWidth() : 0) << LL_ENDL;
        }
        oneDone();
    }

private:
    static bool writeRawAsPng(LLImageRaw* raw, const std::string& out_path)
    {
        if (!raw || !raw->getData() || raw->getWidth() <= 0 || raw->getHeight() <= 0)
            return false;
        LLPointer<LLImagePNG> png = new LLImagePNG;
        if (!png->encode(raw, 1e6F))
            return false;
        return png->save(out_path);
    }

    static void onTextureLoaded(bool success, LLViewerFetchedTexture* src_vi, LLImageRaw* src, LLImageRaw*,
                                S32, bool final, void* userdata)
    {
        if (!final)
            return;
        FSTextureExportJob* job = (FSTextureExportJob*)userdata;
        if (job)
            job->fetchTextureFromCache(src_vi->getID(), success, src);
    }

    void fetchTextureFromCache(const LLUUID& id, bool loaded, LLImageRaw* fallback_raw)
    {
        std::vector<std::string> paths;
        std::map<LLUUID, std::vector<std::string>>::iterator by_uuid = mPendingByUuid.find(id);
        if (by_uuid == mPendingByUuid.end())
            return; // never queued, or already handled
        paths = by_uuid->second;
        mPendingByUuid.erase(by_uuid);

        if (!loaded)
        {
            for (const std::string& out_path : paths)
            {
                std::map<std::string, LLUUID>::iterator pit = mPending.find(out_path);
                if (pit == mPending.end())
                    continue;
                mPending.erase(pit);
                mDone.insert(out_path);
                ++mFailed;
                LL_WARNS("DAEExport") << "Texture never fetched, skipped: " << out_path
                                      << " (" << id.asString() << ")" << LL_ENDL;
                --mRemaining;
            }
            if (mRemaining <= 0)
                finish();
            return;
        }

        // Shared raw fallback: the same decoded image backs every output path
        // for this UUID. Keep one copy per UUID and refcount it per path so the
        // first path to finish can never steal the fallback from the others.
        // (Watchdog salvage passes NULL here; it only has the cache to rely on.)
        if (fallback_raw)
        {
            std::map<LLUUID, S32>::iterator ref = mFallbackRefs.find(id);
            mFallbackRefs[id] = (ref != mFallbackRefs.end() ? ref->second : 0)
                                + (S32)paths.size();
            mFallbackRaw[id] = fallback_raw;
        }
        for (const std::string& out_path : paths)
        {
            std::map<std::string, LLUUID>::iterator pit = mPending.find(out_path);
            if (pit == mPending.end())
                continue;
            mPending.erase(pit);
            mDone.insert(out_path);

            // Ask for the whole stored entry. readFromCache caps the read at
            // the requested size (llmin(max_datasize, stored)), and
            // calcDataSizeJ2C underestimates real files, which returned
            // truncated code streams that openjpeg could not decode. A large
            // cap always reads the full stored J2C so the PNG decode succeeds;
            // a coarse-only cache entry still decodes (at a lower quality)
            // instead of failing.
            FSTextureCacheReadResponder* responder = new FSTextureCacheReadResponder(id, out_path, this);
            LLAppViewer::getTextureCache()->readFromCache(id, 0, 64 * 1024 * 1024, responder);
        }
    }

    void oneDone()
    {
        --mRemaining;
        if (mRemaining <= 0)
        {
            if (mFinished)
                tryDeleteSelf();
            else
                finish();
        }
    }

    void finish()
    {
        if (mFinished)
            return;
        mFinished = true;
        gIdleCallbacks.deleteFunction(&FSTextureExportJob::watchdog, this);

        // First convert any DAE files waiting on textures, so the written
        // .blend files already contain fully loaded textures.
        for (const std::string& dae : mPendingBlendDae)
            FSDAEExporter::exportBlendFile(dae);

        LLSD args;
        args["MESSAGE"] = llformat("Textures exported: %d saved, %d failed",
                                   mSaved, mFailed);
        LLNotificationsUtil::add("GenericAlert", args);
        if (sRetainedJob == this)
            sRetainedJob = NULL;
        tryDeleteSelf();
    }

    void tryDeleteSelf()
    {
        // Defer deletion while responders are still in flight; when the last
        // one completes oneDone() will land here again.
        if (mFinished && mRemaining <= 0)
            delete this;
    }

    std::map<std::string, LLUUID> mPending;
    std::map<LLUUID, std::vector<std::string>> mPendingByUuid;
    std::map<LLUUID, LLPointer<LLImageRaw>> mFallbackRaw;
    std::map<LLUUID, S32> mFallbackRefs;
    std::vector<std::string> mPendingBlendDae;
    std::set<std::string> mDone;
    LLLoadedCallbackEntry::source_callback_list_t mCallbackTextureList;
    bool mFinished = false;
    bool mWatchdogStarted = false;
    bool mDidSoftSalvage = false;
    LLTimer mWatchdogTimer;
    // Wait long enough for BOOST_MAX_LEVEL textures on a normal grid, then a
    // short extra grace for the salvage cache reads to land.
    static constexpr F32 SOFT_SALVAGE_SEC = 45.f;
    static constexpr F32 SALVAGE_GRACE_SEC = 20.f;
    S32 mRemaining = 0;
    S32 mSaved = 0;
    S32 mFailed = 0;

public:
    static FSTextureExportJob* sRetainedJob;
};

FSTextureExportJob* FSTextureExportJob::sRetainedJob = NULL;

void FSTextureCacheReadResponder::completed(bool success)
{
    if (mJob)
    {
        mJob->onTextureDone(success, mFormattedImage, mOutPath, mJob->getFallbackRaw(mId));
        mJob->releaseFallbackRaw(mId);
    }
}

static FSTextureExportJob& textureExportJob()
{
    if (!FSTextureExportJob::sRetainedJob)
        FSTextureExportJob::sRetainedJob = new FSTextureExportJob;
    return *FSTextureExportJob::sRetainedJob;
}

bool FSDAEExporter::exportRiggedMesh(const std::string& filebase, LLViewerObject* object, bool export_blend)
{
    if (!object) return false;

    LLVOVolume* volume_obj = dynamic_cast<LLVOVolume*>(object);
    if (!volume_obj) return false;

    const LLMeshSkinInfo* skin = volume_obj->getSkinInfo();
    LLVolume* volume = object->getVolume();
    if (!volume) return false;

    std::vector<ExportData> faces;
    gatherFaces(volume, skin, volume_obj, faces);
    if (faces.empty()) return false;

    std::string model_name = object->getAttachmentItemName();
    if (model_name.empty())
        model_name = "ExportedMesh";
    model_name = sanitizeId(model_name);

    std::string dae_path = filebase;
    if (dae_path.size() < 4 || dae_path.compare(dae_path.size() - 4, 4, ".dae") != 0)
        dae_path += ".dae";

    std::vector<JointAccumData> skeleton;
    if (skin)
        buildSkeleton(skin, skeleton);

    std::ofstream out(dae_path.c_str(), std::ios::out | std::ios::trunc);
    if (!out.is_open()) return false;

    out << std::fixed << std::setprecision(8);
    writeDAE(out, faces, skin, skeleton, model_name);
    out.close();

    queueTextureSaves(dae_path, faces);

    LL_INFOS("DAEExport") << "Exported rigged mesh '" << model_name << "' to " << dae_path << LL_ENDL;

    if (export_blend)
    {
        // The .blend must be written only after the texture PNGs exist, so the
        // conversion is deferred to the texture job's completion hook. We still
        // write the helper script now (idempotent) so a manual conversion is
        // possible while the textures are still being fetched.
        std::string blend_path = dae_path.substr(0, dae_path.size() - 4) + ".blend";
        writeBlenderScript(dae_path + ".import_to_blend.py", dae_path, blend_path);

        if (findBlenderExecutable().empty())
        {
            LL_INFOS("DAEExport") << "Blender not found, .blend will not be auto-generated. To convert manually run: "
                                  << "blender --background --factory-startup --python \"" << dae_path
                                  << ".import_to_blend.py\" -- \"" << dae_path << "\" \"" << blend_path
                                  << "\"" << LL_ENDL;
        }
        else
        {
            textureExportJob().addBlendAfterTextures(dae_path);
            // Flush: if this mesh queued no textures the job finishes now and
            // instantly converts any pending DAE files.
            textureExportJob().start();
        }
    }
    return true;
}

S32 FSDAEExporter::exportAvatarRiggedMeshes(LLVOAvatar* avatarp, const std::string& directory, bool export_blend)
{
    if (!avatarp)
        return 0;

    S32 exported = 0;

    std::set<LLViewerObject*> exported_volumes;
    std::map<std::string, S32> used_names;

    for (LLVOAvatar::attachment_map_t::const_iterator it = avatarp->mAttachmentPoints.begin();
         it != avatarp->mAttachmentPoints.end(); ++it)
    {
        const LLViewerJointAttachment* attachment = it->second;
        if (!attachment || attachment->getIsHUDAttachment())
            continue;

        for (LLViewerJointAttachment::attachedobjs_vec_t::const_iterator obj_it = attachment->mAttachedObjects.begin();
             obj_it != attachment->mAttachedObjects.end(); ++obj_it)
        {
            // The inventory item name is only visible to us for our own
            // attachments and for the linkset root, so resolve it once here.
            std::string root_item_name;
            if (obj_it->get())
                root_item_name = obj_it->get()->getAttachmentItemName();
            if (root_item_name.empty())
                root_item_name = attachment->getName();
            if (root_item_name.empty())
                root_item_name = "AttachmentMesh";

            // Walk the whole linkset: a linkset may only have a skinned child
            // prim (item is usually the root and the mesh is a child).
            std::vector<LLViewerObject*> nodes;
            collectObjectTree(obj_it->get(), nodes);

            for (size_t n = 0; n < nodes.size(); ++n)
            {
                LLViewerObject* objectp = nodes[n];
                if (!objectp || exported_volumes.count(objectp))
                    continue;

                LLVOVolume* volume_obj = dynamic_cast<LLVOVolume*>(objectp);
                if (!volume_obj)
                {
                    LL_INFOS("DAEExport") << "Skipping non-volume prim '" << root_item_name << "'" << LL_ENDL;
                    continue;
                }
                if (!volume_obj->getSkinInfo())
                {
                    LL_INFOS("DAEExport") << "Skipping unskinned volume '" << root_item_name << "' (no skin block)" << LL_ENDL;
                    continue;
                }

                exported_volumes.insert(objectp);

                std::string base = volume_obj->getAttachmentItemName();
                if (base.empty())
                    base = root_item_name;
                base = sanitizeId(base);
                std::string out_base = base;
                if (used_names[base] > 0)
                    out_base = base + "_" + std::to_string(used_names[base] + 1);
                used_names[base]++;

                std::string folder = directory;
                if (!folder.empty() && folder[folder.size() - 1] != '/' && folder[folder.size() - 1] != '\\')
                    folder += "/";
                folder += out_base;
                if (LLFile::mkdir(folder) != 0)
                    LL_WARNS("DAEExport") << "Could not create export folder " << folder << LL_ENDL;

                std::string filebase = folder;
                if (!filebase.empty() && filebase[filebase.size() - 1] != '/' && filebase[filebase.size() - 1] != '\\')
                    filebase += "/";
                filebase += out_base;

                if (exportRiggedMesh(filebase, objectp, export_blend))
                    ++exported;
            }
        }
    }

    return exported;
}

S32 FSDAEExporter::exportAllAvatarRiggedMeshes(const std::string& directory, bool export_blend)
{
    return exportAvatarRiggedMeshes(gAgentAvatarp, directory, export_blend);
}

LLVOVolume* FSDAEExporter::findFirstSkinnedVolume(LLViewerObject* root)
{
    if (!root)
        return NULL;

    std::vector<LLViewerObject*> nodes;
    collectObjectTree(root, nodes);

    for (size_t n = 0; n < nodes.size(); ++n)
    {
        LLVOVolume* volume_obj = dynamic_cast<LLVOVolume*>(nodes[n]);
        if (volume_obj && volume_obj->getSkinInfo())
            return volume_obj;
    }

    return NULL;
}

//=============================================================================
// Skeleton construction
//=============================================================================

bool FSDAEExporter::buildSkeleton(const LLMeshSkinInfo* skin, std::vector<JointAccumData>& joints_out)
{
    if (!skin)
        return false;

    const S32 n = (S32)skin->mJointNames.size();
    if (n == 0 || (S32)skin->mInvBindMatrix.size() != n)
        return false;

    struct SkinJoint
    {
        std::string name;
        LLMatrix4 world_bind;
    };

    std::vector<SkinJoint> defs;
    defs.reserve(n);

    std::set<std::string> skinned_names;
    std::map<std::string, S32> name_to_idx;

    for (S32 i = 0; i < n; ++i)
    {
        SkinJoint d;
        d.name = skin->mJointNames[i];
        const LLMatrix4a& inv = skin->mInvBindMatrix[i];

        const F32* p = inv.getF32ptr();
        bool all_zero = true;
        for (S32 k = 0; k < 16; ++k)
            if (fabsf(p[k]) > 1e-6f) { all_zero = false; break; }
        if (all_zero)
            return false;   // corrupted skin data

        LLMatrix4 world(inv);
        world.invert();
        d.world_bind = world;

        skinned_names.insert(d.name);
        name_to_idx[d.name] = (S32)defs.size();
        defs.push_back(d);
    }

    // Determine the nearest skinned ancestor for each joint from the avatar skeleton.
    for (S32 i = 0; i < n; ++i)
    {
        JointAccumData acc;
        acc.name = defs[i].name;
        acc.world_bind = defs[i].world_bind;

        acc.parent_name.clear();
        if (gAgentAvatarp)
        {
            LLJoint* joint = gAgentAvatarp->getJoint(acc.name);
            for (LLJoint* parent = joint ? joint->getParent() : NULL; parent; parent = parent->getParent())
            {
                std::map<std::string, S32>::const_iterator found = name_to_idx.find(parent->getName());
                if (found != name_to_idx.end())
                {
                    acc.parent_name = found->first;
                    break;
                }
            }
        }

        if (acc.parent_name.empty())
        {
            acc.local_bind = acc.world_bind;
        }
        else
        {
            S32 parent_idx = name_to_idx[acc.parent_name];
            acc.local_bind = defs[parent_idx].world_bind;
            acc.local_bind.invert();
            acc.local_bind *= acc.world_bind;
        }

        joints_out.push_back(acc);
    }

    return true;
}

//=============================================================================
// Weight extraction from volume face
//=============================================================================

bool FSDAEExporter::extractWeightData(const LLVolumeFace& vf,
    std::vector<std::vector<std::pair<S32, F32>>>& weights)
{
    if (!vf.mWeights || vf.mNumVertices == 0)
        return false;

    weights.resize(vf.mNumVertices);

    for (S32 i = 0; i < vf.mNumVertices; ++i)
    {
        for (S32 j = 0; j < 4; ++j)
        {
            F32 combined = vf.mWeights[i][j];
            if (combined == 0.0f) continue;

            S32 joint_idx = (S32)floorf(combined);
            F32 weight = combined - floorf(combined);
            if (joint_idx >= 0 && weight > 0.001f)
            {
                weights[i].push_back(std::make_pair(joint_idx, weight));
            }
        }
    }

    return true;
}

//=============================================================================
// Face data extraction
//=============================================================================

void FSDAEExporter::queueTextureSaves(const std::string& dae_path, const std::vector<ExportData>& faces)
{
    std::string dir = dae_path;
    size_t slash = dir.find_last_of("/\\");
    if (slash != std::string::npos)
        dir = dir.substr(0, slash);
    else
        dir = ".";

    FSTextureExportJob& job = textureExportJob();
    bool queued = false;

    for (const auto& f : faces)
    {
        if (f.texture_id.isNull() || f.texture_file.empty())
            continue;
        // NOTE: no dedup by texture UUID here. Baked-clothes meshes (flat
        // garment texture shared by several faces, each with its own
        // Scale/Offset transform) legitimately need one saved PNG per face,
        // even though the UUID is the same for all of them. The job itself
        // dedups by output path, so identical paths never double-queue.
        std::string path = dir;
        if (!path.empty() && path[path.size() - 1] != '/' && path[path.size() - 1] != '\\')
            path += "/";
        path += f.texture_file;
        job.addRequest(f.texture_id, path);
        LL_INFOS("DAEExport") << "Queued texture " << f.texture_id << " -> " << path << LL_ENDL;
        queued = true;
    }

    if (queued)
        job.start();
}

void FSDAEExporter::gatherFaces(LLVolume* volume, const LLMeshSkinInfo* skin, LLVOVolume* volume_obj,
                                std::vector<ExportData>& faces)
{
    if (!volume) return;

    // Bind-shape baking: vertex positions are transformed into skeleton/bind
    // space so the mesh overlays the exported armature. The bind shape matrix
    // is then written as identity in the DAE, which preserves the exact SLM
    // skin data on re-import.
    LLMatrix4 normal_mat;   // identity when not skinned
    LLMatrix4a bind_shape;
    bool skinned = (skin != NULL);

    if (skinned)
    {
        bind_shape = skin->mBindShapeMatrix;
        normal_mat = LLMatrix4(skin->mBindShapeMatrix);
        normal_mat.invert();
        normal_mat.transpose();
    }

    S32 num_faces = volume->getNumVolumeFaces();
    for (S32 i = 0; i < num_faces; ++i)
    {
        const LLVolumeFace& vf = volume->getVolumeFace(i);
        if (vf.mNumVertices <= 0 || vf.mNumIndices <= 0) continue;

        ExportData face;
        face.material_name = "Material_" + std::to_string(i);
        face.has_skin = false;
        face.face_color = LLColor4(1.f, 1.f, 1.f, 1.f);

        const LLTextureEntry* face_te = NULL;
        U8 face_texgen = LLTextureEntry::TEX_GEN_DEFAULT;

        if (volume_obj)
        {
            S32 num_tes = volume_obj->getNumTEs();
            if (i < num_tes)
            {
                face_te = volume_obj->getTE(i);
                if (face_te)
                {
                    face.texture_id = face_te->getID();
                    if (!face.texture_id.isNull() && !FSCommon::isDefaultTexture(face.texture_id))
                        face.texture_file = "Material_" + std::to_string(i) + ".png";
                    face.face_color = face_te->getColor();
                    face_texgen = face_te->getTexGen();
                }
            }
        }

        face.positions.resize(vf.mNumVertices);
        face.normals.resize(vf.mNumVertices);
        face.texcoords.resize(vf.mNumVertices);
        face.indices.assign(vf.mIndices, vf.mIndices + vf.mNumIndices);

        for (S32 j = 0; j < vf.mNumVertices; ++j)
        {
            const F32* pos = vf.mPositions[j].getF32ptr();
            LLVector4a v4;
            v4.load3(pos);
            if (skinned)
                bind_shape.affineTransform(v4, v4);
            F32 tmp[4];
            v4.store4a(tmp);
            face.positions[j].set(tmp[0], tmp[1], tmp[2]);

            const F32* nrm = vf.mNormals[j].getF32ptr();
            LLVector3 n(nrm[0], nrm[1], nrm[2]);
            if (skinned)
            {
                F32 nx = n.mV[0] * normal_mat.mMatrix[0][0] + n.mV[1] * normal_mat.mMatrix[1][0] + n.mV[2] * normal_mat.mMatrix[2][0];
                F32 ny = n.mV[0] * normal_mat.mMatrix[0][1] + n.mV[1] * normal_mat.mMatrix[1][1] + n.mV[2] * normal_mat.mMatrix[2][1];
                F32 nz = n.mV[0] * normal_mat.mMatrix[0][2] + n.mV[1] * normal_mat.mMatrix[1][2] + n.mV[2] * normal_mat.mMatrix[2][2];
                n.set(nx, ny, nz);
                n.normVec();
            }
            face.normals[j] = n;
            face.texcoords[j] = vf.mTexCoords[j];

            // Bake the per-face texture transform (Scale/Offset/Rotation from
            // the texture entry) directly into the exported UVs so the DAE
            // reproduces exactly what SL renders on this face. This mirrors
            // LLFace::xform(), which the renderer applies on top of the mesh UVs
            // every frame. Only the default (mesh UV) texgen is baked; planar
            // texgen regenerates coordinates from world space and is left alone.
            if (face_te && face_texgen == LLTextureEntry::TEX_GEN_DEFAULT)
            {
                F32 s = face.texcoords[j].mV[VX];
                F32 t = face.texcoords[j].mV[VY];
                F32 cos_ang = cosf(face_te->getRotation());
                F32 sin_ang = sinf(face_te->getRotation());
                s -= 0.5f;
                t -= 0.5f;
                F32 ns =  s * cos_ang + t * sin_ang;
                F32 nt = -s * sin_ang + t * cos_ang;
                ns *= face_te->getScaleS();
                nt *= face_te->getScaleT();
                ns += face_te->getOffsetS() + 0.5f;
                nt += face_te->getOffsetT() + 0.5f;
                face.texcoords[j].mV[VX] = ns;
                face.texcoords[j].mV[VY] = nt;
            }
        }

        if (extractWeightData(vf, face.skin_weights))
            face.has_skin = true;

        faces.push_back(face);
    }
}

//=============================================================================
// DAE writing
//=============================================================================

void FSDAEExporter::writeDAE(std::ofstream& out,
                              const std::vector<ExportData>& faces,
                              const LLMeshSkinInfo* skin,
                              const std::vector<JointAccumData>& joints,
                              const std::string& geom_id)
{
    std::string skin_id = geom_id + "-skin";

    out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    out << "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n";

    writeAsset(out);
    writeMaterials(out, faces);
    writeGeometry(out, faces, geom_id);

    if (skin && !skin->mJointNames.empty())
    {
        bool has_weights = false;
        for (const auto& f : faces)
            if (f.has_skin) { has_weights = true; break; }

        if (has_weights
            && (S32)skin->mInvBindMatrix.size() == (S32)skin->mJointNames.size()
            && !joints.empty())
        {
            writeSkinning(out, faces, skin, geom_id, skin_id);
        }
    }

    writeScene(out, faces, joints, skin, skin_id, geom_id);
    out << "</COLLADA>\n";
}

void FSDAEExporter::writeAsset(std::ofstream& out)
{
    out << "  <asset>\n";
    out << "    <contributor>\n";
    out << "      <authoring_tool>Blender</authoring_tool>\n";
    out << "    </contributor>\n";
    out << "    <unit name=\"meter\" meter=\"1\"/>\n";
    out << "    <up_axis>Z_UP</up_axis>\n";
    out << "  </asset>\n";
}

void FSDAEExporter::writeMaterials(std::ofstream& out, const std::vector<ExportData>& faces)
{
    if (faces.empty()) return;

    // Images referenced by textured faces
    out << "  <library_images>\n";
    for (S32 i = 0; i < (S32)faces.size(); ++i)
    {
        if (faces[i].texture_file.empty())
            continue;
        std::string name = sanitizeId(faces[i].texture_file);
        std::string::size_type dot = name.find_last_of('.');
        if (dot != std::string::npos)
            name.erase(dot);
        out << "    <image id=\"" << name << "\" name=\"" << name << "\">\n";
        out << "      <init_from>" << faces[i].texture_file << "</init_from>\n";
        out << "    </image>\n";
    }
    out << "  </library_images>\n";

    out << "  <library_materials>\n";
    for (S32 i = 0; i < (S32)faces.size(); ++i)
    {
        std::string name = sanitizeId(faces[i].material_name);
        out << "    <material id=\"" << name << "\" name=\"" << name << "\">\n";
        out << "      <instance_effect url=\"#" << name << "-fx\"/>\n";
        out << "    </material>\n";
    }
    out << "  </library_materials>\n";

    out << "  <library_effects>\n";
    for (S32 i = 0; i < (S32)faces.size(); ++i)
    {
        std::string name = sanitizeId(faces[i].material_name);
        std::string tex_name;
        if (!faces[i].texture_file.empty())
        {
            tex_name = sanitizeId(faces[i].texture_file);
            std::string::size_type dot = tex_name.find_last_of('.');
            if (dot != std::string::npos)
                tex_name.erase(dot);
        }

        out << "    <effect id=\"" << name << "-fx\">\n";
        out << "      <profile_COMMON>\n";
        if (!tex_name.empty())
        {
            out << "        <newparam sid=\"" << tex_name << "_surface\">\n";
            out << "          <surface type=\"2D\">\n";
            out << "            <init_from>" << tex_name << "</init_from>\n";
            out << "          </surface>\n";
            out << "        </newparam>\n";
            out << "        <newparam sid=\"" << tex_name << "_sampler\">\n";
            out << "          <sampler2D>\n";
            out << "            <source>" << tex_name << "_surface</source>\n";
            out << "          </sampler2D>\n";
            out << "        </newparam>\n";
            out << "        <technique sid=\"common\">\n";
            out << "          <lambert>\n";
            out << "            <diffuse><texture texture=\"" << tex_name << "_sampler\" texcoord=\"UVMap\"/></diffuse>\n";
            out << "          </lambert>\n";
            out << "        </technique>\n";
        }
        else
        {
            // No custom texture: keep the actual SL face color/tint the build
            // tools expose, so the export shows the same color the prim does.
            const LLColor4& c = faces[i].face_color;
            out << "        <technique sid=\"common\">\n";
            out << "          <lambert>\n";
            out << "            <diffuse><color>"
                << c.mV[VX] << " " << c.mV[VY] << " " << c.mV[VZ] << " " << c.mV[VW]
                << "</color></diffuse>\n";
            out << "          </lambert>\n";
            out << "        </technique>\n";
        }
        out << "      </profile_COMMON>\n";
        out << "    </effect>\n";
    }
    out << "  </library_effects>\n";
}

void FSDAEExporter::writeGeometry(std::ofstream& out,
                                   const std::vector<ExportData>& faces,
                                   const std::string& geom_id)
{
    U32 total_verts = 0, total_idx = 0;
    for (const auto& f : faces)
    {
        total_verts += (U32)f.positions.size();
        total_idx += (U32)f.indices.size();
    }

    out << "  <library_geometries>\n";
    out << "    <geometry id=\"" << geom_id << "-mesh\" name=\"" << geom_id << "\">\n";
    out << "      <mesh>\n";

    // Positions
    out << "        <source id=\"" << geom_id << "-mesh-positions\">\n";
    out << "          <float_array id=\"" << geom_id << "-mesh-positions-array\" count=\"" << (total_verts * 3) << "\">";
    for (const auto& f : faces)
        for (const auto& v : f.positions)
            out << v.mV[0] << " " << v.mV[1] << " " << v.mV[2] << " ";
    out << "</float_array>\n";
    out << "          <technique_common>\n";
    out << "            <accessor source=\"#" << geom_id << "-mesh-positions-array\" count=\"" << total_verts << "\" stride=\"3\">\n";
    out << "              <param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>\n";
    out << "            </accessor>\n";
    out << "          </technique_common>\n";
    out << "        </source>\n";

    // Normals
    out << "        <source id=\"" << geom_id << "-mesh-normals\">\n";
    out << "          <float_array id=\"" << geom_id << "-mesh-normals-array\" count=\"" << (total_verts * 3) << "\">";
    for (const auto& f : faces)
        for (const auto& v : f.normals)
            out << v.mV[0] << " " << v.mV[1] << " " << v.mV[2] << " ";
    out << "</float_array>\n";
    out << "          <technique_common>\n";
    out << "            <accessor source=\"#" << geom_id << "-mesh-normals-array\" count=\"" << total_verts << "\" stride=\"3\">\n";
    out << "              <param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>\n";
    out << "            </accessor>\n";
    out << "          </technique_common>\n";
    out << "        </source>\n";

    // Texcoords
    out << "        <source id=\"" << geom_id << "-mesh-map-0\">\n";
    out << "          <float_array id=\"" << geom_id << "-mesh-map-0-array\" count=\"" << (total_verts * 2) << "\">";
    for (const auto& f : faces)
        for (const auto& v : f.texcoords)
            out << v.mV[0] << " " << (1.0f - v.mV[1]) << " ";
    out << "</float_array>\n";
    out << "          <technique_common>\n";
    out << "            <accessor source=\"#" << geom_id << "-mesh-map-0-array\" count=\"" << total_verts << "\" stride=\"2\">\n";
    out << "              <param name=\"S\" type=\"float\"/><param name=\"T\" type=\"float\"/>\n";
    out << "            </accessor>\n";
    out << "          </technique_common>\n";
    out << "        </source>\n";

    // Vertices
    out << "        <vertices id=\"" << geom_id << "-mesh-vertices\">\n";
    out << "          <input semantic=\"POSITION\" source=\"#" << geom_id << "-mesh-positions\"/>\n";
    out << "          <input semantic=\"NORMAL\" source=\"#" << geom_id << "-mesh-normals\"/>\n";
    out << "          <input semantic=\"TEXCOORD\" source=\"#" << geom_id << "-mesh-map-0\"/>\n";
    out << "        </vertices>\n";

    // Triangles per face
    U32 base_vert = 0;
    for (const auto& f : faces)
    {
        std::string mat = sanitizeId(f.material_name);
        out << "        <triangles count=\"" << (f.indices.size() / 3) << "\" material=\"" << mat << "\">\n";
        out << "          <input semantic=\"VERTEX\" source=\"#" << geom_id << "-mesh-vertices\" offset=\"0\"/>\n";
        out << "          <p>";
        for (U32 j = 0; j < f.indices.size(); ++j)
            out << (base_vert + f.indices[j]) << " ";
        out << "</p>\n";
        out << "        </triangles>\n";
        base_vert += (U32)f.positions.size();
    }

    out << "      </mesh>\n";
    out << "    </geometry>\n";
    out << "  </library_geometries>\n";
}

void FSDAEExporter::writeMatrix(std::ofstream& out, const LLMatrix4& mat)
{
    for (S32 j = 0; j < 4; ++j)              // columns
    {
        for (S32 i = 0; i < 4; ++i)          // rows
        {
            out << mat.mMatrix[i][j] << " ";
        }
        out << " ";
    }
}

void FSDAEExporter::writeMatrix4a(std::ofstream& out, const LLMatrix4a& mat)
{
    LLMatrix4 m4(mat);
    writeMatrix(out, m4);
}

void FSDAEExporter::writeSkinning(std::ofstream& out,
                                   const std::vector<ExportData>& faces,
                                   const LLMeshSkinInfo* skin,
                                   const std::string& geom_id,
                                   const std::string& skin_id)
{
    U32 num_joints = (U32)skin->mJointNames.size();

    // Collect all weights across all faces
    struct WeightEntry { S32 joint; F32 weight; };
    std::vector<std::vector<WeightEntry>> all_weights;
    U32 total_unique_weights = 0;

    for (const auto& f : faces)
    {
        for (const auto& vw : f.skin_weights)
        {
            std::vector<WeightEntry> entry;
            for (const auto& w : vw)
            {
                if (w.first >= 0 && (U32)w.first < num_joints)
                    entry.push_back({w.first, w.second});
            }
            all_weights.push_back(entry);
            total_unique_weights += (U32)entry.size();
        }
    }

    U32 num_vertices = (U32)all_weights.size();

    out << "  <library_controllers>\n";
    out << "    <controller id=\"" << skin_id << "\">\n";
    out << "      <skin source=\"#" << geom_id << "-mesh\">\n";

    // Bind shape matrix (identity: the bind shape was baked into the vertices)
    out << "        <bind_shape_matrix>";
    LLMatrix4 identity;
    writeMatrix(out, identity);
    out << "</bind_shape_matrix>\n";

    // Joint names (order matches the joint indices used by the vertex weights)
    out << "        <source id=\"" << skin_id << "-joints\">\n";
    out << "          <Name_array id=\"" << skin_id << "-joints-array\" count=\"" << num_joints << "\">";
    for (const auto& name : skin->mJointNames)
        out << sanitizeId(name) << " ";
    out << "</Name_array>\n";
    out << "          <technique_common>\n";
    out << "            <accessor source=\"#" << skin_id << "-joints-array\" count=\"" << num_joints << "\" stride=\"1\">\n";
    out << "              <param name=\"JOINT\" type=\"Name\"/>\n";
    out << "            </accessor>\n";
    out << "          </technique_common>\n";
    out << "        </source>\n";

    // Inv bind matrices (kept verbatim from the SLM skin block)
    out << "        <source id=\"" << skin_id << "-bind-poses\">\n";
    out << "          <float_array id=\"" << skin_id << "-bind-poses-array\" count=\"" << (num_joints * 16) << "\">";
    for (const auto& mat : skin->mInvBindMatrix)
        writeMatrix4a(out, mat);
    out << "</float_array>\n";
    out << "          <technique_common>\n";
    out << "            <accessor source=\"#" << skin_id << "-bind-poses-array\" count=\"" << num_joints << "\" stride=\"16\">\n";
    out << "              <param name=\"TRANSFORM\" type=\"float4x4\"/>\n";
    out << "            </accessor>\n";
    out << "          </technique_common>\n";
    out << "        </source>\n";

    // Weight values
    out << "        <source id=\"" << skin_id << "-weights\">\n";
    out << "          <float_array id=\"" << skin_id << "-weights-array\" count=\"" << total_unique_weights << "\">";
    for (const auto& vw : all_weights)
        for (const auto& w : vw)
            out << w.weight << " ";
    out << "</float_array>\n";
    out << "          <technique_common>\n";
    out << "            <accessor source=\"#" << skin_id << "-weights-array\" count=\"" << total_unique_weights << "\" stride=\"1\">\n";
    out << "              <param name=\"WEIGHT\" type=\"float\"/>\n";
    out << "            </accessor>\n";
    out << "          </technique_common>\n";
    out << "        </source>\n";

    // Joints and weights mapping
    out << "        <joints>\n";
    out << "          <input semantic=\"JOINT\" source=\"#" << skin_id << "-joints\"/>\n";
    out << "          <input semantic=\"INV_BIND_MATRIX\" source=\"#" << skin_id << "-bind-poses\"/>\n";
    out << "        </joints>\n";

    // Vertex weights: vcount + v
    out << "        <vertex_weights count=\"" << num_vertices << "\">\n";
    out << "          <input semantic=\"JOINT\" source=\"#" << skin_id << "-joints\" offset=\"0\"/>\n";
    out << "          <input semantic=\"WEIGHT\" source=\"#" << skin_id << "-weights\" offset=\"1\"/>\n";
    out << "          <vcount>";
    for (const auto& vw : all_weights)
        out << vw.size() << " ";
    out << "</vcount>\n";
    out << "          <v>";
    U32 weight_idx = 0;
    for (const auto& vw : all_weights)
    {
        for (const auto& w : vw)
        {
            out << w.joint << " " << weight_idx << " ";
            weight_idx++;
        }
    }
    out << "</v>\n";
    out << "        </vertex_weights>\n";

    out << "      </skin>\n";
    out << "    </controller>\n";
    out << "  </library_controllers>\n";
}

void FSDAEExporter::writeJointNode(std::ofstream& out,
                                    const std::vector<JointAccumData>& joints,
                                    const std::map<std::string, std::vector<std::string> >& children,
                                    const std::string& name,
                                    S32 depth)
{
    const JointAccumData* current = NULL;
    for (const auto& j : joints)
        if (j.name == name) { current = &j; break; }
    if (!current) return;

    std::string sname = sanitizeId(name);
    std::string indent((size_t)(depth + 1) * 6, ' ');

    out << indent << "<node id=\"" << sname << "\" sid=\"" << sname << "\" name=\"" << sname << "\" type=\"JOINT\">\n";
    out << indent << "  <matrix sid=\"transform\">";
    writeMatrix(out, current->local_bind);
    out << "</matrix>\n";

    std::map<std::string, std::vector<std::string> >::const_iterator it = children.find(name);
    if (it != children.end())
        for (const auto& child : it->second)
            writeJointNode(out, joints, children, child, depth + 1);

    out << indent << "</node>\n";
}

void FSDAEExporter::writeScene(std::ofstream& out,
                                const std::vector<ExportData>& faces,
                                const std::vector<JointAccumData>& joints,
                                const LLMeshSkinInfo* skin,
                                const std::string& skin_id,
                                const std::string& geom_id)
{
    out << "  <library_visual_scenes>\n";
    out << "    <visual_scene id=\"Scene\" name=\"Scene\">\n";

    bool has_skin = (skin && !skin->mJointNames.empty() && !joints.empty());

    // Skeleton joints
    if (has_skin)
    {
        out << "      <node id=\"Armature\" name=\"Armature\" type=\"NODE\">\n";
        out << "        <matrix sid=\"transform\">1 0 0 0  0 1 0 0  0 0 1 0  0 0 0 1</matrix>\n";

        std::map<std::string, std::vector<std::string> > children;
        std::vector<std::string> roots;

        for (const auto& j : joints)
        {
            if (!j.parent_name.empty())
                children[j.parent_name].push_back(j.name);
            else
                roots.push_back(j.name);
        }

        for (const auto& r : roots)
            writeJointNode(out, joints, children, r, 1);

        out << "      </node>\n";
    }

    // Mesh instance
    out << "      <node id=\"" << geom_id << "\" name=\"" << geom_id << "\" type=\"NODE\">\n";
    out << "        <matrix sid=\"transform\">1 0 0 0  0 1 0 0  0 0 1 0  0 0 0 1</matrix>\n";

    if (has_skin)
    {
        // Point the skeleton at a root joint so Blender resolves the armature.
        std::string root_name = joints[0].name;
        for (const auto& j : joints)
            if (j.parent_name.empty()) { root_name = j.name; break; }

        out << "        <instance_controller url=\"#" << skin_id << "\">\n";
        out << "          <skeleton>#" << sanitizeId(root_name) << "</skeleton>\n";
        out << "          <bind_material>\n";
        out << "            <technique_common>\n";
        for (const auto& f : faces)
        {
            std::string mat = sanitizeId(f.material_name);
            out << "              <instance_material symbol=\"" << mat << "\" target=\"#" << mat << "\">\n";
            // The effect's diffuse texture references its texcoord by semantic
            // ("UVMap"); bind it to the geometry's TEXCOORD input so importers
            // (Blender) apply the UV set explicitly.
            out << "                <bind_vertex_input semantic=\"UVMap\" input_semantic=\"TEXCOORD\" input_set=\"0\"/>\n";
            out << "              </instance_material>\n";
        }
        out << "            </technique_common>\n";
        out << "          </bind_material>\n";
        out << "        </instance_controller>\n";
    }
    else
    {
        out << "        <instance_geometry url=\"#" << geom_id << "-mesh\">\n";
        out << "          <bind_material>\n";
        out << "            <technique_common>\n";
        for (const auto& f : faces)
        {
            std::string mat = sanitizeId(f.material_name);
            out << "              <instance_material symbol=\"" << mat << "\" target=\"#" << mat << "\">\n";
            out << "                <bind_vertex_input semantic=\"UVMap\" input_semantic=\"TEXCOORD\" input_set=\"0\"/>\n";
            out << "              </instance_material>\n";
        }
        out << "            </technique_common>\n";
        out << "          </bind_material>\n";
        out << "        </instance_geometry>\n";
    }

    out << "      </node>\n";
    out << "    </visual_scene>\n";
    out << "  </library_visual_scenes>\n";

    out << "  <scene>\n";
    out << "    <instance_visual_scene url=\"#Scene\"/>\n";
    out << "  </scene>\n";
}

std::string FSDAEExporter::sanitizeId(const std::string& name)
{
    std::string result = name;
    for (char& c : result)
    {
        if (!isalnum((unsigned char)c) && c != '_' && c != '-')
            c = '_';
    }
    if (result.empty() || isdigit((unsigned char)result[0]))
        result = "ID_" + result;
    return result;
}

//=============================================================================
// Blender .blend export
//=============================================================================

bool FSDAEExporter::writeBlenderScript(const std::string& script_path,
                                        const std::string& dae_path,
                                        const std::string& blend_path)
{
    std::ofstream out(script_path.c_str(), std::ios::out | std::ios::trunc);
    if (!out.is_open()) return false;

    out << R"PY(
"""
Auto-generated by Tasia Viewer.
Imports an exported rigged COLLADA mesh and saves it as a .blend file so the
wearable can be weight painted and re-exported. Re-importing the re-exported
DAE preserves the exact same rigging.

Usage:
    blender --background --factory-startup --python "<this file>" -- <input.dae> <output.blend>
"""
import bpy
import sys

args = sys.argv[sys.argv.index("--") + 1:]
dae_path = args[0]
blend_path = args[1]

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.wm.collada_import(filepath=dae_path)

# Ensure the file has at least one object so it saves cleanly.
if len(bpy.data.objects) == 0:
    bpy.ops.mesh.primitive_cube_add(size=0.05, location=(0.0, 0.0, 1.0))

bpy.ops.wm.save_as_mainfile(filepath=blend_path)
print("Saved %s" % blend_path)
)PY";

    out.close();
    return true;
}

std::string FSDAEExporter::findBlenderExecutable()
{
    static const std::string setting = "FSBlenderExecutable";
    std::string override_path = gSavedSettings.getString(setting);
    if (!override_path.empty() && LLFile::isfile(override_path))
        return override_path;

    std::vector<std::string> candidates;

#if LL_WINDOWS
    candidates.push_back("C:\\Program Files\\Blender Foundation\\Blender 4.6\\blender.exe");
    candidates.push_back("C:\\Program Files\\Blender Foundation\\Blender 4.5\\blender.exe");
    candidates.push_back("C:\\Program Files\\Blender Foundation\\Blender 4.4\\blender.exe");
    candidates.push_back("C:\\Program Files\\Blender Foundation\\Blender 4.3\\blender.exe");
    candidates.push_back("C:\\Program Files\\Blender Foundation\\Blender 4.2\\blender.exe");
    candidates.push_back("C:\\Program Files\\Blender Foundation\\Blender 4.1\\blender.exe");
    candidates.push_back("C:\\Program Files\\Blender Foundation\\Blender 4.0\\blender.exe");
    candidates.push_back("C:\\Program Files\\Blender Foundation\\Blender 3.6\\blender.exe");
    candidates.push_back("C:\\Program Files\\Blender Foundation\\Blender 3.3\\blender.exe");
    candidates.push_back("C:\\Program Files\\Blender Foundation\\Blender\\blender.exe");
#elif LL_DARWIN
    candidates.push_back("/Applications/Blender.app/Contents/MacOS/Blender");
    candidates.push_back("/usr/local/bin/blender");
    candidates.push_back("/opt/homebrew/bin/blender");
    candidates.push_back("/opt/local/bin/blender");
#else
    candidates.push_back("/usr/bin/blender");
    candidates.push_back("/usr/local/bin/blender");
    candidates.push_back("/snap/bin/blender");
    candidates.push_back("/opt/blender/blender");
    candidates.push_back("/opt/blender-4.0/blender");
    candidates.push_back("/opt/blender-3.6/blender");
#endif

    for (const std::string& c : candidates)
    {
        if (LLFile::isfile(c))
            return c;
    }

    // Fall back to a PATH search for "blender".
    const char* path_env = getenv("PATH");
    if (path_env)
    {
        std::string path_list(path_env);
        std::string token;
        size_t start = 0, sep;
        while ((sep = path_list.find_first_of(":;", start)) != std::string::npos)
        {
            token = path_list.substr(start, sep - start);
            if (!token.empty())
            {
                std::string candidate = token;
                if (candidate[candidate.size() - 1] != '/' && candidate[candidate.size() - 1] != '\\')
                    candidate += "/";
#if LL_WINDOWS
                candidate += "blender.exe";
#else
                candidate += "blender";
#endif
                if (LLFile::isfile(candidate))
                    return candidate;
            }
            start = sep + 1;
        }
    }

    return "";
}

bool FSDAEExporter::exportBlendFile(const std::string& dae_path)
{
    std::string blend_path = dae_path;
    if (blend_path.size() >= 4 && blend_path.compare(blend_path.size() - 4, 4, ".dae") == 0)
        blend_path = blend_path.substr(0, blend_path.size() - 4) + ".blend";
    else
        blend_path += ".blend";

    std::string script_path = dae_path + ".import_to_blend.py";
    if (!writeBlenderScript(script_path, dae_path, blend_path))
    {
        LL_WARNS("DAEExport") << "Failed to write Blender helper script " << script_path << LL_ENDL;
        return false;
    }

    std::string blender = findBlenderExecutable();
    if (blender.empty())
    {
        LL_INFOS("DAEExport") << "Blender not found, .blend not generated. To convert manually run: "
                              << "blender --background --factory-startup --python \"" << script_path
                              << "\" -- \"" << dae_path << "\" \"" << blend_path << "\"" << LL_ENDL;
        return false;
    }

    LLProcess::Params params;
    params.executable = blender;
    params.args.add("--background");
    params.args.add("--factory-startup");
    params.args.add("--python");
    params.args.add(script_path);
    params.args.add("--");
    params.args.add(dae_path);
    params.args.add(blend_path);
    params.autokill = true;

    LLProcessPtr process = LLProcess::create(params);
    if (!process)
    {
        LL_WARNS("DAEExport") << "Failed to launch Blender." << LL_ENDL;
        return false;
    }

    LL_INFOS("DAEExport") << "Launched Blender to write " << blend_path << LL_ENDL;

    // Wait for Blender to finish so every export completes before we move on.
    // (The DAE is already written at this point; this only converts it.)
    const U32 kBlenderTimeoutMs = 120000;
    U32 waited = 0;
    while (LLProcess::isRunning(process) && waited < kBlenderTimeoutMs)
    {
        ms_sleep(100);
        waited += 100;
    }

    if (LLProcess::isRunning(process))
    {
        LL_WARNS("DAEExport") << "Blender timed out writing " << blend_path << LL_ENDL;
        return false;
    }

    return true;
}