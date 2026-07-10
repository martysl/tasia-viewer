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
#include <fstream>
#include <sstream>
#include <iomanip>

//=============================================================================
// Public API
//=============================================================================

bool FSDAEExporter::exportRiggedMesh(const std::string& filename, LLViewerObject* object)
{
    if (!object) return false;

    // Get skin info from the object
    LLVOVolume* volume_obj = dynamic_cast<LLVOVolume*>(object);
    if (!volume_obj) return false;

    const LLMeshSkinInfo* skin = volume_obj->getSkinInfo();

    // Get volume for mesh data
    LLVolume* volume = object->getVolume();
    if (!volume) return false;

    // Extract face data directly from volume faces
    std::vector<ExportData> faces;
    S32 num_faces = volume->getNumVolumeFaces();

    for (S32 i = 0; i < num_faces; ++i)
    {
        const LLVolumeFace& vf = volume->getVolumeFace(i);
        if (vf.mNumVertices <= 0 || vf.mNumIndices <= 0) continue;

        ExportData face;

        face.positions.assign(vf.mPositions, vf.mPositions + vf.mNumVertices);
        face.normals.assign(vf.mNormals, vf.mNormals + vf.mNumVertices);
        face.texcoords.assign(vf.mTexCoords, vf.mTexCoords + vf.mNumVertices);
        face.indices.assign(vf.mIndices, vf.mIndices + vf.mNumIndices);

        faces.push_back(face);
    }

    if (faces.empty()) return false;

    std::string model_name = object->getAttachmentItemName();
    if (model_name.empty())
    {
        model_name = object->getFirstName();
    }

    std::ofstream out(filename.c_str());
    if (!out.is_open()) return false;

    writeDAE(out, faces, skin, model_name);
    out.close();
    return true;
}

bool FSDAEExporter::exportModelToDAE(const std::string& filename,
                                      LLModel* model,
                                      const LLMeshSkinInfo* skin,
                                      const std::string& model_name)
{
    if (!model) return false;

    std::vector<ExportData> faces;
    gatherFaces(model, faces);
    if (faces.empty()) return false;

    std::ofstream out(filename.c_str());
    if (!out.is_open()) return false;

    writeDAE(out, faces, skin, model_name);
    out.close();
    return true;
}

//=============================================================================
// Internal helpers
//=============================================================================

void FSDAEExporter::gatherFaces(LLModel* model, std::vector<ExportData>& faces)
{
    S32 num_faces = model->getNumVolumeFaces();
    for (S32 i = 0; i < num_faces; ++i)
    {
        const LLVolumeFace& volface = model->getVolumeFace(i);
        if (volface.mNumVertices <= 0 || volface.mNumIndices <= 0) continue;

        ExportData face;
        face.material_name = model->getMaterialList()[i];

        face.positions.resize(volface.mNumVertices);
        face.normals.resize(volface.mNumVertices);
        face.texcoords.resize(volface.mNumVertices);

        for (S32 j = 0; j < volface.mNumVertices; ++j)
        {
            face.positions[j] = volface.mPositions[j];
            face.normals[j] = volface.mNormals[j];
            face.texcoords[j] = volface.mTexCoords[j];
        }

        face.indices.resize(volface.mNumIndices);
        for (S32 j = 0; j < volface.mNumIndices; ++j)
        {
            face.indices[j] = volface.mIndices[j];
        }

        faces.push_back(face);
    }
}

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
        // Count total vertices for the skin
        U32 total_verts = 0;
        for (const auto& face : faces) total_verts += static_cast<U32>(face.positions.size());
        writeSkin(out, skin, geom_id, skin_id, total_verts);
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
    out << "    <created>" << time(nullptr) << "</created>\n";
    out << "    <modified>" << time(nullptr) << "</modified>\n";
    out << "    <unit name=\"meter\" meter=\"1\"/>\n";
    out << "    <up_axis>Z_UP</up_axis>\n";
    out << "  </asset>\n";
}

void FSDAEExporter::writeGeometry(std::ofstream& out, const std::vector<ExportData>& faces, const std::string& geom_id)
{
    out << "  <library_geometries>\n";
    out << "    <geometry id=\"" << geom_id << "-mesh\" name=\"" << geom_id << "\">\n";
    out << "      <mesh>\n";

    // Collect all vertices into a single array
    std::vector<LLVector3> all_pos;
    std::vector<LLVector3> all_norm;
    std::vector<LLVector2> all_uv;
    U32 base_vertex = 0;

    // Count total
    U32 total_verts = 0, total_idx = 0;
    for (const auto& face : faces)
    {
        total_verts += static_cast<U32>(face.positions.size());
        total_idx += static_cast<U32>(face.indices.size());
    }

    all_pos.reserve(total_verts);
    all_norm.reserve(total_verts);
    all_uv.reserve(total_verts);

    // Position source
    out << "        <source id=\"" << geom_id << "-mesh-positions\">\n";
    out << "          <float_array id=\"" << geom_id << "-mesh-positions-array\" count=\"" << (total_verts * 3) << "\">";
    for (const auto& face : faces)
    {
        for (const auto& v : face.positions)
        {
            out << v.mV[0] << " " << v.mV[1] << " " << v.mV[2] << " ";
            all_pos.push_back(v);
        }
    }
    out << "</float_array>\n";
    out << "          <technique_common>\n";
    out << "            <accessor source=\"#" << geom_id << "-mesh-positions-array\" count=\"" << total_verts << "\" stride=\"3\">\n";
    out << "              <param name=\"X\" type=\"float\"/>\n";
    out << "              <param name=\"Y\" type=\"float\"/>\n";
    out << "              <param name=\"Z\" type=\"float\"/>\n";
    out << "            </accessor>\n";
    out << "          </technique_common>\n";
    out << "        </source>\n";

    // Normal source
    out << "        <source id=\"" << geom_id << "-mesh-normals\">\n";
    out << "          <float_array id=\"" << geom_id << "-mesh-normals-array\" count=\"" << (total_verts * 3) << "\">";
    for (const auto& face : faces)
    {
        for (const auto& v : face.normals)
        {
            out << v.mV[0] << " " << v.mV[1] << " " << v.mV[2] << " ";
            all_norm.push_back(v);
        }
    }
    out << "</float_array>\n";
    out << "          <technique_common>\n";
    out << "            <accessor source=\"#" << geom_id << "-mesh-normals-array\" count=\"" << total_verts << "\" stride=\"3\">\n";
    out << "              <param name=\"X\" type=\"float\"/>\n";
    out << "              <param name=\"Y\" type=\"float\"/>\n";
    out << "              <param name=\"Z\" type=\"float\"/>\n";
    out << "            </accessor>\n";
    out << "          </technique_common>\n";
    out << "        </source>\n";

    // Texcoord source
    out << "        <source id=\"" << geom_id << "-mesh-map-0\">\n";
    out << "          <float_array id=\"" << geom_id << "-mesh-map-0-array\" count=\"" << (total_verts * 2) << "\">";
    for (const auto& face : faces)
    {
        for (const auto& v : face.texcoords)
        {
            out << v.mV[0] << " " << (1.0f - v.mV[1]) << " ";
            all_uv.push_back(v);
        }
    }
    out << "</float_array>\n";
    out << "          <technique_common>\n";
    out << "            <accessor source=\"#" << geom_id << "-mesh-map-0-array\" count=\"" << total_verts << "\" stride=\"2\">\n";
    out << "              <param name=\"S\" type=\"float\"/>\n";
    out << "              <param name=\"T\" type=\"float\"/>\n";
    out << "            </accessor>\n";
    out << "          </technique_common>\n";
    out << "        </source>\n";

    // Vertices
    out << "        <vertices id=\"" << geom_id << "-mesh-vertices\">\n";
    out << "          <input semantic=\"POSITION\" source=\"#" << geom_id << "-mesh-positions\"/>\n";
    out << "        </vertices>\n";

    // Triangles
    for (const auto& face : faces)
    {
        out << "        <triangles count=\"" << (face.indices.size() / 3) << "\" material=\"" << sanitizeId(face.material_name) << "\">\n";
        out << "          <input semantic=\"VERTEX\" source=\"#" << geom_id << "-mesh-vertices\" offset=\"0\"/>\n";
        out << "          <input semantic=\"NORMAL\" source=\"#" << geom_id << "-mesh-normals\" offset=\"1\"/>\n";
        out << "          <input semantic=\"TEXCOORD\" source=\"#" << geom_id << "-mesh-map-0\" offset=\"2\"/>\n";
        out << "          <p>";
        for (U32 j = 0; j < face.indices.size(); j += 3)
        {
            out << face.indices[j] << " " << face.indices[j] << " "
                << face.indices[j] << " "
                << face.indices[j+1] << " " << face.indices[j+1] << " "
                << face.indices[j+1] << " "
                << face.indices[j+2] << " " << face.indices[j+2] << " "
                << face.indices[j+2] << " ";
        }
        out << "</p>\n";
        out << "        </triangles>\n";
    }

    out << "      </mesh>\n";
    out << "    </geometry>\n";
    out << "  </library_geometries>\n";
}

void FSDAEExporter::writeSkin(std::ofstream& out, const LLMeshSkinInfo* skin,
                               const std::string& geom_id, const std::string& skin_id,
                               U32 num_vertices)
{
    if (!skin || skin->mJointNames.empty()) return;

    U32 num_joints = static_cast<U32>(skin->mJointNames.size());

    out << "  <library_controllers>\n";
    out << "    <controller id=\"" << skin_id << "\">\n";
    out << "      <skin source=\"#" << geom_id << "-mesh\">\n";

    // Bind shape matrix
    out << "        <bind_shape_matrix>1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1</bind_shape_matrix>\n";

    // Joint names source
    out << "        <source id=\"" << skin_id << "-joints\">\n";
    out << "          <Name_array id=\"" << skin_id << "-joints-array\" count=\"" << num_joints << "\">";
    for (const auto& name : skin->mJointNames)
    {
        out << sanitizeId(name) << " ";
    }
    out << "</Name_array>\n";
    out << "          <technique_common>\n";
    out << "            <accessor source=\"#" << skin_id << "-joints-array\" count=\"" << num_joints << "\" stride=\"1\">\n";
    out << "              <param name=\"JOINT\" type=\"Name\"/>\n";
    out << "            </accessor>\n";
    out << "          </technique_common>\n";
    out << "        </source>\n";

    // Bind poses source
    out << "        <source id=\"" << skin_id << "-bind-poses\">\n";
    out << "          <float_array id=\"" << skin_id << "-bind-poses-array\" count=\"" << (num_joints * 16) << "\">";
    for (const auto& bind : skin->mInvBindMatrix)
    {
        for (U32 j = 0; j < 16; ++j)
        {
            out << bind.mMatrix[j] << " ";
        }
    }
    out << "</float_array>\n";
    out << "          <technique_common>\n";
    out << "            <accessor source=\"#" << skin_id << "-bind-poses-array\" count=\"" << num_joints << "\" stride=\"16\">\n";
    out << "              <param name=\"TRANSFORM\" type=\"float4x4\"/>\n";
    out << "            </accessor>\n";
    out << "          </technique_common>\n";
    out << "        </source>\n";

    // Weights source
    out << "        <source id=\"" << skin_id << "-weights\">\n";
    out << "          <float_array id=\"" << skin_id << "-weights-array\" count=\"0\">";
    out << "</float_array>\n";
    out << "          <technique_common>\n";
    out << "            <accessor source=\"#" << skin_id << "-weights-array\" count=\"0\" stride=\"1\">\n";
    out << "              <param name=\"WEIGHT\" type=\"float\"/>\n";
    out << "            </accessor>\n";
    out << "          </technique_common>\n";
    out << "        </source>\n";

    out << "        <joints>\n";
    out << "          <input semantic=\"JOINT\" source=\"#" << skin_id << "-joints\"/>\n";
    out << "          <input semantic=\"INV_BIND_MATRIX\" source=\"#" << skin_id << "-bind-poses\"/>\n";
    out << "        </joints>\n";

    out << "        <vertex_weights count=\"" << num_vertices << "\">\n";
    out << "          <input semantic=\"JOINT\" source=\"#" << skin_id << "-joints\" offset=\"0\"/>\n";
    out << "          <input semantic=\"WEIGHT\" source=\"#" << skin_id << "-weights\" offset=\"1\"/>\n";
    out << "          <vcount>";
    for (U32 i = 0; i < num_vertices; ++i) out << "0 ";
    out << "</vcount>\n";
    out << "          <v></v>\n";
    out << "        </vertex_weights>\n";

    out << "      </skin>\n";
    out << "    </controller>\n";
    out << "  </library_controllers>\n";
}

void FSDAEExporter::writeScene(std::ofstream& out, const LLMeshSkinInfo* skin,
                                const std::string& skin_id, const std::string& geom_id)
{
    out << "  <library_visual_scenes>\n";
    out << "    <visual_scene id=\"Scene\" name=\"Scene\">\n";

    // Write skeleton nodes
    if (skin && !skin->mJointNames.empty())
    {
        // Create joint nodes in the scene
        out << "      <node id=\"Armature\" name=\"Armature\" type=\"NODE\">\n";
        for (const auto& name : skin->mJointNames)
        {
            out << "        <node id=\"" << sanitizeId(name) << "\" name=\"" << sanitizeId(name) << "\" type=\"JOINT\">\n";
            out << "          <matrix sid=\"transform\">1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1</matrix>\n";
            out << "        </node>\n";
        }
        out << "      </node>\n";
    }

    // Instance the mesh
    out << "      <node id=\"" << geom_id << "\" name=\"" << geom_id << "\" type=\"NODE\">\n";
    out << "        <matrix sid=\"transform\">1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1</matrix>\n";
    if (skin && !skin->mJointNames.empty())
    {
        out << "        <instance_controller url=\"#" << skin_id << "\">\n";
        out << "          <skeleton>#" << sanitizeId(skin->mJointNames[0]) << "</skeleton>\n";
        out << "        </instance_controller>\n";
    }
    else
    {
        out << "        <instance_geometry url=\"#" << geom_id << "-mesh\"/>\n";
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
        if (!isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-')
        {
            c = '_';
        }
    }
    if (result.empty() || isdigit(static_cast<unsigned char>(result[0])))
    {
        result = "ID_" + result;
    }
    return result;
}
