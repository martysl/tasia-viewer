#include "llviewerprecompiledheaders.h"
#include "fsdaeexporter.h"
#include "llvolume.h"
#include "llvoavatar.h"
#include "llvovolume.h"
#include "llviewerobject.h"
#include "llviewercontrol.h"
#include "llfilepicker.h"
#include "llagent.h"
#include "llmeshrepository.h"
#include "llvolume.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>

//=============================================================================
// Public API
//=============================================================================

bool FSDAEExporter::exportRiggedMesh(const std::string& filename, LLViewerObject* object)
{
    if (!object) return false;

    LLVOVolume* volume_obj = dynamic_cast<LLVOVolume*>(object);
    if (!volume_obj) return false;

    const LLMeshSkinInfo* skin = volume_obj->getSkinInfo();
    LLVolume* volume = object->getVolume();
    if (!volume) return false;

    std::vector<ExportData> faces;
    gatherFaces(volume, faces);
    if (faces.empty()) return false;

    std::string model_name = object->getAttachmentItemName();
    if (model_name.empty())
        model_name = "ExportedMesh";

    std::ofstream out(filename.c_str());
    if (!out.is_open()) return false;

    writeDAE(out, faces, skin, model_name);
    out.close();

    LL_INFOS("DAEExport") << "Exported rigged mesh '" << model_name << "' to " << filename << LL_ENDL;
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
            if (weight > 0.001f)
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

void FSDAEExporter::gatherFaces(LLVolume* volume, std::vector<ExportData>& faces)
{
    S32 num_faces = volume->getNumVolumeFaces();
    for (S32 i = 0; i < num_faces; ++i)
    {
        const LLVolumeFace& vf = volume->getVolumeFace(i);
        if (vf.mNumVertices <= 0 || vf.mNumIndices <= 0) continue;

        ExportData face;
        face.material_name = "Material_" + std::to_string(i);
        face.has_skin = false;

        face.positions.resize(vf.mNumVertices);
        face.normals.resize(vf.mNumVertices);
        face.texcoords.resize(vf.mNumVertices);
        face.indices.assign(vf.mIndices, vf.mIndices + vf.mNumIndices);

        for (S32 j = 0; j < vf.mNumVertices; ++j)
        {
            const F32* pos = vf.mPositions[j].getF32ptr();
            face.positions[j].set(pos[0], pos[1], pos[2]);
            const F32* nrm = vf.mNormals[j].getF32ptr();
            face.normals[j].set(nrm[0], nrm[1], nrm[2]);
            face.texcoords[j] = vf.mTexCoords[j];
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
                              const std::string& model_name)
{
    std::string geom_id = sanitizeId(model_name.empty() ? "Mesh" : model_name);
    std::string skin_id = geom_id + "-skin";

    out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    out << "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n";

    writeAsset(out);
    writeGeometry(out, faces, geom_id);

    if (skin && !skin->mJointNames.empty())
    {
        bool has_weights = false;
        for (const auto& f : faces)
            if (f.has_skin) { has_weights = true; break; }

        if (has_weights)
            writeSkinning(out, faces, skin, geom_id, skin_id);
    }

    writeScene(out, skin, skin_id, geom_id);
    out << "</COLLADA>\n";
}

void FSDAEExporter::writeAsset(std::ofstream& out)
{
    out << "  <asset>\n";
    out << "    <contributor>\n";
    out << "      <authoring_tool>Tasia Viewer</authoring_tool>\n";
    out << "    </contributor>\n";
    out << "    <unit name=\"meter\" meter=\"1\"/>\n";
    out << "    <up_axis>Z_UP</up_axis>\n";
    out << "  </asset>\n";
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
        out << "        <triangles count=\"" << (f.indices.size() / 3) << "\" material=\"" << sanitizeId(f.material_name) << "\">\n";
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
                entry.push_back({w.first, w.second});
            all_weights.push_back(entry);
            total_unique_weights += (U32)entry.size();
        }
    }

    U32 num_vertices = (U32)all_weights.size();

    out << "  <library_controllers>\n";
    out << "    <controller id=\"" << skin_id << "\">\n";
    out << "      <skin source=\"#" << geom_id << "-mesh\">\n";

    // Bind shape matrix
    out << "        <bind_shape_matrix>";
    for (U32 i = 0; i < 16; ++i)
        out << skin->mBindShapeMatrix.mMatrix[i] << " ";
    out << "</bind_shape_matrix>\n";

    // Joint names
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

    // Bind poses
    out << "        <source id=\"" << skin_id << "-bind-poses\">\n";
    out << "          <float_array id=\"" << skin_id << "-bind-poses-array\" count=\"" << (num_joints * 16) << "\">";
    for (const auto& mat : skin->mInvBindMatrix)
        for (U32 j = 0; j < 16; ++j)
            out << mat.mMatrix[j] << " ";
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

void FSDAEExporter::writeScene(std::ofstream& out,
                                const LLMeshSkinInfo* skin,
                                const std::string& skin_id,
                                const std::string& geom_id)
{
    out << "  <library_visual_scenes>\n";
    out << "    <visual_scene id=\"Scene\" name=\"Scene\">\n";

    bool has_skin = (skin && !skin->mJointNames.empty());

    // Skeleton joints
    if (has_skin)
    {
        out << "      <node id=\"Armature\" name=\"Armature\" type=\"NODE\">\n";
        for (const auto& name : skin->mJointNames)
        {
            std::string sname = sanitizeId(name);
            out << "        <node id=\"" << sname << "\" name=\"" << sname << "\" sid=\"" << sname << "\" type=\"JOINT\">\n";
            out << "          <matrix sid=\"transform\">0 0 0 0  0 0 0 0  0 0 0 0  0 0 0 0</matrix>\n";
            out << "        </node>\n";
        }
        out << "      </node>\n";
    }

    // Mesh instance
    out << "      <node id=\"" << geom_id << "\" name=\"" << geom_id << "\" type=\"NODE\">\n";
    out << "        <matrix sid=\"transform\">1 0 0 0  0 1 0 0  0 0 1 0  0 0 0 1</matrix>\n";

    if (has_skin && !skin->mJointNames.empty())
    {
        out << "        <instance_controller url=\"#" << skin_id << "\">\n";
        out << "          <skeleton>#" << sanitizeId(skin->mJointNames[0]) << "</skeleton>\n";
        out << "          <bind_material>\n";
        out << "            <technique_common>\n";
        out << "            </technique_common>\n";
        out << "          </bind_material>\n";
        out << "        </instance_controller>\n";
    }
    else
    {
        out << "        <instance_geometry url=\"#" << geom_id << "-mesh\">\n";
        out << "          <bind_material>\n";
        out << "            <technique_common>\n";
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
