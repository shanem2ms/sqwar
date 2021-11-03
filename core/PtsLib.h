#pragma once

#include <vector>
#include <gmtl/gmtl.h>
#include <gmtl/Vec.h>
#include <gmtl/Matrix.h>
#include <gmtl/Point.h>
#include <gmtl/AABox.h>

typedef unsigned int uint32;
typedef unsigned int uint64;
typedef unsigned char uchar;
typedef unsigned char byte;

struct RGBt
{
    RGBt(byte _r, byte _g, byte _b) :
        R(_r),
        G(_g),
        B(_b) {}

    byte R;
    byte G;
    byte B;
};

struct TreePt
{
    TreePt(const RGBt &rgb, const gmtl::Vec3f &nrm, int ff) :
        color(rgb),
        normal(nrm),
        firstFrame(ff),
        nHits(0)
    {
    }

    void Aggregate(const TreePt& other)
    {
        firstFrame = std::min(firstFrame, other.firstFrame);
        lastFrame = std::max(firstFrame, other.firstFrame);
    }

    RGBt color;
    gmtl::Vec3f normal;
    int firstFrame;
    int lastFrame;
    int nHits;
};

struct WorldPt
{
    gmtl::Point3f pt;
    gmtl::Vec3f nrm;
    float size;
    RGBt color;
};

using namespace gmtl;

Matrix44f CameraMat(const Vec4f& cameraCalibrationVals,
    const Vec2f& cameraCalibrationDims,
    int dw, int dh);

inline bool IsValid(const Point3f& pt)
{
    return !isnan(pt[0]) && !isinf(pt[0]);
}

#define PTEXPORT extern "C" __declspec (dllexport)

void CalcNormals(Point3f* pts0, Vec3f* nrm, int dw, int dh);

void CalcDepthPts(const Matrix44f& camMatrix, float* depthVals, int width, int height, std::vector<Point3f> &pOutPts);
void DepthBuildLods(float* dbuf, float* outpts, int depthWidth, int depthHeight, float maxDist = -1);

inline void AAExpand(AABoxf& aabox, const Point3f& v)
{
    for (int i = 0; i < 3; ++i)
    {
        if (v[i] < aabox.mMin[i])
            aabox.mMin[i] = v[i];
        if (v[i] > aabox.mMax[i])
            aabox.mMax[i] = v[i];
    }
}
