#include "StdIncludes.h"
#include "DepthPts.h"
#include "DepthProps.h"
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
        float xOff = floatArray[6] / ratioX;
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

    void DepthBuildLods(float* dbuf, float* outpts, int depthWidth, int depthHeight)
    {
        std::vector<float> dinv;
        dinv.reserve(depthWidth * depthHeight);
        float* dend = dbuf + (depthWidth * depthHeight);
        for (float* dptr = dbuf; dptr != dend; ++dptr)
        {
            if (isnan(*dptr) || isinf(*dptr))
                dinv.push_back(NAN);
            else
                dinv.push_back(1.0f / *dptr);
        }

        int dsw = depthWidth;
        int dw = depthWidth / 2;
        int dh = depthHeight / 2;
        const float* osrc = dinv.data();
        float* dsrc = dinv.data();
        float* dptr = outpts;
        while (dw >= 16)
        {
            for (int y = 0; y < dh; ++y)
            {
                for (int x = 0; x < dw; ++x)
                {
                    float v[4] = {
                        dsrc[(y * 2) * dsw + (x * 2)],
                        dsrc[(y * 2) * dsw + (x * 2) + 1],
                        dsrc[(y * 2 + 1) * dsw + (x * 2)],
                        dsrc[(y * 2 + 1) * dsw + (x * 2) + 1] };
                    float t = 0;
                    float vf = 0;
                    for (int i = 0; i < 4; ++i)
                    {
                        if (!isnan(v[i]))
                        {
                            vf += v[i];
                            t += 1.0f;
                        }
                    }
                    dptr[y * dw + x] = (t > 0) ? (vf / t) : NAN;
                }
            }

            dsrc = dptr;
            dptr += dw * dh;

            dw /= 2;
            dsw /= 2;
            dh /= 2;
        }

        for (float* ptr = outpts; ptr < dptr; ++ptr)
        {
            *ptr = !isnan(*ptr) ? 1.0f / *ptr : NAN;
        }
    }


    void ConvertDepthToYUV(float* data, int width, int height, float maxDepth, uint8_t* ydata, uint8_t* udata, uint8_t* vdata)
    {
        float scalar = 254.0f / maxDepth;
        float* line0 = data;
        uint8_t* yout0 = ydata;
        uint8_t* uout = udata;
        uint8_t* vout = vdata;
        for (int y = 0; y < height; y += 2)
        {
            float* line1 = line0 + width;
            uint8_t* yout1 = yout0 + width;
            for (int x = 0; x < width; x += 2)
            {
                float v[4] = { line0[x],
                    line0[x + 1],
                    line1[x],
                    line1[x + 1] };
                uint8_t* outy[4] = {
                    &yout0[x],
                    &yout0[x + 1],
                    &yout1[x],
                    &yout1[x + 1]
                };
                int u_yval = -1;
                float u_remavg = 0;
                float u_remtot = 0;
                int v_yval = -1;
                float v_remavg = 0;
                float v_remtot = 0;
                for (int idx = 0; idx < 4; ++idx)
                {
                    uint8_t y;
                    if (isnan(v[idx]) || isinf(v[idx]))
                    { y = 255; }
                    else {
                        float sv = v[idx] * scalar;
                        float rem = fmod(sv, 1);
                        y = (uint8_t)sv;
                        if (u_yval < 0 || u_yval == y)
                        {
                            u_remavg += rem;
                            u_remtot++;
                            u_yval = y;
                        }
                        else {
                            v_remavg += rem;
                            v_remtot++;
                            v_yval = y;
                        }

                    }
                    *outy[idx] = y;
                }
                if (u_remtot > 0)
                {
                    u_remavg /= u_remtot;
                    uout[x/2] = (uint8_t)(u_remavg * 255.0f); 
                }
                else
                {
                    uout[x / 2] = 0;
                }

                if (v_remtot > 0)
                {
                    v_remavg /= v_remtot;
                    vout[x / 2] = (uint8_t)(v_remavg * 255.0f);
                }
                else
                {
                    vout[x / 2] = 0;
                }
            }
            line0 += width * 2;
            yout0 += width * 2;
            uout += width / 2;
            vout += width / 2;
        }
    }

    void ConvertYUVToDepth(uint8_t* ydata, uint8_t* udata, uint8_t* vdata, int width, int height, float maxDepth, float* depthData)
    {
        float scalar = maxDepth / 254.0f;
        float* line0 = depthData;
        uint8_t* yin0 = ydata;
        uint8_t* uin = udata;
        uint8_t* vin = vdata;
        for (int y = 0; y < height; y += 2)
        {
            float* line1 = line0 + width;
            uint8_t* yin1 = yin0 + width;
            for (int x = 0; x < width; x += 2)
            {
                float *outv[4] = { &line0[x],
                    &line0[x + 1],
                    &line1[x],
                    &line1[x + 1] };
                uint8_t iny[4] = {
                    yin0[x],
                    yin0[x + 1],
                    yin1[x],
                    yin1[x + 1]
                };

                int u_yval = -1;
                int v_yval = -1;

                float remavg = 0;
                float remtot = 0;
                float u_rem = uin[x / 2] / 255.0f;
                float v_rem = vin[x / 2] / 255.0f;
                for (int idx = 0; idx < 4; ++idx)
                {
                    uint8_t y = iny[idx];
                    if (y == 255)
                    {                        
                        *outv[idx] = NAN;
                    }
                    else
                    {
                        float rem;
                        if (u_yval < 0 || u_yval == y) {
                            rem = u_rem; u_yval = y;
                        }
                        else 
                            rem = v_rem;

                        float sv = (float)iny[idx] + rem;
                        *outv[idx] = sv * scalar;
                    }
                }
            }
            line0 += width * 2;
            yin0 += width * 2;
            uin += width / 2;
            vin += width / 2;
        }
    }

    void CalcDepthError(const std::vector<float>& d1, const std::vector<float>& d2,
        float &outAvgErr, float &outMaxErr)
    {
        auto itD1 = d1.begin();
        auto itD2 = d2.begin();
        float totalErr = 0;
        float maxErr = 0;
        float totalCnt = 0;
        while (itD1 != d1.end())
        {
            if (!isnan(*itD1) && !isinf(*itD1))
            {
                float diff = *itD1 - *itD2;
                diff = diff * diff;
                maxErr = std::max(maxErr, diff);
                totalErr += diff;
                totalCnt++;
            }
            ++itD1;
            ++itD2;
        }

        outAvgErr = sqrt(totalErr / totalCnt);
        outMaxErr = sqrt(maxErr);
    }
}

