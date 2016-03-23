#include "treeStyle.h"

#include "tangram.h"
#include "platform.h"
#include "gl/shaderProgram.h"
#include "gl/mesh.h"
#include "scene/stops.h"
#include "scene/drawRule.h"
#include "tile/tile.h"
#include "util/builders.h"

#define PAR_SHAPES_IMPLEMENTATION
#include "par/par_shapes.h"

#include "glm/vec3.hpp"
#include "glm/gtc/type_precision.hpp"

namespace Tangram {

struct TreeStyleVertex {
    glm::vec4 position;
    glm::vec3 normal;
    glm::vec4 color;
};

TreeStyle::TreeStyle(std::string _name, Blending _blendMode, GLenum _drawMode)
    : Style(_name, _blendMode, _drawMode)
{}

void TreeStyle::constructVertexLayout() {

    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 4, GL_FLOAT, false, 0},
        {"a_normal", 3, GL_FLOAT, false, 0},
        {"a_color", 4, GL_FLOAT, true, 0},
    }));

}

void TreeStyle::constructShaderProgram() {

    std::string vertShaderSrcStr = stringFromFile("shaders/tree.vs", PathType::internal);
    std::string fragShaderSrcStr = stringFromFile("shaders/tree.fs", PathType::internal);

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);

}

struct TreeStyleBuilder : public StyleBuilder {

public:

    void setup(const Tile& _tile) override;

    void addFeature(const Feature& _feat, const DrawRule& _rule) override;

    std::unique_ptr<StyledMesh> build() override;

    TreeStyleBuilder(const TreeStyle& _style) : StyleBuilder(_style), m_style(_style) {}

    virtual const Style& style() const override { return m_style; }

private:

    void addTree(const glm::vec3& point, par_shapes_mesh* parMesh, uint32_t order);

    const TreeStyle& m_style;
    
    MeshData<TreeStyleVertex> m_meshData;

    float m_tileUnitsPerMeter;

};

void TreeStyleBuilder::setup(const Tile& tile) {
    m_tileUnitsPerMeter = tile.getInverseScale();
    m_meshData.clear();
}

std::unique_ptr<StyledMesh> TreeStyleBuilder::build() {
    if (m_meshData.vertices.empty()) { return nullptr; }

    auto mesh = std::make_unique<Mesh<TreeStyleVertex>>(m_style.vertexLayout(), m_style.drawMode());
    mesh->compile(m_meshData);

    m_meshData.clear();

    return std::move(mesh);
}

void TreeStyleBuilder::addTree(const glm::vec3& point, par_shapes_mesh* parMesh, uint32_t order) {
    static float scale = 15.f;

    size_t vertexOffset = m_meshData.vertices.size();

    for (int i = 0; i < parMesh->npoints; i++) {
        TreeStyleVertex vertex {
            glm::vec4(parMesh->points[i*3+0],
                      parMesh->points[i*3+1],
                      parMesh->points[i*3+2] + 1.0, 0.0),
            glm::vec3(0.0),
            glm::vec4(0.5, 0.66, 0.5, 1.0)
        };
        vertex.position *= m_tileUnitsPerMeter * scale;
        vertex.position.x += point.x;
        vertex.position.y += point.y;
        vertex.position.w = order;
        m_meshData.vertices.push_back(vertex);
    }

    std::vector<uint16_t> indices;
    for (int i = 0; i < parMesh->ntriangles * 3; i++) {
        indices.push_back(parMesh->triangles[i]);
    }

    /// Compute normals
    for (int i = 0; i < parMesh->ntriangles * 3; i += 3) {
        auto& v0 = m_meshData.vertices[vertexOffset + indices[i]];
        auto& v1 = m_meshData.vertices[vertexOffset + indices[i+1]];
        auto& v2 = m_meshData.vertices[vertexOffset + indices[i+2]];

        glm::vec3 normal = glm::cross(glm::vec3(v1.position - v0.position),
                                      glm::vec3(v2.position - v0.position));

        v0.normal += normal;
        v1.normal += normal;
        v2.normal += normal;
    }

    for (auto& v : m_meshData.vertices) {
        glm::normalize(v.normal);
    }

    m_meshData.indices.insert(m_meshData.indices.end(),
                              indices.begin(),
                              indices.end());

    m_meshData.offsets.emplace_back(indices.size(),
                                    m_meshData.vertices.size());

}

void TreeStyleBuilder::addFeature(const Feature& _feat, const DrawRule& _rule) {

    par_shapes_mesh* parMesh = par_shapes_create_empty();
    par_shapes_mesh* icosahedron = par_shapes_create_icosahedron();
    par_shapes_mesh* cylinder = par_shapes_create_cylinder(10, 1);

    par_shapes_translate(icosahedron, 0.2, 0.2, 0.5);
    par_shapes_scale(cylinder, 0.2, 0.2, 1.5);
    par_shapes_translate(cylinder, 0.05, 0.05, -1.0);

    par_shapes_merge(parMesh, cylinder);
    par_shapes_merge(parMesh, icosahedron);

    uint32_t order;

    _rule.get(StyleParamKey::order, order);
    switch (_feat.geometryType) {
        case GeometryType::points: {
            for (auto& point : _feat.points) {
                addTree(point, parMesh, order);
            }
        } break;
        case GeometryType::lines: {
            for (auto& line : _feat.lines) {
                for (auto& point : line) {
                    addTree(point, parMesh, order);
                }
            }
        } break;
        case GeometryType::polygons: {
            for (auto& polygon : _feat.polygons) {
                for (const auto& line : polygon) {
                    for (auto& point : line) {
                        addTree(point, parMesh, order);
                    }
                }
            }
        } break;
        case GeometryType::unknown:
        default: {}
    }

    par_shapes_free_mesh(parMesh);
    par_shapes_free_mesh(cylinder);
    par_shapes_free_mesh(icosahedron);

}

std::unique_ptr<StyleBuilder> TreeStyle::createBuilder() const {
    return std::make_unique<TreeStyleBuilder>(*this);
}

}
