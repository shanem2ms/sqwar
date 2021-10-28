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

    struct YCrCbData
    {
        int yOffset;
        int yRowBytes;
        int cbCrOffset;
        int cbCrRowBytes;
    };

    inline RGBt GetRGBVal(int ix, int iy, int width, int height, const byte* yData, const byte *uvData, int yPitch)
    {
        float m[9] = { 1, 1, 1,
            0, -0.18732f, 1.8556f,
            1.57481f, -0.46813f, 0 };
        Matrix33f matyuv;
        memcpy(matyuv.mData, m, sizeof(m));
        matyuv.mState = Matrix33f::FULL;
        byte yVal = yData[iy * yPitch + ix];
        int uvy = iy / 2;
        int uvx = ix & 0xFFFE;
        byte uVal = uvData[uvy * yPitch + uvx];
        byte vVal = uvData[uvy * yPitch + uvx + 1];
        Vec3f yuv(yVal / 255.0f, (uVal / 255.0f) - 0.5f, (vVal / 255.0f) - 0.5f);
        Vec3f rgb = matyuv * yuv;
        return RGBt((byte)(rgb[2] * 255.0f), (byte)(rgb[1] * 255.0f), (byte)(rgb[0] * 255.0f));
        //return RGBt(yVal, yVal, yVal);
    }

    void GetDepthPointsWithColor(const std::vector<float>& floatArray,
        const unsigned char* vdata, int vWidth, int vHeight,
        std::vector<Vec4f>& outPts, int depthWidth, int depthHeight, float maxdist)
    {
        float ratioX = floatArray[9] / depthWidth;
        float ratioY = floatArray[10] / depthHeight;
        float xScl = floatArray[0] / ratioX;
        float yScl = floatArray[4] / ratioY;
        float xOff = floatArray[6] / ratioX + 30;
        float yOff = floatArray[7] / ratioY;

        YCrCbData* yuvData = (YCrCbData*)vdata;
        yuvData->cbCrOffset -= yuvData->yOffset;
        yuvData->yOffset = 0;

        int yPitch = yuvData->yRowBytes;
        const unsigned char* ydata = vdata + sizeof(YCrCbData);
        const unsigned char* uvData = ydata + yuvData->cbCrOffset;
        
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
                int vx = x * vWidth / depthWidth;
                int vy = y * vHeight / depthHeight;
                RGBt rgbt = GetRGBVal(vx, vy, vWidth, vHeight, ydata, uvData, yPitch);
                float v = rgbt.R + rgbt.G * 256.0 + rgbt.B * 256.0 * 256.0;
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