#include <cmath>
#include "maths/utilityFunctions.h"

#include "renderTriangle.h"

#include "rasterizer.h"
#include "frameBuffer.h"
#include "vertex.h"
#include "rasterizingVertex.h"
#include "color.h"
#include "light.h"
#include "material.h"

constexpr int deltaPixelsX = 5; 
constexpr int deltaPixelsY = 1; 

RenderTriangle::RenderTriangle(const std::array<RasterizingVertex, 3>& triangleVertices, FrameBuffer* pTarget, std::vector<Light>& lights)
    : triangleVertices({&triangleVertices[0], &triangleVertices[1], &triangleVertices[2]}),
      pTarget(pTarget),
      lights(lights)
{

}



////////Get weight of a point in a triangle//////////
bool RenderTriangle::getWeight(const Vec2& p, float* weight) const
{
    const Vec2& p1 = triangleVertices[0]->position2D;
    const Vec2& p2 = triangleVertices[1]->position2D;
    const Vec2& p3 = triangleVertices[2]->position2D;

    //float det = (p2.y - p3.y) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.y - p3.y);
    if (det == 0)
    {
        weight[0] = 0;
        weight[1] = 0;
        weight[2] = 0;
        return false;
    }

    // weight[0] = ((p2.y - p3.y) * (p.x - p3.x) + (p3.x - p2.x) * (p.y - p3.y)) / det;
    // weight[1] = ((p3.y - p1.y) * (p.x - p3.x) + (p1.x - p3.x) * (p.y - p3.y)) / det;
    // weight[2] = 1 - weight[1] - weight[0];

    weight[0] = (cst[0] * (p.x - p3.x) + cst[1] * (p.y - p3.y)) / det;
    weight[1] = (cst[2] * (p.x - p3.x) + cst[3] * (p.y - p3.y)) / det;
    weight[2] = 1 - weight[1] - weight[0];
    
    return weight[0] >= 0 && weight[1] >= 0 && weight[2] >= 0;
}

float RenderTriangle::getPixelLight(const RasterizingVertex& vertex) const
{
    Material mat;

    float ambient  = 0.f;
    float diffuse  = 0.f;
    float specular = 0.f;

    for (const Light& light : lights)
    {
        ambient = light.ambientComponent * mat.ambient;

        Vec3 pixelLightDist = (light.position-vertex.position3D);
        pixelLightDist.Normalize();
        diffuse = light.diffuseComponent * mat.diffuse * ((dotProduct(pixelLightDist,(vertex.normal))));

        Vec3 eyeToPixelVec = vertex.position3D; //TODO
        eyeToPixelVec.z -= 1;
        Vec3 lightReflection = 2 * dotProduct(vertex.normal, pixelLightDist) * vertex.normal - pixelLightDist;
        specular = light.specularComponent * mat.specular * std::pow(dotProduct(lightReflection, eyeToPixelVec), mat.brillance);
    }
    
    float total = ambient + diffuse + specular;

    return total;
}

void RenderTriangle::draw() 
{
    const Vec3& v1 = triangleVertices[0]->position2D;//Rasterizer::projection(triangleVertices[0]->position);
    const Vec3& v2 = triangleVertices[1]->position2D;
    const Vec3& v3 = triangleVertices[2]->position2D;

    //getWeight constants
    det = (v2.y - v3.y) * (v1.x - v3.x) + (v3.x - v2.x) * (v1.y - v3.y);
    cst[0] = (v2.y - v3.y);
    cst[1] = (v3.x - v2.x);
    cst[2] = (v3.y - v1.y);
    cst[3] = (v1.x - v3.x);




    int minX = std::min(std::min(v1.x, v2.x), v3.x);
    int maxX = std::max(std::max(v1.x, v2.x), v3.x);
    int minY = std::min(std::min(v1.y, v2.y), v3.y);
    int maxY = std::max(std::max(v1.y, v2.y), v3.y);

    bool isInside = false;
    bool isValid = false;
    float weight[3];

    float intensityVertex[3];
    intensityVertex[0] = getPixelLight(*triangleVertices[0]);
    intensityVertex[1] = getPixelLight(*triangleVertices[1]);
    intensityVertex[2] = getPixelLight(*triangleVertices[2]);


    //TODO : set WeightVar outside of the loops
    for (int y = minY; y <= maxY; y++)
    {
        isInside = false;
        for (int x = minX; x <= maxX; x++)
        {
            isValid = getWeight(Vec2(x,y), weight);

            if (isValid)
            {
                Color c(0,0,0);

                Vec3 p(0, 0, 0); //3D point
                //Vec3 n(1, 0, 0); //normal

                    // p.x = triangleVertices[2]->position3D.x;
                    // p.y = triangleVertices[2]->position3D.y;
                    // p.z = triangleVertices[2]->position3D.z;

                float intensity = 0.f;

                //get color
                for (unsigned int i = 0; i < 3; i++)
                {
                    c.r += weight[i] * triangleVertices[i]->color.r;
                    c.g += weight[i] * triangleVertices[i]->color.g;
                    c.b += weight[i] * triangleVertices[i]->color.b;


                     intensity += weight[i] * intensityVertex[i];
                    p.x += weight[i] * triangleVertices[i]->position3D.x;
                    p.y += weight[i] * triangleVertices[i]->position3D.y;
                    p.z += weight[i] * triangleVertices[i]->position3D.z;

                    // n.x += weight[i] * triangleVertices[i]->normal.x;
                    // n.y += weight[i] * triangleVertices[i]->normal.y;
                    // n.z += weight[i] * triangleVertices[i]->normal.z;
                }
                // RasterizingVertex vert;
                // vert.position3D = p;
                // intensity = getPixelLight(vert);

                if (intensity < 0)
                    intensity = 0;

                //since -1 < intensity < 1, we need to lerp the intensity for the color
                if (c.r > 255.f / intensity)
                    c.r = 255;
                else 
                    c.r *= intensity;
                    
                if (c.g > 255.f / intensity)
                    c.g = 255;
                else 
                    c.g *= intensity;

                if (c.b > 255.f / intensity)
                    c.b = 255;       
                else 
                    c.b *= intensity;    
                //std::cout << n << '\n';
                // Vec3 face1 = triangleVertices[1]->position3D - triangleVertices[2]->position3D;
                // Vec3 face2 = triangleVertices[2]->position3D - triangleVertices[3]->position3D;
                // n = crossProduct(face1, face2);
                // if (n.GetMagnitude() <= 0)
                // {
                //     continue;
                // }
                //n = triangleVertices[0]->normal + triangleVertices[1]->normal + triangleVertices[2]->normal;
                //std::cout << n << '\n';
                // n = triangleVertices[0]->normal;
                //n.Normalize();

                //n = triangleVertices[0]->normal;

                // n.x = 0;
                // n.y = 0;
                // n.z = 0;
                // n.x = triangleVertices[1]->normal.y * triangleVertices[2]->normal.z - triangleVertices[1]->normal.y * triangleVertices[2]->normal.z;
                // n.y = triangleVertices[1]->normal.y * triangleVertices[2]->normal.z - triangleVertices[1]->normal.y * triangleVertices[2]->normal.z;
                // n.z = triangleVertices[1]->normal.y * triangleVertices[2]->normal.z - triangleVertices[1]->normal.y * triangleVertices[2]->normal.z;
                // n.Normalize();
                
                //calcul z
                float z = (v1.z) * weight[0] + (v2.z) * weight[1] + (v3.z) * weight[2];

                pTarget->SetPixel(x, y, z, c);

                isInside = true;
                isValid = false;
            }
            else 
            {
                if (isInside)
                    break;
            }
        }
    }
}