#ifndef FS_DAE_EXPORTER_H
#define FS_DAE_EXPORTER_H

#include "llmodel.h"
#include "llmatrix4a.h"
#include "m4math.h"
#include "lluuid.h"
#include "v4color.h"
#include <string>
#include <vector>
#include <map>
#include <fstream>

class LLViewerObject;
class LLMeshSkinInfo;
class LLVOVolume;
class LLVOAvatar;

// Rigged-mesh exporter (COLLADA 1.4.1) that preserves skinning so an exported
// wearable can be edited (weight painted) and re-imported with identical rigging.
//
// Round-trip guarantee: on import, the viewer applies
//     world * invBind * bindShape * pos
// so we bake bindShape into the vertex positions and write bind_shape_matrix as
// identity while keeping the original inv-bind matrices. The skeleton joint
// rest transforms are set to the inverse of the inv-bind matrices, which makes
// an unmodified re-export from Blender reproduce the exact same skin data.
class FSDAEExporter
{
public:
    // Exports one rigged mesh object to <filebase>.dae (and, when export_blend
    // is true and Blender is found, also <filebase>.blend). Textures referenced
    // by the mesh faces are written as PNG files next to the DAE asynchronously.
    static bool exportRiggedMesh(const std::string& filebase, LLViewerObject* object, bool export_blend);

    // Exports every skin-wearing mesh worn by the given avatar into directory,
    // each mesh into its own <directory>/<item>/ subfolder.
    // Returns the number of meshes exported.
    static S32 exportAvatarRiggedMeshes(LLVOAvatar* avatarp, const std::string& directory, bool export_blend);

    // Exports every attached, skin-wearing mesh on the agent avatar into
    // directory, each mesh into its own <directory>/<item>/ subfolder.
    // Returns the number of meshes exported.
    static S32 exportAllAvatarRiggedMeshes(const std::string& directory, bool export_blend);

    // Finds the first skin-wearing volume in the object tree (the object itself
    // or any of its child prims). Used by the single-object export so that
    // clicking any prim of a rigged linkset works.
    static LLVOVolume* findFirstSkinnedVolume(LLViewerObject* root);

    // Converts an exported <filebase>.dae into <filebase>.blend by locating and
    // driving Blender headlessly. Called from the texture-export completion
    // hook only, so the .blend is written after its textures already exist.
    static bool exportBlendFile(const std::string& dae_path);

private:
    struct ExportData
    {
        std::vector<LLVector3> positions;
        std::vector<LLVector3> normals;
        std::vector<LLVector2> texcoords;
        std::vector<U16> indices;
        std::string material_name;

        // Skin weights per vertex: [vertex][influence] = (joint_idx, weight)
        std::vector<std::vector<std::pair<S32, F32>>> skin_weights;
        bool has_skin;

        // Face texture (diffuse). texture_id is the viewer-side asset UUID;
        // texture_file is the PNG file name written next to the DAE (empty when
        // the face has no custom texture).
        LLUUID texture_id;
        std::string texture_file;

        // Actual SL face color (the per-face tint the build tools expose).
        // Used for faces without a custom texture so the export keeps the real
        // color instead of inventing one.
        LLColor4 face_color;
    };

    struct JointAccumData
    {
        std::string name;
        std::string parent_name;    // empty when the joint has no skinned ancestor (root)
        LLMatrix4   world_bind;     // bind-pose world transform (rest pose)
        LLMatrix4   local_bind;     // transform relative to parent / armature root
    };

    static bool buildSkeleton(const LLMeshSkinInfo* skin, std::vector<JointAccumData>& joints);
    static bool extractWeightData(const LLVolumeFace& vf, std::vector<std::vector<std::pair<S32, F32>>>& weights);
    static void gatherFaces(LLVolume* volume, const LLMeshSkinInfo* skin, LLVOVolume* volume_obj,
                            std::vector<ExportData>& faces);
    static void writeDAE(std::ofstream& out, const std::vector<ExportData>& faces,
                         const LLMeshSkinInfo* skin, const std::vector<JointAccumData>& joints,
                         const std::string& geom_id);
    static void writeAsset(std::ofstream& out);
    static void writeMaterials(std::ofstream& out, const std::vector<ExportData>& faces);
    static void queueTextureSaves(const std::string& dae_path, const std::vector<ExportData>& faces);
    static void writeGeometry(std::ofstream& out, const std::vector<ExportData>& faces, const std::string& geom_id);
    static void writeSkinning(std::ofstream& out, const std::vector<ExportData>& faces,
                              const LLMeshSkinInfo* skin, const std::string& geom_id, const std::string& skin_id);
    static void writeScene(std::ofstream& out, const std::vector<ExportData>& faces,
                           const std::vector<JointAccumData>& joints,
                           const LLMeshSkinInfo* skin,
                           const std::string& skin_id, const std::string& geom_id);
    static void writeJointNode(std::ofstream& out,
                               const std::vector<JointAccumData>& joints,
                               const std::map<std::string, std::vector<std::string> >& children,
                               const std::string& name, S32 depth);
    static std::string sanitizeId(const std::string& name);
    static void writeMatrix(std::ofstream& out, const LLMatrix4& mat);
    static void writeMatrix4a(std::ofstream& out, const LLMatrix4a& mat);

    // Blender .blend export
    static bool writeBlenderScript(const std::string& script_path, const std::string& dae_path, const std::string& blend_path);
    static std::string findBlenderExecutable();
};

#endif