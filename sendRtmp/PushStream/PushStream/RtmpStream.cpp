#include "RtmpStream.h"
#include <memory>
// 平台特定头文件
// 如果不是Windows平台
#ifndef _WIN32
// 定义毫秒级睡眠函数
#define SLEEP_MS(ms) usleep((ms)*1000)
// 如果是Windows平台
#else
// 定义毫秒级睡眠函数
#define SLEEP_MS(ms) Sleep(ms)
#endif
// 定义连接超时时间，单位为秒
const int CONNECT_TIMEOUT = 5;          // seconds
// 定义缓冲区持续时间，单位为秒（1小时）
const int BUFFER_DURATION = 3600;       // seconds (1 hour)
// 定义目标帧率，单位为帧每秒
const int TARGET_FRAME_RATE = 30;       // fps
// 计算每帧之间的延迟时间，单位为毫秒
const int FRAME_DELAY_MS = 1000 / TARGET_FRAME_RATE;

// 构造函数，初始化RTMP URL
RtmpStream::RtmpStream(const char* rtmpUrl)
	: m_rtmpUrl(rtmpUrl)
	, m_rtmp(nullptr)
{
}

// 析构函数，清理资源
RtmpStream::~RtmpStream()
{
	cleanup();
}

// 初始化RTMP连接
bool RtmpStream::initialize()
{
	// 分配RTMP对象内存
	m_rtmp = RTMP_Alloc();
	if (!m_rtmp) {
		// 记录错误日志
		RTMP_Log(RTMP_LOGERROR, "RTMP_Alloc failed");
		return false;
	}
	// 初始化RTMP对象
	RTMP_Init(m_rtmp);

	// 设置连接超时时间
	m_rtmp->Link.timeout = CONNECT_TIMEOUT;

	// 设置RTMP URL
	if (!RTMP_SetupURL(m_rtmp, const_cast<char*>(m_rtmpUrl))) {
		// 记录错误日志
		RTMP_Log(RTMP_LOGERROR, "RTMP_SetupURL failed");
		return false;
	}

	// 启用发布模式
	RTMP_EnableWrite(m_rtmp);

	// 设置缓冲区持续时间
	RTMP_SetBufferMS(m_rtmp, BUFFER_DURATION * 1000);

	return true;
}

// 连接到RTMP服务器
bool RtmpStream::connect()
{
	// 建立RTMP连接
	if (!RTMP_Connect(m_rtmp, nullptr)) {
		// 记录错误日志
		RTMP_Log(RTMP_LOGERROR, "RTMP_Connect failed");
		return false;
	}

	// 连接到RTMP流
	if (!RTMP_ConnectStream(m_rtmp, 0)) {
		// 记录错误日志
		RTMP_Log(RTMP_LOGERROR, "RTMP_ConnectStream failed");
		return false;
	}

	return true;
}

// 发布FLV文件到RTMP服务器
bool RtmpStream::writePacket(char* packet, uint32_t packetSize, uint32_t timestamp)
{
	// 记录开始发布日志
	RTMP_LogPrintf("Starting to publish stream...");

	// 记录开始时间
	uint32_t startTime = RTMP_GetTime();
	// 记录上一帧时间戳
	uint32_t lastTimestamp = 0;
	// 记录已发送帧数
	uint32_t framesSent = 0;

	// 检查RTMP连接是否仍然有效
	if (!RTMP_IsConnected(m_rtmp)) {
		// 若连接丢失，记录错误日志
		RTMP_Log(RTMP_LOGERROR, "RTMP connection lost");
		return false;
	}

	// 尝试将数据包写入RTMP连接
	if (!RTMP_Write(m_rtmp, (char*)packet, packetSize)) {
		// 若写入失败，记录错误日志
		RTMP_Log(RTMP_LOGERROR, "RTMP_Write failed");
		// 释放已分配的内存
		return false;
	}

	// 增加已发送的帧数计数
	framesSent++;

	// 帧率控制逻辑
	// 计算从开始发布到现在经过的时间
	uint32_t elapsed = RTMP_GetTime() - startTime;
	// 如果当前标签的时间戳大于已过去的时间，进行适当延迟
	if (timestamp > elapsed) {
		// 调用平台特定的睡眠函数进行延迟
		SLEEP_MS(FRAME_DELAY_MS);
	}

	// 定期记录发布进度
	if (framesSent % 100 == 0) {
		// 每发送100帧，记录已发送帧数和当前时间戳
		RTMP_LogPrintf("Sent %u frames, timestamp: %ums",
			framesSent, timestamp);
	}

	RTMP_LogPrintf("Publishing completed. Total frames sent: %u", framesSent);
	return true;
}

bool RtmpStream::sendPacket(char* data, uint32_t dataSize, uint8_t type, uint32_t timestamp)
{
	// 记录开始发布日志
	RTMP_LogPrintf("Starting to sendPacket stream...");

	// 记录开始时间
	uint32_t startTime = RTMP_GetTime();
	// 记录上一帧时间戳
	uint32_t lastTimestamp = 0;
	// 记录已发送帧数
	uint32_t framesSent = 0;

	// 检查RTMP连接是否仍然有效
	if (!RTMP_IsConnected(m_rtmp)) {
		// 若连接丢失，记录错误日志
		RTMP_Log(RTMP_LOGERROR, "RTMP connection lost");
		return false;
	}

	// 尝试将数据包写入RTMP连接
	std::unique_ptr<RTMPPacket> packet = std::make_unique<RTMPPacket>();
	ZeroMemory(packet.get(), sizeof(RTMPPacket));
	packet->m_hasAbsTimestamp = 0;
	packet->m_nChannel = 0x04;
	packet->m_nInfoField2 = m_rtmp->m_stream_id;
	packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	packet->m_nTimeStamp = timestamp;
	packet->m_packetType = type;
	packet->m_nBodySize = dataSize;
	RTMPPacket_Alloc(packet.get(), dataSize);
	memcpy(packet->m_body, data, dataSize);

	if (!RTMP_SendPacket(m_rtmp, packet.get(), TRUE)) {
		// 若写入失败，记录错误日志
		RTMP_Log(RTMP_LOGERROR, "RTMP_Write failed");
		// 释放已分配的内存
		return false;
	}
	// 增加已发送的帧数计数
	framesSent++;

	// 帧率控制逻辑
	// 计算从开始发布到现在经过的时间
	uint32_t elapsed = RTMP_GetTime() - startTime;
	// 如果当前标签的时间戳大于已过去的时间，进行适当延迟
	if (timestamp > elapsed) {
		// 调用平台特定的睡眠函数进行延迟
		SLEEP_MS(FRAME_DELAY_MS);
	}

	// 定期记录发布进度
	if (framesSent % 100 == 0) {
		// 每发送100帧，记录已发送帧数和当前时间戳
		RTMP_LogPrintf("Sent %u frames, timestamp: %ums",
			framesSent, timestamp);
	}

	RTMP_LogPrintf("SendPacket completed. Total frames sent: %u", framesSent);
	return true;
}

void RtmpStream::cleanup()
{
	if (m_rtmp) {
		if (RTMP_IsConnected(m_rtmp)) {
			RTMP_Close(m_rtmp);
		}
		RTMP_Free(m_rtmp);
		m_rtmp = nullptr;
	}
}
