#include "StdIncludes.h"
#include "DepthPts.h"
#include <gmtl/AABox.h>
#include <gmtl/AABoxOps.h>
#include <gmtl/Plane.h>
#include <gmtl/PlaneOps.h>
#include <gmtl/Frustum.h>
#include <functional>

using namespace gmtl;
namespace sam
{

    void GetDepthPointsWithColor(const std::vector<float>& floatArray,
        const unsigned char* vdata,
        std::vector<Vec4f>& outPts, int depthWidth, int depthHeight, float maxdist)
    {
        float ratioX = floatArray[9] / depthWidth;
        float ratioY = floatArray[10] / depthHeight;
        float xScl = floatArray[0] / ratioX;
        float yScl = floatArray[4] / ratioY;
        float xOff = floatArray[6] / ratioX + 30;
        float yOff = floatArray[7] / ratioY;

        
        Quatf q = make<gmtl::Quatf>(AxisAnglef(pi, 0.0f, 1.0f, 0.0f)) *
            make<gmtl::Quatf>(AxisAnglef(-pi * 0.5f, 0.0f, 0.0f, 1.0f));
        Matrix44f matTransform = makeRot<Matrix44f>(q);

        int dOffsetX = 0;
        int dOffsetY = 0;
        float dSclX = 1.0f;
        float dSclY = 1.0f;

        std::vector<Vec4f>& pos = outPts;
        const float* depthArray = floatArray.data() + 16;

        for (int y = 0; y < depthHeight; ++y)
        {
            for (int x = 0; x < depthWidth; ++x)
            {
                float depthVal = depthArray[y * depthWidth + x];
                unsigned char r = vdata[(y * depthWidth + x) * 4];
                unsigned char g = vdata[(y * depthWidth + x) * 4 + 1];
                unsigned char b = vdata[(y * depthWidth + x) * 4 + 2];
                float v =r + g * 256.0 + b * 256.0 * 256.0;
                if (!isnan(depthVal) && depthVal < maxdist)
                {
                    float xrw = (x - xOff) * depthVal / xScl;
                    float yrw = (y - yOff) * depthVal / yScl;
                    Vec4f viewPos = Vec4f(xrw, yrw, depthVal, 1);
                    Vec4f modelPos;
                    xform(modelPos, matTransform, viewPos);
                    pos.push_back(Vec4f(modelPos[0], modelPos[1], modelPos[2], v));
                }
                else
                {
                    pos.push_back(Vec4f(INFINITY, INFINITY, INFINITY, v));
                }
            }
        }
    }

    void GetDepthPoints(const std::vector<float>& floatArray,
        std::vector<Point3f>& outPts, int depthWidth, int depthHeight, float maxdist)
    {
        float ratioX = floatArray[9] / depthWidth;
        float ratioY = floatArray[10] / depthHeight;
        float xScl = floatArray[0] / ratioX;
        float yScl = floatArray[4] / ratioY;
        float xOff = floatArray[6] / ratioX + 30;
        float yOff = floatArray[7] / ratioY;


        Quatf q = make<gmtl::Quatf>(AxisAnglef(pi, 0.0f, 1.0f, 0.0f)) *
            make<gmtl::Quatf>(AxisAnglef(-pi * 0.5f, 0.0f, 0.0f, 1.0f));
        Matrix44f matTransform = makeRot<Matrix44f>(q);

        int dOffsetX = 0;
        int dOffsetY = 0;
        float dSclX = 1.0f;
        float dSclY = 1.0f;

        std::vector<Point3f>& pos = outPts;
        const float* depthArray = floatArray.data() + 16;

        for (int y = 0; y < depthHeight; ++y)
        {
            for (int x = 0; x < depthWidth; ++x)
            {
                float depthVal = depthArray[y * depthWidth + x];
                if (!isnan(depthVal) && depthVal < maxdist)
                {
                    float xrw = (x - xOff) * depthVal / xScl;
                    float yrw = (y - yOff) * depthVal / yScl;
                    Vec4f viewPos = Vec4f(xrw, yrw, depthVal, 1);
                    Vec4f modelPos;
                    xform(modelPos, matTransform, viewPos);
                    pos.push_back(Point3f(modelPos[0], modelPos[1], modelPos[2]));
                }
                else
                {
                    pos.push_back(Point3f(INFINITY, INFINITY, INFINITY));
                }
            }
        }
    }
}