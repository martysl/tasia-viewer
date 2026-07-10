#ifndef FS_DAE_EXPORTER_H
#define FS_DAE_EXPORTER_H

#include "llmodel.h"
#include <string>
#include <vector>

class LLViewerObject;
class LLMeshSkinInfo;

class FSDAEExporter
{
public:
    static bool exportRiggedMesh(const std::string& filename, LLViewerObject* object);

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
    };

    static bool extractWeightData(const LLVolumeFace& vf, std::vector<std::vector<std::pair<S32, F32>>>& weights);
    static void gatherFaces(LLVolume* volume, std::vector<ExportData>& faces);
    static void writeDAE(std::ofstream& out, const std::vector<ExportData>& faces,
                         const LLMeshSkinInfo* skin, const std::string& model_name);
    static void writeAsset(std::ofstream& out);
    static void writeGeometry(std::ofstream& out, const std::vector<ExportData>& faces, const std::string& geom_id);
    static void writeSkinning(std::ofstream& out, const std::vector<ExportData>& faces,
                              const LLMeshSkinInfo* skin, const std::string& geom_id, const std::string& skin_id);
    static void writeScene(std::ofstream& out, const LLMeshSkinInfo* skin,
                           const std::string& skin_id, const std::string& geom_id);
    static std::string sanitizeId(const std::string& name);
};

#endif
