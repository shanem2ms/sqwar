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
    void GetDepthPoints(const std::vector<float>& floatArray,
        std::vector<gmtl::Vec3f>& outPts, int depthWidth, int depthHeight);
}