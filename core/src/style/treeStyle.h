#pragma once

#pragma once

#include "style.h"

namespace Tangram {

class TreeStyle : public Style {

protected:

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;

    virtual std::unique_ptr<StyleBuilder> createBuilder() const override;

public:

    TreeStyle(std::string _name, Blending _blendMode = Blending::none, GLenum _drawMode = GL_TRIANGLES);

    virtual ~TreeStyle() {}

};

}
