#include "tile/tileBuilder.h"

#include "gl/mesh.h"

#include "data/dataSource.h"

#include "scene/dataLayer.h"
#include "scene/scene.h"
#include "style/style.h"
#include "tile/tile.h"
#include "tile/tileTask.h"


namespace Tangram {

TileBuilder::TileBuilder() {}

TileBuilder::~TileBuilder() {}

StyleBuilder* TileBuilder::getStyleBuilder(const std::string& _name) {
    auto it = m_styleBuilder.find(_name);
    if (it == m_styleBuilder.end()) { return nullptr; }

    return it->second.get();
}

void TileBuilder::setScene(std::shared_ptr<Scene> _scene) {

    m_scene = _scene;

    m_styleContext.initFunctions(*_scene);

    // Initialize StyleBuilders
    m_styleBuilder.clear();
    for (const auto& style : _scene->styles()) {
        m_styleBuilder[style->getName()] = style->createBuilder();
    }
}

bool TileBuilder::beginLayer(const std::string& _layerName) {

    m_activeLayers.clear();

    for (const auto& layer : m_scene->layers()) {

        if (layer.source() != m_activeSource) {
            continue;
        }

        if (!_layerName.empty()) {
            const auto& dlc = layer.collections();
            if (std::find(dlc.begin(), dlc.end(), _layerName) == dlc.end()) {
                continue;
            }
        }
        m_activeLayers.push_back(&layer);
    }

    return !m_activeLayers.empty();
}

// TileDataSink callback
bool TileBuilder::matchFeature(const Feature& _feature) {
    m_matchedLayer = nullptr;

    for (auto* layer : m_activeLayers) {
        if(m_ruleSet.match(_feature, *layer, m_styleContext)) {
            // keep reference to matched layer for addFeature
            m_matchedLayer = layer;
            return true;
        }
    }

    return false;
}

// TileDataSink callback
void TileBuilder::addFeature(const Feature& _feature) {

    // Require that matchFeature found a layer.
    if (!m_matchedLayer) { return; }

    for (const auto* layer : m_activeLayers) {

        if (m_matchedLayer) {
            // Skip until first matched layer
            if (m_matchedLayer != layer) {
                continue;
            }
            m_matchedLayer = nullptr;

        } else if (!m_ruleSet.match(_feature, *layer, m_styleContext)) {
            continue;
        }

        m_ruleSet.apply(_feature, *layer, m_styleContext, *this);
    }
}

bool TileBuilder::build(TileTask& _task) {

    auto tile = std::make_shared<Tile>(_task.tileId(),
                                       *m_scene->mapProjection(),
                                       &_task.source());

    tile->initGeometry(m_scene->styles().size());

    m_styleContext.setKeywordZoom(_task.tileId().s);

    for (auto& builder : m_styleBuilder) {
        if (builder.second) {
            builder.second->setup(*tile);
        }
    }

    m_activeSource = _task.source().name();

    // Pass 'this' as TileDataSink
    if (!_task.source().process(_task, *m_scene->mapProjection(), *this)) {
        _task.cancel();
        return false;
    }

    for (auto& builder : m_styleBuilder) {
        tile->setMesh(builder.second->style(), builder.second->build());
    }

    _task.setTile(std::move(tile));

    return true;
}

}
