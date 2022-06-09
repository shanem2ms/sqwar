#pragma once
#include "gmtl/Vec.h"
#include "gmtl/Matrix.h"

namespace sam
{
    struct DepthDataProps
    {
        double timestamp;
        uint32_t vidWidth;
        uint32_t vidHeight;
        uint32_t vidMode;
        uint32_t depthWidth;
        uint32_t depthHeight;
        uint32_t depthMode;
    };

    struct DepthData
    {
        DepthDataProps props;
        std::vector<unsigned char> vidData;
        std::vector<float> depthData;
        std::vector<gmtl::Vec4f> pts;
        gmtl::Matrix44f alignMtx;
    };

    struct FaceDataProps
    {
        double timestamp;
        float viewMatf[16];
        float wMatf[16];
        float projMatf[16];
    };

    struct YCrCbData
    {
        int yOffset;
        int yRowBytes;
        int cbCrOffset;
        int cbCrRowBytes;
    };

}