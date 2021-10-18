#pragma once

#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "gmtl/Vec.h"

namespace sam
{

class World;
class Engine;
struct DrawContext;
class Server;
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
    std::unique_ptr<Server> m_server;

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
    void Tick(float time);
    void Draw();
    void Initialize(const char *folder);
    static void SendMDNSQueryThread();
    const std::string &Documents() const
    { return m_documentsPath; }
    void OnDepthBuffer(const std::vector<float> &pixelData);
};

}
