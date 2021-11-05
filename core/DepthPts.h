#pragma once

#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <gmtl/gmtl.h>
#include <gmtl/Matrix.h>
#include <gmtl/Point.h>
#include <gmtl/Quat.h>
#include <gmtl/Vec.h>

namespace sam
{
    void GetDepthPointsWithColor(const std::vector<float>& floatArray,
        const unsigned char* vdata, int vWidth, int vHeight,
        std::vector<gmtl::Vec4f>& outPts, int depthWidth, int depthHeight, float maxdist);

    void GetDepthPoints(const std::vector<float>& floatArray,
        std::vector<gmtl::Point3f>& outPts, int depthWidth, int depthHeight, float maxdist);

    void DepthMakePlanes(const gmtl::Vec4f* vals, int depthWidth, int depthHeight,
        std::vector<gmtl::Vec3f>& outVertices, std::vector<gmtl::Vec3f>& outTexCoords);
}