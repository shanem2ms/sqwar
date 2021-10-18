#pragma once

#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <memory>
#include "gmtl/Vec.h"

namespace sam
{

    class World;
    class Engine;
    struct DrawContext;

    class Server
    {
        std::thread m_mdnsThread;
        std::thread m_tcpServerThread;
       
    public:
        Server();
        ~Server();
        static void MdsnThreadFunc();
        static void TcpThreadFunc();
    };


    class TcpClient;
    class Client
    {
        std::unique_ptr<TcpClient> m_tcpClient;
    public:

        Client();
        ~Client();
        bool SendData(const unsigned char* data, size_t len);
    };
}