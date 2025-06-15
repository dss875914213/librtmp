#pragma once
// RTMPͷ�ļ�
// ����RTMPϵͳ��ͷ�ļ�
#include "rtmp_sys.h"
// ������־��ͷ�ļ�
#include "log.h"

class RtmpStream
{
public:
	RtmpStream(const char* rtmpUrl);
	~RtmpStream();

public:
	bool initialize();
	bool connect();
	bool publish(char* packet, uint32_t packetSize, uint32_t timestamp);

private:
	void cleanup();

private:
	const char* m_rtmpUrl;
	RTMP* m_rtmp;
};

