#include "renderTriangle2.h"

#include "vec4.h"
#include "mat4.h"
#include "macros.h"
#include "renderTriangle.h"
#include "light.h"
#include "texture.h"
#include "color.h"
#include "frameBuffer.h"
#include "vec2.h"

__inline
void RenderTriangle2::setupForTriangle(const Vertex& v1, const Vertex& v2, const Vertex& v3)
{
    this->triangleVertices = {v1, v2, v3};
}

__inline
bool RenderTriangle2::isBackFaceCulled(const Vec3& cameraLocation)
{
    //is it really performant to compute normals for every triangle?
    //couldn't we do in another way, with already computed normals?

    //compute plane normal
    Vec3 N = crossProduct(Vec3((triangleVertices[1].position - triangleVertices[0].position)), 
                          Vec3((triangleVertices[2].position - triangleVertices[0].position)));

    worldVertices[0] = triangleVertices[0].position;
    worldVertices[1] = triangleVertices[1].position;
    worldVertices[2] = triangleVertices[2].position;

    return dotProduct(N, triangleVertices[0].position - cameraLocation) > 0;
}

__inline
void RenderTriangle2::setRelativeToCamera(const Mat4& transform)
{
    //TODO : optimization : Vec3 -> Vec4 -> Vec3
    triangleVertices[0].position = transform * Vec4(triangleVertices[0].position, 1);
    triangleVertices[1].position = transform * Vec4(triangleVertices[1].position, 1);
    triangleVertices[2].position = transform * Vec4(triangleVertices[2].position, 1);
}

__inline
bool RenderTriangle2::isClipped(const Texture& pTarget, 
                                std::vector<RenderTriangle2>& additionalTriangles)
{
    // RenderTriangle2 newTriangle;
    // newTriangle.triangleVertices = triangleVertices;
    // newTriangle.worldVertices = worldVertices;
    // for (unsigned int i = 0; i < 3; i++)
    // {
    //     newTriangle.triangleVertices[i].position.y += 1;
    //     newTriangle.worldVertices[i].y += 1;
    // }
    // additionalTriangles.emplace_back(newTriangle);



    // if (triangleVertices[0].position.x < 0)
    //     std::cout << "ERROR : Out of box\n";

    return triangleVertices[0].position.z >= 0 
        || triangleVertices[1].position.z >= 0 
        || triangleVertices[2].position.z >= 0;
    
    // TODO: add triangles with clipping
}

__inline
std::array<float, 3> RenderTriangle2::projectVertices(const Mat4& projection)
{
    std::array<float, 3> w;
    //TODO: opti possible 
    Vec4 projectedLoc = projection * triangleVertices[0].position;
    w[0] = projectedLoc.w;
    triangleVertices[0].position = projectedLoc.getHomogenizedVec();

    projectedLoc = projection * triangleVertices[1].position;
    w[1] = projectedLoc.w;
    triangleVertices[1].position = projectedLoc.getHomogenizedVec();

    projectedLoc = projection * triangleVertices[2].position;
    w[2] = projectedLoc.w;
    triangleVertices[2].position = projectedLoc.getHomogenizedVec();

    return w;
}

__inline
void RenderTriangle2::setVerticesToScreenResolution(const Mat4& screenConversion)
{
    triangleVertices[0].position = screenConversion * Vec4(triangleVertices[0].position, 1);
    triangleVertices[1].position = screenConversion * Vec4(triangleVertices[1].position, 1);
    triangleVertices[2].position = screenConversion * Vec4(triangleVertices[2].position, 1);
    // if ((screenConversion * Vec4(v1.position, 1)).w != 1)
    // std::cout << (screenConversion * Vec4(v1.position, 1)).w << '\n';
}

__inline
void RenderTriangle2::addTransparency(const float transparency)
{
    triangleVertices[0].color.a *= transparency;
    triangleVertices[1].color.a *= transparency;
    triangleVertices[2].color.a *= transparency;
}

__inline
void RenderTriangle2::setDefaultColor()
{
    triangleVertices[0].color = Color(1, 0, 0);
    triangleVertices[1].color = Color(0, 1, 0);
    triangleVertices[2].color = Color(0, 0, 1);
}


__inline
void RenderTriangle2::drawLineX(FrameBuffer* pTarget, const Vertex& vertex1, const Vertex& vertex2)
{
    //get distance between two points
    const int& x1 = vertex1.position.x;
    const int& y1 = vertex1.position.y;
    const int& x2 = vertex2.position.x;
    const int& y2 = vertex2.position.y;
    
    Vec2 vec2(x2-x1, y2-y1);
    //get distance between the 2 points
    float magnitude = vec2.GetMagnitude();

    if (magnitude != 0.f)
    {
        vec2.toUnit();

        for (float i = 0; i <= magnitude; ++i)
        {
            float ratio = i / magnitude;
            Vec3 point(x1 + i * vec2.x, 
                       y1 + i * vec2.y, 
                       vertex1.position.z * ratio + vertex2.position.z * (1 - ratio));

            Color c(1,1,1);

            if (point.x >= 0 && point.x < pTarget->width && point.y >= 0 && point.y < pTarget->height)
                pTarget->SetPixel(point.x, point.y, point.z, c);
        }
    }
    else 
        if (x2 >= 0 && x2 < int(pTarget->width) && y2 >= 0 && y2 < int(pTarget->height))
            pTarget->SetPixel(x2, y2, vertex2.position.z, vertex1.color);
}

__inline 
void RenderTriangle2::drawWireframe(FrameBuffer* pTarget)
{
    drawLineX(pTarget, triangleVertices[0], triangleVertices[1]);
    drawLineX(pTarget, triangleVertices[1], triangleVertices[2]);
    drawLineX(pTarget, triangleVertices[2], triangleVertices[0]);
}

struct weightPrecomputedData
{
    inline
    weightPrecomputedData(const float p2p3y, const float p1p3y, const float p3p2x, const float p1p3x) 
        : p2p3y(p2p3y), p1p3y(p1p3y), p3p2x(p3p2x), p1p3x(p1p3x), det(p2p3y * p1p3x + p3p2x * p1p3y)
        {}

    float p2p3y; // p2.y - p3.y
    float p1p3y; // p3.y - p1.y
    float p3p2x; // p3.x - p2.x
    float p1p3x; // p1.x - p3.x

    float det;
};

inline
void getWeight(const Vec2& p, const Vec3& p1, const Vec3& p2, const Vec3& p3, 
                std::array<float, 3>& weight)
{
    float det = (p2.y - p3.y) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.y - p3.y);
    if (det == 0)
    {
        weight[0] = 0;
        weight[1] = 0;
        weight[2] = 0;
    }

    weight[0] = ((p2.y - p3.y) * (p.x - p3.x) + (p3.x - p2.x) * (p.y - p3.y)) / det;
    weight[1] = ((p3.y - p1.y) * (p.x - p3.x) + (p1.x - p3.x) * (p.y - p3.y)) / det;
    weight[2] = 1 - weight[1] - weight[0];
}

//12 operations
inline
void getWeight(const float pxp3x, const float pyp3y, 
                std::array<float, 3>& weight, 
                const weightPrecomputedData& precomputed)
{
    // float det = (p2.y - p3.y) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.y - p3.y);
    weight[0] = (precomputed.p2p3y * pxp3x + precomputed.p3p2x * pyp3y) / precomputed.det;
    weight[1] = (precomputed.p1p3x * pyp3y - precomputed.p1p3y * pxp3x) / precomputed.det;
    weight[2] = 1 - weight[1] - weight[0];
}

static bool tryToDrawPixel(unsigned int x, unsigned int y, float depth, 
                           FrameBuffer* pTarget, std::function<Color(void)>& getColor)
{   
    //BECAREFUL
    float currentDepth = pTarget->depthBuffer.getDepth(x, y);

    if (currentDepth <= depth)
        return false;

    pTarget->SetPixel(x, y, depth, getColor());

    return true;
}

// BECAREFUL
// loop could be unrolled without this function
template<typename T, typename U, long unsigned int N>
U operator*(const std::array<T, N>& lhs, const std::array<U, N>& rhs)
{
    U result = 0.f;
    for (unsigned int i = 0; i < N; ++i)
    {
        result += lhs[i] * rhs[i];
    }
    return result;
}

__inline
void getTextureColor(Texture const * const pTexture, 
                     std::array<Vertex, 3>& triangleVertices, 
                     std::array<float, 3>& weight, 
                     std::array<float, 3>& uP, 
                     std::array<float, 3>& vP, 
                     Color& c)
{
    if (pTexture != nullptr && pTexture->pixels != nullptr)
    {
        // could be unrolled
        // for (unsigned int i = 0; i < 3; i++)
        // {
        //     u += weight[i] * uP[i];
        //     v += weight[i] * vP[i];
        // }
        float u = weight * uP;
        float v = weight * vP;

        //if not in range, due to float imprecision
        // u = clamp(u, 0.f, 1.f);
        // v = clamp(v, 0.f, 1.f);
        u = fmod(u, 1);
        if (u < 0)
            u += 1;
        v = fmod(v, 1);
        if (v < 0)
            v += 1;
        
        //TODO : enum for interpolation type
        #ifdef __NEAREST_INTERPOLATION__
        {
            getTexturedColorNearestInterpolation(pTexture, 
                                                u, v, c);
        }
        #endif

        #ifdef __BILINEAR_INTERPOLATION__
        {
            getTexturedColorBilinearInterpolation(pTexture, 
                                                u, v, c);
        }
        #endif

        c.a += weight[0] * triangleVertices[0].color.a
            + weight[1] * triangleVertices[1].color.a
            + weight[2] * triangleVertices[2].color.a;
    }
    #if defined(__NEAREST_INTERPOLATION__) || defined(__BILINEAR_INTERPOLATION__)
    else 
    #endif
        getUntexturedPixelColor(triangleVertices, weight, c);
}
// extern unsigned int GLOBAL_INDEX;
__inline
void RenderTriangle2::drawTriangleX(FrameBuffer* pTarget, std::array<float, 3>& ww, 
                                    const Vec3& cameraLocation, std::vector<Light>& lights, 
                                    Texture* pTexture, const Material& mat)
{
    // std::array<Vertex*, 3> triangleVertices;
    // triangleVertices[0] = &v1;
    // triangleVertices[1] = &v2;
    // triangleVertices[2] = &v3;

    #ifdef __PERSPECTIVE_FIX__
    // TODO: assert?
    if (ww[0] == 0 || ww[1] == 0 || ww[2] == 0)
        return;

    #endif

    const Vec3& p1 = triangleVertices[0].position;
    const Vec3& p2 = triangleVertices[1].position;
    const Vec3& p3 = triangleVertices[2].position;

    //BECAREFUL
    //clipping in rasterization
    #ifdef __ADD_OFFSET__
    unsigned int minX = std::min(std::max(windowRenderMinX, std::min(std::min(p1.x, p2.x), p3.x)), windowRenderMaxX);
    unsigned int maxX = std::min(std::max(windowRenderMinX, std::max(std::max(p1.x, p2.x), p3.x)), windowRenderMaxX);
    unsigned int minY = std::min(std::max(windowRenderMinY, std::min(std::min(p1.y, p2.y), p3.y)), windowRenderMaxY);
    unsigned int maxY = std::min(std::max(windowRenderMinY, std::max(std::max(p1.y, p2.y), p3.y)), windowRenderMaxY);
    #else
    unsigned int minX = std::min(std::max(0.f, std::min(std::min(p1.x, p2.x), p3.x)), float(pTarget->width));
    unsigned int maxX = std::min(std::max(0.f, std::max(std::max(p1.x, p2.x), p3.x)), float(pTarget->width));
    unsigned int minY = std::min(std::max(0.f, std::min(std::min(p1.y, p2.y), p3.y)), float(pTarget->height));
    unsigned int maxY = std::min(std::max(0.f, std::max(std::max(p1.y, p2.y), p3.y)), float(pTarget->height));
    #endif

    std::array<float, 3> uP = {triangleVertices[0].u, triangleVertices[1].u, triangleVertices[2].u};
    std::array<float, 3> vP = {1-triangleVertices[0].v, 1-triangleVertices[1].v, 1-triangleVertices[2].v};

    std::array<float, 3> weight;

    std::function<Color(void)> getColor;

    #ifdef __MULTI_SAMPLING_LIGHT__
    bool bComputed = false;
    float currentLuminosity = 0.f;

    std::function<float(const Vec3&, 
                        const Vec3&, 
                        const std::vector<Light>&, 
                        const Vec3&, 
                        const Material&)> 
        getLight = [&bComputed, &currentLuminosity](const Vec3& location3D, 
                                                    const Vec3& normal, 
                                                    const std::vector<Light>& lights, 
                                                    const Vec3& cameraLocation, const Material& mat)
    {
        if (!bComputed)
        {        
            bComputed = true;
            currentLuminosity = getPixelLight(location3D, normal, lights, cameraLocation, mat);
        }
        return currentLuminosity;
    };
    #endif

    #ifdef __BLINN_PHONG_LIGHTING__
    // using blinn phong
    getColor = [&](void)
    {
        Vec3 location3D = weight * worldVertices;

        // // could be unrolled
        // for (unsigned int i = 0; i < 3; i++)
        // {
        //     location3D.x += weight[i] * worldVertices[i].x;
        //     location3D.y += weight[i] * worldVertices[i].y;
        //     location3D.z += weight[i] * worldVertices[i].z;
        // }

        //compute z
        // float z = (triangleVertices[0].position.z) * weight[0] 
        //         + triangleVertices[1].position.z * weight[1] 
        //         + triangleVertices[2].position.z * weight[2];

        Vec3 normal(0, 0, 0);
        // could be unrolled
        for (unsigned int i = 0; i < 3; i++)
        {
            normal += weight[i] * triangleVertices[i].normal;
        }

        #ifdef __PERSPECTIVE_FIX__
        {
            weight[0] /= ww[0];
            weight[1] /= ww[1];
            weight[2] /= ww[2];

            float total = weight[0] + weight[1] + weight[2];
            if (total == 0)
                return Color(0, 0, 0, 0);
                
            weight[0] = weight[0] / total;
            weight[1] = weight[1] / total;
            weight[2] = weight[2] / total;
        }
        #endif

        //RasterizingVertex vert;
        Color c(0, 0, 0, 0);
        
        // lighting
        float intensity;

        normal.Normalize();

        #ifdef __MULTI_SAMPLING_LIGHT__
        intensity = getLight(location3D, normal, lights, cameraLocation, mat);
        #else
        // BLINN PHONG
        intensity = getPixelLight(location3D, normal, lights, cameraLocation, mat);
        #endif

        getTextureColor(pTexture, 
                        triangleVertices, 
                        weight, 
                        uP, 
                        vP, 
                        c);

        #ifdef __ADDITIONAL_SHADERS__
        Vec3 newLoc = weight * worldVertices;
        mat.additionalShaders(c, newLoc);
        #endif

        c.r *= intensity;
        c.g *= intensity;
        c.b *= intensity;

        return c;
    };

    #endif

    #if defined(__PHONG_LIGHTING__)

    std::array<float, 3> intensity;
    for (unsigned int i = 0; i < 3; ++i)
    {
        intensity[i] = getPixelLight(worldVertices[i], triangleVertices[i].normal, lights, cameraLocation, mat);
        // GLOBAL_INDEX++;
        // if (GLOBAL_INDEX > 50)
        // {
        //     std::cout << worldVertices[i] << '\n';
        //     std::cout << triangleVertices[i].normal << '\n';
        //     std::cout << intensity[i] << '\n';
        //     GLOBAL_INDEX = 0;
        // }
    }

    // std::array<Color, 3> finalColor; // of the 3 vertices

    // using phong
    getColor = [&](void)
    {
        Color c(0, 0, 0, 0);

        // for (unsigned int i = 0; i < 4; ++i)
        //     c += finalColor[i] * weight[i];

        #ifdef __PERSPECTIVE_FIX__
        {
            weight[0] /= ww[0];
            weight[1] /= ww[1];
            weight[2] /= ww[2];

            float total = weight[0] + weight[1] + weight[2];
            if (total == 0)
                return Color(0, 0, 0, 0);
                
            weight[0] = weight[0] / total;
            weight[1] = weight[1] / total;
            weight[2] = weight[2] / total;
        }
        #endif

        getTextureColor(pTexture, 
                        triangleVertices, 
                        weight, 
                        uP, 
                        vP, 
                        c);

        // float finalIntensity = intensity * weight;

        //intensity[2] = 0.5f;

        float finalIntensity = 0.f;
        for (unsigned int i = 0; i < 3; ++i)
        {
            finalIntensity += intensity[i] * weight[i];
        }
        //std::cout << intensity[2] << " / " << weight[2] << '\n';

        if (finalIntensity > 1)
            finalIntensity = 1;

        else if (finalIntensity < 0)
            finalIntensity = 0;

        #ifdef __ADDITIONAL_SHADERS__
        Vec3 newLoc = weight * worldVertices;
        mat.additionalShaders(c, newLoc);
        #endif

        c.r *= finalIntensity;
        c.g *= finalIntensity;
        c.b *= finalIntensity;

        return c;
    };
    #endif

    #ifdef __MULTI_SAMPLING_TRIANGLES__
    // every pixel respecting (x % 2 == 0 && y % 2 == 0)) 
    // construct the base texture.
    minX -= minX % 2;
    minY -= minY % 2;
    maxX -= maxX % 2;
    maxY -= maxY % 2;
    #endif

    const weightPrecomputedData weightData (p2.y - p3.y, 
                                            p1.y - p3.y,
                                            p3.x - p2.x,
                                            p1.x - p3.x);

    //TODO : set WeightVar outside of the loops
    #if defined(__MULTI_SAMPLING_TRIANGLES__) || defined(__MULTI_SAMPLING_LIGHT__)
    for (unsigned int y = minY; y <= maxY; y += antiAliasingY)
    {
        for (unsigned int x = minX; x <= maxX; x += antiAliasingX)
        {
    #else 
    for (unsigned int y = minY; y <= maxY; y ++)
    {
        for (unsigned int x = minX; x <= maxX; x ++)
        {
    #endif
            // tryToDrawPixel(x, y, 
            //                 // p1, p2, p3, 
            //                 triangleVertices,
            //                 ww, uP, vP, cameraLocation, 
            //                 worldVertices, pTarget, lights, pTexture, mat);

            // bool isValid = getWeight(Vec2(x,y), triangleVertices[0].position, 
            //                                     triangleVertices[1].position, 
            //                                     triangleVertices[2].position, weight);

            #ifdef __MULTI_SAMPLING_LIGHT__
            bComputed = false;
            for (unsigned int yAliasing = 0; yAliasing < antiAliasingY; yAliasing ++)
            {
                for (unsigned int xAliasing = 0; xAliasing < antiAliasingX; xAliasing ++)
                {
                    getWeight(x + xAliasing - triangleVertices[2].position.x, 
                              y + yAliasing - triangleVertices[2].position.y, 
                              weight, weightData);

                    if (weight[0] >= 0 && weight[1] >= 0 && weight[2] >= 0)
                    {
                        const float depth = (triangleVertices[0].position.z) * weight[0] 
                                        + (triangleVertices[1].position.z) * weight[1] 
                                        + (triangleVertices[2].position.z) * weight[2];

                        tryToDrawPixel(x + xAliasing, y + + yAliasing, depth, pTarget, getColor);
                    }
                }
            }
            
            #else

            getWeight(x - triangleVertices[2].position.x, y - triangleVertices[2].position.y, weight, weightData);

            if (weight[0] >= 0 && weight[1] >= 0 && weight[2] >= 0)
            {
                const float depth = (triangleVertices[0].position.z) * weight[0] 
                                  + (triangleVertices[1].position.z) * weight[1] 
                                  + (triangleVertices[2].position.z) * weight[2];

                tryToDrawPixel(x, y, depth, pTarget, getColor);
            }

            #endif
        }
    }
}



__inline
void getUntexturedPixelColor(const std::array<Vertex, 3>& triangleVertices, 
                             const std::array<float, 3>& weight, 
                             Color& colorOutput)
{
    // Could be unrolled for optimization
    // interpolate 
    for (unsigned int i = 0; i < 3; i++)
    {
        colorOutput.r += weight[i] * triangleVertices[i].color.r;
        colorOutput.g += weight[i] * triangleVertices[i].color.g;
        colorOutput.b += weight[i] * triangleVertices[i].color.b;
        colorOutput.a += weight[i] * triangleVertices[i].color.a;
    }
}

__inline
void getTexturedColorNearestInterpolation(const Texture* texture, 
                                          const float u,
                                          const float v, 
                                          Color& colorOutput)
{
    // u and v represents a pixel location in the  texture, and them must be between 0 and 1.
    assert(0 <= u && u <= 1 && 0 <= v && v <= 1);
 
    // Max uv is 1, 1 * width = width, width isn't a valid index, 
    // so we substract by 1 before the multiplication.
    // since (u, v) >= 0, we can cast them to unsigned int without errors.
    colorOutput.copyRGB(texture->GetPixelColor(static_cast<unsigned int>(u * (float(texture->width))), 
                                               static_cast<unsigned int>(v * (float(texture->height)))));
}

__inline
void getTexturedColorBilinearInterpolation(const Texture* texture, 
                                           const float u,
                                           const float v, 
                                           Color& colorOutput)
{
    // floor
    // x1,y1--------x2,y1
    //   |            |
    //   |            |
    //   |  x,y       |
    //   |   #        |
    //   |            |
    // x1,y2--------x2,y2
    //               ceil

    // u and v represents a pixel location in the  texture, and them must be between 0 and 1.
    assert(0 <= u && u <= 1 && 0 <= v && v <= 1);

    const unsigned int x1 = static_cast<unsigned int>(floor(u * (float(texture->width)-1)));
    const unsigned int x2 = std::min(static_cast<unsigned int>(ceil(u * (float(texture->width)-1))), texture->width - 1);

    const unsigned int y1 = static_cast<unsigned int>(floor(v * (float(texture->height)-1)));
    const unsigned int y2 = std::min(static_cast<unsigned int>(ceil(v * (float(texture->height)-1))), texture->height - 1);

    const float curX = u * (float(texture->width) - 1);
    const float curY = v * (float(texture->height) - 1);

    //can't interpolate in case interpolating points are the same
    if (x2 == x1 && y2 == y1)
    {
        colorOutput.reset();
        return;
    }
    //in case the two interpolating points are on the same horizontal line
    else if (x2 == x1)
    {
        const Color c1 = texture->GetPixelColor(x1, y1);
        const Color c2 = texture->GetPixelColor(x1, y2);

        colorOutput.copyRGB(getAverageColor(c2, c1, (curY - y1) / (y2 - y1)));
    }
    //in case the two interpolating points are on the same vertical line
    else if (y2 == y1)
    {
        const Color c1 = texture->GetPixelColor(x1, y1);
        const Color c2 = texture->GetPixelColor(x2, y1);

        colorOutput.copyRGB(getAverageColor(c2, c1, (curX - x1) / (x2 - x1)));
    }
    //general case : 
    else 
    {
        const Color c11 = texture->GetPixelColor(x1, y1);
        const Color c21 = texture->GetPixelColor(x2, y1);
        const Color c12 = texture->GetPixelColor(x1, y2);
        const Color c22 = texture->GetPixelColor(x2, y2);

        const Color c1 = getAverageColor(c21, c11, (curX - x1 / (x2 - x1)));
        const Color c2 = getAverageColor(c22, c12, (curX - x1 / (x2 - x1)));

        colorOutput.copyRGB(getAverageColor(c2, c1, (curY - y1) / (y2 - y1)));
    }
}

__inline
float getPixelLight(const Vec3& location3D, 
                                    const Vec3& normal, 
                                    const std::vector<Light>& lights, 
                                    const Vec3& cameraLocation, const Material& mat)
{
    float ambient  = 0.f;
    float diffuse  = 0.f;
    float specular = 0.f;  
    float total    = 0.f;

    for (const Light& light : lights)
    {
        //ambient
        ambient = light.ambientComponent * mat.ambient;

        //diffuse
        Vec3 pixelToLightVec = (light.position - location3D);
        pixelToLightVec.Normalize();
        
        float cosTeta = std::max(0.f, dotProduct(pixelToLightVec, normal));

        diffuse = light.diffuseComponent * mat.diffuse * cosTeta;

        //specular
        Vec3 pixelToEyeVec = cameraLocation - location3D;
        // pixelToEyeVec.z -= 1;
        pixelToEyeVec.Normalize();
        Vec3 h = pixelToLightVec + pixelToEyeVec;
        h.Normalize();
        float cosAlpha = dotProduct(normal.getNormalized(), h);

        if (cosAlpha < 0)
            cosAlpha = 0;

        specular = light.specularComponent * mat.specular * std::pow(cosAlpha, mat.brillance);

        total += ambient + diffuse + specular;
    }

    return total;
}