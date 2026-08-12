#pragma once

#include <stdint.h>

enum class NetSocketProtocol
{
	NONE,
	TCP,
	UDP
};

enum class NetSocketState
{
	CLOSED,
	OPEN,
	CONNECTED,
	BOUND
};

struct NetSocket final
{
	NetSocket() = default;
	NetSocket(NetSocketProtocol protocol);
	~NetSocket();

	void open();
	bool connect(const char* host, uint16_t port);
	bool bind(const char* host, uint16_t port);
	void close();
	bool listen(uint32_t backlog = 0x7fffffff);
	bool accept(NetSocket* client);
	int32_t receive(void* readBuffer, int32_t bufferSize);
	int32_t available();
	bool send(const void* writeBuffer, int32_t bufferSize);
	bool isValid();
	NetSocketProtocol getProtocol() const { return protocol; }
	NetSocketState getState() const { return state; }
	bool isConnected() const { return getState() == NetSocketState::CONNECTED; }
	bool isBound() const { return getState() == NetSocketState::BOUND; }

private:
	NetSocketProtocol protocol = NetSocketProtocol::NONE;
	NetSocketState state = NetSocketState::CLOSED;
	void* socketHandle = ((void*)~0);

};

namespace net
{
	void printPacket(const char* title, const char* data, size_t len);
}
