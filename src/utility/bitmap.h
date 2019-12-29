#ifndef __BITMAP_H
#define __BITMAP_H

#include <algorithm>
#include <string>
#include <iostream>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "../ext/stb_images/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../ext/stb_images/stb_image_write.h"

class Bitmap
{
public:
    Bitmap() {}
    Bitmap(const std::string &filename)
    {
        InputPNG(filename);
    }
    Bitmap(const int &width, const int &height, const int &channels)
        : m_width(width), m_height(height), m_channels(channels)
    {
        Initalize();
    }
    ~Bitmap()
    {
        delete[] m_data;
    }

    void Initalize()
    {
        m_data = new unsigned char[m_width * m_height * m_channels];
        memset(m_data, 0, sizeof(unsigned char) * (m_width * m_height * m_channels));
    }

    void InputPNG(const std::string &path)
    {
        m_data = stbi_load(path.c_str(), &m_width, &m_height, &m_channels, 0);
        if (!m_data)
        {
            std::cerr << "Fail to load " + path + "\n";
        }
    }

    void OutputPNG(const std::string &path) const
    {
        stbi_write_png(path.c_str(), m_width, m_height, m_channels, m_data, 0);
    }

    unsigned char *GetPixel(const int &px, const int &py) const
    {
        return &m_data[(px * m_width + py) * m_channels];
    }

    std::vector<unsigned char> GetSide(const int &orientation) const
    {
        assert(m_width == m_height);
        std::vector<unsigned char> ret(m_width * m_channels);
        switch (orientation)
        {
        case 0:
        {
            // Up : (0, 0) ... (0, width - 1)
            unsigned char *pixel = GetPixel(0, 0);
            memcpy(&ret[0], pixel, sizeof(unsigned char) * m_width * m_channels);
            break;
        }
        case 1:
        {
            // Down : (width - 1, 0) ... (width - 1, width - 1)
            unsigned char *pixel = GetPixel(m_width - 1, 0);
            memcpy(&ret[0], pixel, sizeof(unsigned char) * m_width * m_channels);
            break;
        }
        case 2:
        {
            // Left : (0, 0) ... (width - 1, 0)
            for (int i = 0; i < m_width; i++)
            {
                unsigned char *pixel = GetPixel(i, 0);
                memcpy(&ret[i * m_channels], pixel, sizeof(unsigned char) * m_channels);
            }
            break;
        }
        case 3:
        {
            // Right : (0, width - 1) ... (width - 1, width - 1)
            for (int i = 0; i < m_width; i++)
            {
                unsigned char *pixel = GetPixel(i, m_width - 1);
                memcpy(&ret[i * m_channels], pixel, sizeof(unsigned char) * m_channels);
            }
            break;
        }
        }
        return ret;
    }

    void Merge(const Bitmap &bitmap, const int &row, const int &col)
    {
        int blockSize = bitmap.m_height;
        for (int i = 0; i < blockSize; i++)
        {
            unsigned char *dst = GetPixel(row * blockSize + i, col * blockSize);
            unsigned char *src = bitmap.GetPixel(i, 0);
            memcpy(dst, src, sizeof(unsigned char) * blockSize * m_channels);
        }
    }

    unsigned char *m_data;
    int m_width, m_height, m_channels;
};

inline std::vector<Bitmap> Split(const Bitmap &bitmap, const int &w)
{
    int row = bitmap.m_width / w, col = bitmap.m_height / w;
    std::vector<Bitmap> ret(row * col);
    int idx = 0;
    for (int x = 0; x < bitmap.m_height; x += w)
    {
        for (int y = 0; y < bitmap.m_width; y += w)
        {
            Bitmap &now = ret[idx];
            now.m_channels = bitmap.m_channels;
            now.m_height = now.m_width = w;
            now.Initalize();
            for (int i = 0; i < w; i++)
            {
                unsigned char *nowPixel = now.GetPixel(i, 0);
                unsigned char *bitmapPixel = bitmap.GetPixel(x + i, y);
                memcpy(nowPixel, bitmapPixel, sizeof(unsigned char) * w * bitmap.m_channels);
            }
            idx++;
        }
    }
    return ret;
}

inline float Dissimilarity(const Bitmap &piece1, const Bitmap &piece2, const int &orientation)
{
    std::vector<unsigned char> side1 = piece1.GetSide(orientation);
    std::vector<unsigned char> side2 = piece2.GetSide(orientation ^ 1);
    assert(side1.size() == side2.size());
    int n = side1.size();
    float ret = 0, inv = 1. / 255;
    for (int i = 0; i < n; i++)
    {
        float delta = (float)((int)side1[i] - (int)side2[i]) * inv;
        ret += delta * delta;
    }
    ret = std::sqrt(ret);
    return ret;
}

#endif // BITMAP_H