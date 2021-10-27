#pragma once

#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <memory>
#include <functional>
#include "gmtl/Vec.h"
namespace asio
{
    struct io_context;
}

namespace sam
{

    class World;
    class Engine;
    struct DrawContext;

    class Server
    {
        std::shared_ptr<asio::io_context> m_iocontext;
        std::thread m_mdnsThread;
        std::thread m_tcpServerThread;
        std::function<void(const std::vector<unsigned char>& data)> m_onData;
    public:
        Server(const std::function<void(const std::vector<unsigned char>& data)>& onData);
        ~Server();
        static void MdsnThreadFunc();
        static void TcpThreadFunc(Server* pServer);
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