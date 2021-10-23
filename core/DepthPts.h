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
        const unsigned char* vdata,
        std::vector<gmtl::Vec4f>& outPts, int depthWidth, int depthHeight);
}