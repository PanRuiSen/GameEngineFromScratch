#include "SceneObjectTexture.hpp"

using namespace My;
using namespace std;

void SceneObjectTexture::LoadTextureAsync() {
    if(!m_asyncLoadFuture.valid())
    {
        m_asyncLoadFuture = async(launch::async, &SceneObjectTexture::LoadTexture, this);
    }
}

bool SceneObjectTexture::LoadTexture() {
    if(!g_pAssetLoader->FileExists(m_Name.c_str())) return false;

    cerr << "Start async loading of " << m_Name << endl;

    Image image;
    Buffer buf = g_pAssetLoader->SyncOpenAndReadBinary(m_Name.c_str());
    string ext = m_Name.substr(m_Name.find_last_of('.'));
    if (ext == ".jpg" || ext == ".jpeg")
    {
        JfifParser jfif_parser;
        image = jfif_parser.Parse(buf);
    }
    else if (ext == ".png")
    {
        PngParser png_parser;
        image = png_parser.Parse(buf);
    }
    else if (ext == ".bmp")
    {
        BmpParser bmp_parser;
        image = bmp_parser.Parse(buf);
    }
    else if (ext == ".tga")
    {
        TgaParser tga_parser;
        image = tga_parser.Parse(buf);
    }
    else if (ext == ".dds")
    {
        DdsParser dds_parser;
        image = dds_parser.Parse(buf);
    }
    else if (ext == ".hdr")
    {
        HdrParser hdr_parser;
        image = hdr_parser.Parse(buf);
    }

    // GPU does not support 24bit and 48bit textures, so adjust it
    if (image.bitcount == 24)
    {
        // DXGI does not have 24bit formats so we have to extend it to 32bit
        uint32_t new_pitch = image.pitch / 3 * 4;
        size_t data_size = new_pitch * image.Height;
        auto* data = new uint8_t[data_size];
        uint8_t* buf;
        uint8_t* src;
        for (uint32_t row = 0; row < image.Height; row++) {
            buf = data + row * new_pitch;
            src = image.data + row * image.pitch;
            for (uint32_t col = 0; col < image.Width; col++) {
                memcpy(buf, src, 3);
                memset(buf+3, 0x00, 1);  // set alpha to 0
                buf += 4;
                src += 3;
            }
        }

        delete image.data;
        image.data = data;
        image.data_size = data_size;
        image.pitch = new_pitch;
        image.bitcount = 32;
        
        // adjust mipmaps
        for (uint32_t mip = 0; mip < image.mipmap_count; mip++)
        {
            image.mipmaps[mip].pitch = image.mipmaps[mip].pitch / 3 * 4;
            image.mipmaps[mip].offset = image.mipmaps[mip].offset / 3 * 4;
            image.mipmaps[mip].data_size = image.mipmaps[mip].data_size / 3 * 4;
        }
    }
    else if (image.bitcount == 48)
    {
        // DXGI does not have 48bit formats so we have to extend it to 64bit
        uint32_t new_pitch = image.pitch / 3 * 4;
        size_t data_size = new_pitch * image.Height;
        auto* data = new uint8_t[data_size];
        uint8_t* buf;
        uint8_t* src;
        for (uint32_t row = 0; row < image.Height; row++) {
            buf = data + row * new_pitch;
            src = image.data + row * image.pitch;
            for (uint32_t col = 0; col < image.Width; col++) {
                memcpy(buf, src, 6);
                memset(buf+6, 0x00, 2); // set alpha to 0
                buf += 8;
                src += 6;
            }
        }

        delete image.data;
        image.data = data;
        image.data_size = data_size;
        image.pitch = new_pitch;
        image.bitcount = 64;
        
        // adjust mipmaps
        for (uint32_t mip = 0; mip < image.mipmap_count; mip++)
        {
            image.mipmaps[mip].pitch = image.mipmaps[mip].pitch / 3 * 4;
            image.mipmaps[mip].offset = image.mipmaps[mip].offset / 3 * 4;
            image.mipmaps[mip].data_size = image.mipmaps[mip].data_size / 3 * 4;
        }
    }

    cerr << "End async loading of " << m_Name << endl;

    m_pImage = make_shared<Image>(image);

    return true;
}

std::shared_ptr<Image> SceneObjectTexture::GetTextureImage()
{ 
    if(!m_asyncLoadFuture.valid())
    {
        LoadTextureAsync();
    }

    assert(m_asyncLoadFuture.valid());

    m_asyncLoadFuture.wait();
    assert(m_asyncLoadFuture.get());
    return m_pImage;
}
