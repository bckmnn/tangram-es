#include "textureAssets.h"

#include "util/util.h"
#include "platform.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Tangram {

TextureAssets::TextureAssets() {}
TextureAssets::~TextureAssets() {}

TextureBlob::~TextureBlob() {
    if (data) {
        stbi_image_free(data);
    }
}

bool TextureAssets::decode(const unsigned char* _data, size_t _size, TextureBlob& _blob, bool _flipOnLoad) {
    int comp;

    // stbi_load_from_memory loads the image as a serie of scanline starting from
    // the top-left corner of the image. When shouldFlip is set to true, the image
    // would be flipped vertically.
    stbi_set_flip_vertically_on_load((int)_flipOnLoad);

    if (_data != nullptr && _size != 0) {
        _blob.data = stbi_load_from_memory(_data, _size, &_blob.width, &_blob.height, &comp, STBI_rgb_alpha);
    }

    return _blob.data;
}

TextureOptions TextureAssets::rgbaLinear() {
    return {GL_RGBA, GL_RGBA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}};
}

bool TextureAssets::textureFiltering(const std::string& _filtering, TextureFiltering& _filter) {
    // TODO: return whether the filtering is mipmap is misleading, do a isMipMapFiltering instead

    if (_filtering == "linear") {
        _filter.min = _filter.mag = GL_LINEAR;
        return false;
    } else if (_filtering == "mipmap") {
        _filter.min = GL_LINEAR_MIPMAP_LINEAR;
        return true;
    } else if (_filtering == "nearest") {
        _filter.min = _filter.mag = GL_NEAREST;
        return false;
    } else {
        return false;
    }
}

std::shared_ptr<Texture> TextureAssets::nullTexture() {
    static std::shared_ptr<Texture> texture = nullptr;

    if (texture) { return texture; }

    // Default inconsistent texture data is set to a 1*1 pixel texture
    // This reduces inconsistent behavior when texture failed loading
    // texture data but a Tangram style shader requires a shader sampler
    GLuint blackPixel = 0x0000ff;

    texture = std::make_shared<Texture>(reinterpret_cast<unsigned char*>(&blackPixel), 1, 1);

    return texture;
}

std::shared_ptr<Texture> TextureAssets::get(std::string _uri, std::string _filtering, bool _flipOnLoad) {
    std::shared_ptr<Texture> texture;

    // Default texture option, rgba linear, clamp to edge
    TextureOptions options = rgbaLinear();

    if (tryFind(m_textures, _uri + _filtering, texture)) {
        return texture;
    } else {
        std::shared_ptr<TextureBlob> blob;

        bool generateMipmaps = false;

        if (textureFiltering(_filtering, options.filtering)) {
            generateMipmaps = true;
        }

        if (tryFind(m_textureBlobs, _uri, blob)) {
            texture = std::make_shared<Texture>(blob->data, blob->width,
                blob->height, options, generateMipmaps);

            // Store texture
            m_textures[_uri + _filtering] = texture;

            return texture;
        } else {
            blob = std::make_shared<TextureBlob>();

            unsigned int size;
            unsigned char* data;

            data = bytesFromFile(_uri.c_str(), PathType::resource, &size);

            if (decode(data, size, *blob, _flipOnLoad)) {
                texture = std::make_shared<Texture>(blob->data, blob->width,
                    blob->height, options, generateMipmaps);
            } else {
                return nullTexture();
            }

            // Store texture and blob
            m_textures[_uri + _filtering] = texture;
            m_textureBlobs[_uri] = blob;

            free(data);
        }
    }

    return texture;
}

}
