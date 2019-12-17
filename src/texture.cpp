#include <cassert>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>

// #define STB_IMAGE_IMPLEMENTATION
// #include "stb_image.h"

#include "texture.h"

#include "color.h"

Uint32 getpixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        return *p;

    case 2:
        return *(Uint16 *)p;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;

    case 4:
        return *(Uint32 *)p;

    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
}

Texture::Texture(const Texture& rhs)
    : width(rhs.width), height(rhs.height), pixels(new Color[width * height])
{
    pixels = new Color[width * height];
    for (unsigned int i = 0; i < width * height; i++)
    {
        pixels[i] = rhs.pixels[i];
    }
}

Texture::Texture(Texture&& rhs) noexcept
    : width(std::move(rhs.width)),
      height(std::move(rhs.height)),
      pixels(std::move(rhs.pixels))
{
    rhs.pixels = nullptr;
}

Texture::Texture(const char* filename)
    : width(0),
      height(0),
      pixels() //default constructor init pixels to black
{
    SDL_Surface* surface = IMG_Load(filename);

    if (surface != nullptr)
    {
        width = static_cast<unsigned int>(surface->w);
        height = static_cast<unsigned int>(surface->h);

        pixels = new Color[width * height];

        for (unsigned int y = 0; y < height; y++)
        { 
            for (unsigned int x = 0; x < width; x++)
            { 
                Uint32 color = getpixel(surface, x, y);
                unsigned char* colors = (unsigned char*) &color;
                pixels[y * width + x].colors[0] = static_cast<float> (colors[0]) / 255.f;
                pixels[y * width + x].colors[1] = static_cast<float> (colors[1]) / 255.f;
                pixels[y * width + x].colors[2] = static_cast<float> (colors[2]) / 255.f;
                pixels[y * width + x].colors[3] = static_cast<float> (colors[3]) / 255.f;
            }
        }

        SDL_FreeSurface(surface);

        // for (int i = 0; i < width * height; i++)
        // {
        //     //pixel index
        //     for (int j = 0; j < 4; j++)
        //     {
        //         //pixel color
        //         pixels[i][j] = getpixel(surface, i, j);
        //     }
        //     // pixels[i].r = data[i];
        //     // pixels[i].g = data[i + 1];
        //     // pixels[i].b = data[i + 2];
        // }

        // int x, y, n;
        // // unsigned char* data = stbi_load(filename, &x, &y, &n, STBI_rgb_alpha);
        // unsigned char* data = stbi_load(filename, &x, &y, &n, STBI_rgb);

        // if (data != NULL)
        // {
        //     width = static_cast<unsigned int>(x);
        //     height = static_cast<unsigned int>(y);

        //     pixels = new Color[x * y];

        //     for (int i = 0; i < x * y; i++)
        //     {
        //         //pixel index
        //         for (int j = 0; j < n; j++)
        //         {
        //             //pixel color
        //             pixels[i][j] = data[i + j];
        //         }
        //         // pixels[i].r = data[i];
        //         // pixels[i].g = data[i + 1];
        //         // pixels[i].b = data[i + 2];
        //     }
        // }

        // setDegradee();
    }
}

void Texture::setDegradee()
{
    for (unsigned int i = 0; i < height; i++)
    {
        for (unsigned int j = 0; j < width; j++)
        {
            pixels[j + i * width].r = 255.f / height * i;
            pixels[j + i * width].g = 0;//255.f / width * j;
            pixels[j + i * width].b = 0;
            pixels[j + i * width].a = 255;
        }
    }
}

Texture::Texture(unsigned int width, unsigned int height) 
    : width(width),
      height(height),
      pixels(new Color[width * height]) //default constructor init pixels to black
{
    
}

Texture::~Texture()
{
    if (pixels != nullptr)
        delete[] pixels;
}

void Texture::SetPixelColor(unsigned int x, unsigned int y, const Color& c)
{
    //c.clamp();
    if (x > 0 && x < width && y > 0 && y < height)
        pixels[x + y * width] = c;
}

// Color Texture::GetPixelColor(unsigned int x, unsigned int y) const
// {
//     // if (!(0 <= x && x < width && 0 <= y && y < height))
//     // {
//     //     std::cout << "y" << std::endl;
//     // }

//     assert(0 <= x && x < width && 0 <= y && y < height);
//     return pixels[x + y * width];   
// }

void Texture::FillBlack()
{
    // 4 * sizeof(unsigned char)
    //memset(pixels, 0, 4 * sizeof(unsigned char) * width * height);
    //#pragma omp parallel for
    for (unsigned int i = 0; i < width * height; i++)
    {
        // pixels[i].r = 0;
        // pixels[i].g = 0;
        // pixels[i].b = 0;
        // pixels[i].a = 255;
        pixels[i] = {0, 0, 0, 1};
    }
}

void Texture::ToTexture(SDL_Texture* sdlTexture) const
{
    unsigned char pix[width * height * 4];
    for (unsigned int i = 0; i < width * height * 4; i += 4)
    {
        //std::cout << this->pixels[i].r << std::endl;
        pix[i]   =  this->pixels[i].r*255;
        pix[i+1] =  this->pixels[i].g*255;
        pix[i+2] =  this->pixels[i].b*255;
        pix[i+3] =  255;
    }
    SDL_UpdateTexture(sdlTexture, nullptr, pix, sizeof(unsigned char) * width); 
}