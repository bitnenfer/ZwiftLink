#include "net_socket.h"
#include "debug.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <time.h>
#include <ctype.h>

#pragma comment(lib, "Ws2_32.lib")

static WSADATA gWSAData = { 0 };

static void InitWinSock2() {
	static bool shouldInit = true;
	if (shouldInit) {
		int32_t result = WSAStartup(MAKEWORD(2, 2), &gWSAData);
		if (result != 0) {
			WSACleanup();
			dbg::logFmt("Failed to init winsock2 %d\n", result);
		}
	}
}

static void ResolveAddress(struct sockaddr_in* addr, const char* host, uint16_t port)
{
	struct hostent* hostname = gethostbyname(host);
	if (hostname != NULL)
	{
		memmove(&addr->sin_addr, hostname->h_addr_list[0], hostname->h_length);
	}
	else
	{
		addr->sin_addr.s_addr = inet_addr(host);
	}
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
}

NetSocket::NetSocket(NetSocketProtocol protocol) :
	socketHandle((void*)INVALID_SOCKET), protocol(protocol), state(NetSocketState::CLOSED) {
	InitWinSock2();
}

NetSocket::~NetSocket() {
	close();
}

void NetSocket::open() {
	if (!isValid()) {
		int32_t type = -1;
		int32_t prot = -1;
		switch (protocol)
		{
		case NetSocketProtocol::NONE:
			break;
		case NetSocketProtocol::UDP:
			type = SOCK_DGRAM;
			prot = IPPROTO_UDP;
			break;
		case NetSocketProtocol::TCP:
			type = SOCK_STREAM;
			prot = IPPROTO_TCP;
			break;
		default:
			break;
		}
		socketHandle = (void*)socket(AF_INET, type, prot);
		state = NetSocketState::OPEN;
	}
}

bool NetSocket::connect(const char* host, uint16_t port) {
	if (!isValid()) {
		size_t hostnameLen = strlen(host);
		if (hostnameLen > 0) {
			sockaddr_in server;
			open();
			ResolveAddress(&server, host, port);
			int32_t result = ::connect((SOCKET)socketHandle, (sockaddr*)&server, sizeof(server));
			if (result == SOCKET_ERROR) {
				dbg::logFmt("Failed to connect to %s:%u - WSA Error: %d\n", host, port, WSAGetLastError());
				close();
				return false;
			}
			state = NetSocketState::CONNECTED;
			return true;
		}
	}
	return false;
}

bool NetSocket::bind(const char* host, uint16_t port) {
	if (!isValid()) {
		size_t hostnameLen = strlen(host);
		if (hostnameLen > 0) {
			sockaddr_in server;
			open();
			ResolveAddress(&server, host, port);
			int32_t result = ::bind((SOCKET)socketHandle, (sockaddr*)&server, sizeof(server));
			if (result == SOCKET_ERROR) {
				dbg::logFmt("Failed to bind %d\n", WSAGetLastError());
				close();
				return false;
			}
			state = NetSocketState::BOUND;
			return true;
		}
	}
	return false;
}

void NetSocket::close() {
	if (isValid()) {
		closesocket((SOCKET)socketHandle);
		shutdown((SOCKET)socketHandle, SD_BOTH);
		socketHandle = (void*)INVALID_SOCKET;
		state = NetSocketState::CLOSED;
		dbg::logFmt("Connection closed.\n");
	}
}

bool NetSocket::listen(uint32_t backlog) {
	if (state == NetSocketState::BOUND) {
		int32_t result = ::listen((SOCKET)socketHandle, SOMAXCONN);
		if (result == SOCKET_ERROR) {
			dbg::logFmt("Failed to listen %d\n", WSAGetLastError());
			close();
			return false;
		}
		return true;
	}
	return false;
}

bool NetSocket::accept(NetSocket* client) {
	if (state == NetSocketState::BOUND) {
		sockaddr_in clientInfo = { 0 };
		int32_t clientInfoSize = sizeof(clientInfo);
		client->socketHandle = (void*)::accept((SOCKET)socketHandle, (sockaddr*)&clientInfo, &clientInfoSize);
		if (!client->isValid()) {
			closesocket((SOCKET)client->socketHandle);
			return false;
		}
		dbg::logFmt("---- Client Accepted ----\n");
		getpeername((SOCKET)client->socketHandle, (sockaddr*)&clientInfo, &clientInfoSize);
		const char* clientIp = inet_ntoa(clientInfo.sin_addr);
		if (clientIp != nullptr) {
			dbg::logFmt("Client accepted %s:%u.\n", clientIp, clientInfo.sin_port);
		}
		else {
			dbg::logFmt("Client accepted. No IP could be read.\n");
		}
		dbg::logFmt("-------------------------\n");
		client->protocol = protocol;
		client->state = NetSocketState::CONNECTED;
		return true;
	}
	return false;
}

int32_t NetSocket::receive(void* readBuffer, int32_t bufferSize) {
	int32_t receivedBytes = recv((SOCKET)socketHandle, (char*)readBuffer, bufferSize, 0);
	if (receivedBytes == 0) {
		close();
	}
	return receivedBytes;
}

int32_t NetSocket::available() {
	return 0;
	//fd_set readFds;
	//timeval timeout;
	//FD_ZERO(&readFds);
	//FD_SET((SOCKET)socketHandle, &readFds);
	//timeout.tv_usec = 100000;
	//return (int32_t)::select(0, &readFds, NULL, NULL, &timeout)
}

bool NetSocket::send(const void* writeBuffer, int32_t bufferSize) {
	int32_t result = ::send((SOCKET)socketHandle, (const char*)writeBuffer, bufferSize, 0);
	if (result == SOCKET_ERROR) {
		dbg::logFmt("Failed to send %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

bool NetSocket::isValid() {
	return (SOCKET)socketHandle != INVALID_SOCKET;
}

static bool _isprint(char code) {
	if (code == '\n' || code == '\r' || code == '\t' || (code >= '\x0' && code <= '\xf')) {
		return false;
	}
	return true;
}

void printPacket(const char* title, const char* data, size_t len) {
	dbg::logFmt("---- Packet [%s]: %d ----\n", title, len);
	for (int i = 0; i < len; i += 16) {
		// Print the hex values
		for (int j = 0; j < 16; ++j) {
			if (i + j < len) {
				dbg::logFmt("%02X ", (unsigned char)data[i + j]);
			}
			else {
				dbg::logFmt("   "); // Pad with spaces if beyond the length
			}
		}

		// Print the ASCII characters
		dbg::logFmt("  ");
		for (int j = 0; j < 16; ++j) {
			if (i + j < len) {
				char c = data[i + j];
				if (_isprint(c)) {
					dbg::logFmt("%c", c);
				}
				else {
					dbg::logFmt(".");
				}
			}
			else {
				dbg::logFmt(" ");
			}
		}
		dbg::logFmt("\n");
	}
	dbg::logFmt("--------\n");
}
