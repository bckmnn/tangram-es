#pragma once

#include <string>
#include <memory>
#include <unordered_map>

#include "gl/texture.h"


namespace Tangram {

struct TextureBlob {
    unsigned char* data = nullptr;
    int width = 0;
    int height = 0;

    ~TextureBlob();
};

class TextureAssets {

public:
    TextureAssets();
    ~TextureAssets();

    static bool decode(const unsigned char* _data, size_t _size, TextureBlob& _blob, bool _flipOnLoad = false);
    static bool textureFiltering(const std::string& _filtering, TextureFiltering& _filter);
    static std::shared_ptr<Texture> nullTexture();
    static TextureOptions rgbaLinear();

    std::shared_ptr<Texture> get(std::string _uri, std::string _filtering, bool _flipOnLoad = false);

private:

    std::unordered_map<std::string, std::shared_ptr<TextureBlob>> m_textureBlobs;
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;

};

}