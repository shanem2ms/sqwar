#include "StdIncludes.h"
#include "Network.h"
#include "mdns.h"
#include <thread>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <asio.hpp>


extern "C" {
    int send_mdns_query(const char* service, int record,
        struct ipaddr_array* poutip);
    int service_with_hostname(const char* service);
}

namespace sam
{
    using asio::ip::tcp;

   enum { max_length = 1024 };


    class TcpSession
        : public std::enable_shared_from_this<TcpSession>
    {
    public:
        TcpSession(tcp::socket socket)
            : socket_(std::move(socket))
        {
        }

        void start()
        {
            do_read();
        }

    private:
        void do_read()
        {
            auto self(shared_from_this());
            socket_.async_read_some(asio::buffer(data_, max_length),
                [this, self](std::error_code ec, std::size_t length)
                {
                    if (!ec)
                    {
                        do_write(length);
                    }
                });
        }

        void do_write(std::size_t length)
        {
            auto self(shared_from_this());
            asio::async_write(socket_, asio::buffer(data_, length),
                [this, self](std::error_code ec, std::size_t /*length*/)
                {
                    if (!ec)
                    {
                        do_read();
                    }
                });
        }

        tcp::socket socket_;
        enum { max_length = 1024 };
        char data_[max_length];
    };

    class TcpServer
    {
    public:
        TcpServer(asio::io_context& io_context, short port)
            : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
            socket_(io_context)
        {
            do_accept();
        }

    private:
        void do_accept()
        {
            acceptor_.async_accept(socket_,
                [this](std::error_code ec)
                {
                    if (!ec)
                    {
                        std::make_shared<TcpSession>(std::move(socket_))->start();
                    }

                    do_accept();
                });
        }

        tcp::acceptor acceptor_;
        tcp::socket socket_;
    };


    
    Server::Server() :
        m_mdnsThread(MdsnThreadFunc),
        m_tcpServerThread(TcpThreadFunc)
    {}

    void Server::MdsnThreadFunc()
    {
        service_with_hostname("SqWar");
    }

    Server::~Server()
    {
        m_mdnsThread.join();
    }

    void Server::TcpThreadFunc()
    {
#ifdef _WIN32
        WORD versionWanted = MAKEWORD(2, 2);
        WSADATA wsaData;
        if (WSAStartup(versionWanted, &wsaData)) {
            printf("Failed to initialize WinSock\n");
            return;
        }
#endif
        asio::io_context io_context;
        TcpServer tcpServer(io_context, 13579);
        io_context.run();

#ifdef _WIN32
        WSACleanup();
#endif
    }

    class TcpClient
    {
    public:
        TcpClient()
        {

        }

        void Send(const unsigned char* data, size_t len)
        {
            asio::io_context io_context;
            tcp::socket s(io_context);
            tcp::resolver resolver(io_context);
            asio::connect(s, resolver.resolve("192.168.1.40", "13579"));

            asio::write(s, asio::buffer(data, len));

            char reply[max_length];
            size_t reply_length = asio::read(s,
                asio::buffer(reply, len));
            std::cout << "Reply is: ";
            std::cout.write(reply, reply_length);
            std::cout << "\n";
        }
    };

    Client::Client() :
        m_tcpClient(std::make_unique<TcpClient>())
    {
        struct ipaddr_array iparray;
        send_mdns_query("SqWar", 12, &iparray);
    }

    bool Client::SendData(const unsigned char* data, size_t len)
    {
        m_tcpClient->Send(data, len);
        return true;
    }

    Client::~Client()
    {

    }

}



