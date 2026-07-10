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
    // Export a rigged mesh from a viewer object to DAE file
    static bool exportRiggedMesh(const std::string& filename, LLViewerObject* object);

    // Export from raw model data (for testing / non-viewer use)
    static bool exportModelToDAE(const std::string& filename,
                                 LLModel* model,
                                 const LLMeshSkinInfo* skin,
                                 const std::string& model_name);

private:
    struct ExportData
    {
        std::vector<LLVector3> positions;
        std::vector<LLVector3> normals;
        std::vector<LLVector2> texcoords;
        std::vector<U16> indices;
        std::string material_name;
    };

    static void gatherFaces(LLModel* model, std::vector<ExportData>& faces);
    static void writeDAE(std::ofstream& out,
                         const std::vector<ExportData>& faces,
                         const LLMeshSkinInfo* skin,
                         const std::string& model_name);
    static void writeAsset(std::ofstream& out);
    static void writeGeometry(std::ofstream& out, const std::vector<ExportData>& faces, const std::string& geom_id);
    static void writeSkin(std::ofstream& out, const LLMeshSkinInfo* skin, const std::string& geom_id, const std::string& skin_id, U32 num_vertices);
    static void writeScene(std::ofstream& out, const LLMeshSkinInfo* skin, const std::string& skin_id, const std::string& geom_id);
    static std::string sanitizeId(const std::string& name);
};

#endif
