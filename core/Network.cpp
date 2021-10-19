#include "StdIncludes.h"
#include "Network.h"
#include "Application.h"
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
    mdns_string_t
        ipv4_address_to_string(char* buffer, size_t capacity, const struct sockaddr_in* addr,
            size_t addrlen);

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
            : socket_(std::move(socket)),
            m_pCurPacket(nullptr),
            data(max_length)
        {
            Application::DebugMsg("TcpSession");
        }

        ~TcpSession()
        {
            Application::DebugMsg("~TcpSession");
        }

        void start()
        {
            do_read();
        }

    private:
            
        struct DataPacket
        {
            size_t m_totalBytes;
            size_t m_bytesRead;
            
        };

        DataPacket* m_pCurPacket;
        size_t totalPackets = 0;
        void do_read()
        {
            auto self(shared_from_this());
            socket_.async_read_some(asio::buffer(data.data(), max_length),
                [this, self](std::error_code ec, std::size_t length)
                {
                    if (m_pCurPacket == nullptr)
                    {
                        if (length != sizeof(size_t))
                            throw;
                        m_pCurPacket = new DataPacket();
                        m_pCurPacket->m_totalBytes = *(size_t*)(data.data());
                        m_pCurPacket->m_bytesRead = 0;
                    }
                    else
                    {
                        m_pCurPacket->m_bytesRead += length;
                        if (m_pCurPacket->m_bytesRead ==
                            m_pCurPacket->m_totalBytes)
                        {
                            std::stringstream ss;
                            ss << "received packet. " << totalPackets++ << "\r\n";
                            Application::DebugMsg(ss.str());
                            delete m_pCurPacket;
                            m_pCurPacket = nullptr;
                        }
                    }
                    send_response(1);
                    do_read();
                });
        }

        void send_response(std::size_t responsecode)
        {
            auto self(shared_from_this());
            char* buf = new char[sizeof(responsecode)];
            memcpy(buf, &responsecode, sizeof(responsecode));
            asio::async_write(socket_, asio::buffer(buf, sizeof(responsecode)),
                [this, self, buf](std::error_code ec, std::size_t /*length*/)
                {
                    delete[]buf;
                });
        }

        tcp::socket socket_;
        enum { max_length = 1 << 16 };
        std::vector<char> data;
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
        std::string m_ippaddr;
        asio::io_context io_context;
        tcp::socket s;
    public:
        TcpClient(const std::string
                  &ippaddr) : 
                    m_ippaddr(ippaddr),
                    s(io_context)
        {
        }

        void ConnectIfNeeded()
        {
            if (!s.is_open())
            {
                tcp::resolver resolver(io_context);
                asio::connect(s, resolver.resolve(m_ippaddr, "13579"));
            }
        }
        void Send(const unsigned char* data, size_t len)
        {
            ConnectIfNeeded();
            int64_t bytesRemain = len;
            size_t chunkSize = 1 << 16;            
            {
                size_t responseCode = 0;
                asio::error_code ec;
                asio::write(s, asio::buffer(&len, sizeof(len)));
                size_t replysize =
                    s.read_some(asio::buffer(&responseCode, sizeof(responseCode)), ec);
            }
            while (bytesRemain > 0)
            {
                size_t sentBytes = min(chunkSize, bytesRemain);
                asio::write(s, asio::buffer(data, sentBytes));
                size_t responseCode = 0;
                asio::error_code ec;
                std::stringstream ss;
                Application::DebugMsg(ss.str());
                size_t replysize =
                    s.read_some(asio::buffer(&responseCode, sizeof(responseCode)), ec);
                bytesRemain -= chunkSize;
            }
        }
    };

    Client::Client()
    {
        struct ipaddr_array iparray;
        send_mdns_query("SqWar", 12, &iparray);
        char ipaddr[256];
        for (int idx = 0; idx < iparray.count; ++idx)
        {
            ipv4_address_to_string(ipaddr, sizeof(ipaddr), &iparray.addr[idx], sizeof(iparray.addr[idx]));
        }
        
        m_tcpClient = std::make_unique<TcpClient>(ipaddr);
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



