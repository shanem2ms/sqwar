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

    class PtScorer;
    class PTCloudAlign;

    PtScorer* CreatePtScorer(float* m_pts0, size_t ptCount0, float* m_pts1, size_t ptCount1, float* pmatrix,
        int frameIdx);

    float GetScore(PtScorer* pthis, float* pmatrix);

    void FreePtScorer(PtScorer* pthis);

    PTCloudAlign* CreatePtCloudAlign(gmtl::Vec3f* pts0, size_t ptCount0, gmtl::Vec3f* pts1, size_t ptCount1);

    int AlignStep(PTCloudAlign* pthis, float* matrix);

    void FreePtCloudAlign(PTCloudAlign* pthis);

    void DepthBuildLods(float* dbuf, float* outpts, int depthWidth, int depthHeight);

    void ConvertDepthToYUV(float* data, int width, int height, float maxDepth, uint8_t* ydata, uint8_t* udata, uint8_t* vdata);

}