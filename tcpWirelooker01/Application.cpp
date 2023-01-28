#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <WinSock2.h>
#include <WS2tcpip.h>

#include <string>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")


class WSHelper
{
	WSADATA m_data{};
	bool wasInitialized = false;
	bool Initialize()
	{
		auto result = WSAStartup(MAKEWORD(2, 2), &m_data);
		if (result != 0)
		{
			std::fprintf(stdout, "failed to start Winsock 2.2\n");
			return false;
		}
		wasInitialized = true;
		return true;
	}
public:
	WSHelper()
	{
		if (!Initialize())
		{
			throw std::runtime_error("failed to initialize");
		}

	}

	~WSHelper()
	{
		std::fprintf(stderr, "dtor");
		if (wasInitialized)
		{
			WSACleanup();
		}

	}

	auto& Data() const
	{
		return m_data;
	}
};


class ServerSocket
{
	SOCKET m_socket;

public:
	ServerSocket(int family, int type, int protocol) // should update it to some form of Enum, separate it from the API
	{
		m_socket = socket(family, type, protocol);
		if (m_socket == INVALID_SOCKET)
		{
			throw std::runtime_error("failed to create socket");
		}
	}

	bool Bind(sockaddr const* addr, int len)
	{
		auto result = bind(m_socket, addr, len);
		if (result == SOCKET_ERROR)
		{
			// TODO: should think about what I want to return or throw...
			return false;
		}
		return true;
	}

	bool Listen(int cConnection)
	{
		if(listen(m_socket, cConnection) == SOCKET_ERROR)
		{
			return false;
		}
		return true;
	}

	auto Accept() -> SOCKET
	{
		return accept(m_socket, nullptr, nullptr);
	}

	~ServerSocket()
	{
		if (m_socket != INVALID_SOCKET)
			closesocket(m_socket);
	}
};


class Socket
{
	SOCKET m_socket;
public:
	Socket(ServerSocket const& serverSocket)
	{
		
	}

	~Socket()
	{
	
	}
};

int main()
{

	WSHelper helper;


	struct addrinfo* pAddrInfo = nullptr;
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE; // Read up on this

	auto result = getaddrinfo(nullptr, "7777", &hints, &pAddrInfo);
	if (result != 0)
	{
		fprintf(stderr, "failed to getaddrinfo\n");
		return 1;
	}

	auto Listener = ServerSocket(pAddrInfo->ai_family, pAddrInfo->ai_socktype, pAddrInfo->ai_protocol);

	if (!Listener.Bind(pAddrInfo->ai_addr, pAddrInfo->ai_addrlen))
	{
		freeaddrinfo(pAddrInfo); // we do not need the information any more
		return 1;
	}
	freeaddrinfo(pAddrInfo); // we do not need the information any more
	if(!Listener.Listen(SOMAXCONN))
	{
		std::fprintf(stderr, "listen failed with error %d\n", WSAGetLastError());
		return 1;
	}
	

	SOCKET Client = INVALID_SOCKET;

	Client = Listener.Accept();
	if (Client == INVALID_SOCKET)
	{
		std::fprintf(stderr, "accept failed %d\n", WSAGetLastError());
		return 1;
	}

	Listener.~ServerSocket(); // According to example from MSDN, we can kill the server now.


	char recvbuf[8192];
	int iResult, iSendResult;
	int recvbuflen = 8192;

	do
	{
		iResult = recv(Client, recvbuf, recvbuflen, 0); // read up on 0 flag
		if (iResult == 0)
		{
			std::fprintf(stderr, "connection closing...\n");
		}
		else if (iResult > 0)
		{
			std::fprintf(stdout, "Bytes received %d\n", iResult);

			iSendResult = send(Client, recvbuf, iResult, 0); // read up on Zero flag
			if (iSendResult == SOCKET_ERROR)
			{
				std::fprintf(stderr, "send failed %d\n", WSAGetLastError());
				closesocket(Client);
				return 1;
			}
			std::printf("Bytes %s\n", recvbuf);
			std::printf("Bytes sent %d\n", iSendResult);
		}
	} while (iResult > 0);

	iResult = shutdown(Client, SD_SEND);
	if (iResult == SOCKET_ERROR)
	{
		std::fprintf(stderr, "shutdown failed %d\n", WSAGetLastError());
		closesocket(Client);
		return 1;
	}

	closesocket(Client);




	return 0;
}
