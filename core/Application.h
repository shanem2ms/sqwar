#pragma once

#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include "gmtl/Vec.h"
#include "gmtl/Matrix.h"

namespace sam
{

class World;
class Engine;
struct DrawContext;
class Server;
class Client;
struct DepthData;
struct DepthDataProps;
struct FaceDataProps;
class FFmpegFileWriter;
class FFmpegFileReader;
class Recorder;
class Player;

class Application
{
    std::unique_ptr<World> m_world;
    std::unique_ptr<Engine> m_engine;
    int m_width;
    int m_height;
    gmtl::Vec2f m_touchDown;
    gmtl::Vec2f m_touchPos;
    int m_frameIdx;
    int m_buttonDown;
    std::string m_documentsPath;
    std::mutex m_filemtx;
    std::unique_ptr<Server> m_server;
    std::unique_ptr<Client> m_client;
    bool m_clientInit;
    static void (*m_dbgFunc)(const char*);
    bool m_isrecording;
    bool m_wasrecording;
    double m_deviceTimestamp;
    std::shared_ptr<Recorder> m_bkgWriter;
    std::shared_ptr<Player> m_player;
public:    
    Application();
    ~Application();
    static Application& Inst();
    int FrameIdx() const { return m_frameIdx; }
    void TouchDown(float x, float y, int touchId);
    void TouchMove(float x, float y, int touchId);
    void TouchUp(int touchId);
    void KeyDown(int keyId);
    void KeyUp(int keyId);
    void Resize(int w, int h);
    void Tick(float time, double deviceTimestamp);
    void DoPlayback();
    void Draw();
    void Initialize(const char *folder);
    static void SendMDNSQueryThread();
    const std::string &Documents() const
    { return m_documentsPath; }
    
    void WriteDepthDataToFile(DepthData &depthData);
    void WriteFaceDataToFile(const FaceDataProps &props, const std::vector<float> &vertices,
                             const std::vector<int16_t> indices);

    void OnDepthBufferInst(DepthData& depthData);
    static void OnDepthBuffer(DepthData& depthData);
    static void OnFaceData(FaceDataProps &props, const std::vector<float> &vertices,
                           const std::vector<int16_t> &indices);
    void OnFaceDataInst(FaceDataProps& props, const std::vector<float>& vertices,
        const std::vector<int16_t>& indices);
    static void SetDebugMsgFunc(void (*dbgfunc)(const char*));
    static void DebugMsg(const std::string& str);
};

}
