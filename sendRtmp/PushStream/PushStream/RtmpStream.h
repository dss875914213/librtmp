#pragma once
// RTMP头文件
// 包含RTMP系统库头文件
#include "rtmp_sys.h"
// 包含日志库头文件
#include "log.h"

class RtmpStream
{
public:
	RtmpStream(const char* rtmpUrl);
	~RtmpStream();

public:
	bool initialize();
	bool connect();
	bool writePacket(char* packet, uint32_t packetSize, uint32_t timestamp);
	bool sendPacket(char* packet, uint32_t packetSize, uint8_t type, uint32_t timestamp);

private:
	void cleanup();

private:
	const char* m_rtmpUrl;
	RTMP* m_rtmp;
};

