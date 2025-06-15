#include "RtmpStream.h"
#include <memory>
// ƽ̨�ض�ͷ�ļ�
// �������Windowsƽ̨
#ifndef _WIN32
// ������뼶˯�ߺ���
#define SLEEP_MS(ms) usleep((ms)*1000)
// �����Windowsƽ̨
#else
// ������뼶˯�ߺ���
#define SLEEP_MS(ms) Sleep(ms)
#endif
// �������ӳ�ʱʱ�䣬��λΪ��
const int CONNECT_TIMEOUT = 5;          // seconds
// ���建��������ʱ�䣬��λΪ�루1Сʱ��
const int BUFFER_DURATION = 3600;       // seconds (1 hour)
// ����Ŀ��֡�ʣ���λΪ֡ÿ��
const int TARGET_FRAME_RATE = 30;       // fps
// ����ÿ֮֡����ӳ�ʱ�䣬��λΪ����
const int FRAME_DELAY_MS = 1000 / TARGET_FRAME_RATE;

// ���캯������ʼ��RTMP URL
RtmpStream::RtmpStream(const char* rtmpUrl)
	: m_rtmpUrl(rtmpUrl)
	, m_rtmp(nullptr)
{
}

// ����������������Դ
RtmpStream::~RtmpStream()
{
	cleanup();
}

// ��ʼ��RTMP����
bool RtmpStream::initialize()
{
	// ����RTMP�����ڴ�
	m_rtmp = RTMP_Alloc();
	if (!m_rtmp) {
		// ��¼������־
		RTMP_Log(RTMP_LOGERROR, "RTMP_Alloc failed");
		return false;
	}
	// ��ʼ��RTMP����
	RTMP_Init(m_rtmp);

	// �������ӳ�ʱʱ��
	m_rtmp->Link.timeout = CONNECT_TIMEOUT;

	// ����RTMP URL
	if (!RTMP_SetupURL(m_rtmp, const_cast<char*>(m_rtmpUrl))) {
		// ��¼������־
		RTMP_Log(RTMP_LOGERROR, "RTMP_SetupURL failed");
		return false;
	}

	// ���÷���ģʽ
	RTMP_EnableWrite(m_rtmp);

	// ���û���������ʱ��
	RTMP_SetBufferMS(m_rtmp, BUFFER_DURATION * 1000);

	return true;
}

// ���ӵ�RTMP������
bool RtmpStream::connect()
{
	// ����RTMP����
	if (!RTMP_Connect(m_rtmp, nullptr)) {
		// ��¼������־
		RTMP_Log(RTMP_LOGERROR, "RTMP_Connect failed");
		return false;
	}

	// ���ӵ�RTMP��
	if (!RTMP_ConnectStream(m_rtmp, 0)) {
		// ��¼������־
		RTMP_Log(RTMP_LOGERROR, "RTMP_ConnectStream failed");
		return false;
	}

	return true;
}

// ����FLV�ļ���RTMP������
bool RtmpStream::writePacket(char* packet, uint32_t packetSize, uint32_t timestamp)
{
	// ��¼��ʼ������־
	RTMP_LogPrintf("Starting to publish stream...");

	// ��¼��ʼʱ��
	uint32_t startTime = RTMP_GetTime();
	// ��¼��һ֡ʱ���
	uint32_t lastTimestamp = 0;
	// ��¼�ѷ���֡��
	uint32_t framesSent = 0;

	// ���RTMP�����Ƿ���Ȼ��Ч
	if (!RTMP_IsConnected(m_rtmp)) {
		// �����Ӷ�ʧ����¼������־
		RTMP_Log(RTMP_LOGERROR, "RTMP connection lost");
		return false;
	}

	// ���Խ����ݰ�д��RTMP����
	if (!RTMP_Write(m_rtmp, (char*)packet, packetSize)) {
		// ��д��ʧ�ܣ���¼������־
		RTMP_Log(RTMP_LOGERROR, "RTMP_Write failed");
		// �ͷ��ѷ�����ڴ�
		return false;
	}

	// �����ѷ��͵�֡������
	framesSent++;

	// ֡�ʿ����߼�
	// ����ӿ�ʼ���������ھ�����ʱ��
	uint32_t elapsed = RTMP_GetTime() - startTime;
	// �����ǰ��ǩ��ʱ��������ѹ�ȥ��ʱ�䣬�����ʵ��ӳ�
	if (timestamp > elapsed) {
		// ����ƽ̨�ض���˯�ߺ��������ӳ�
		SLEEP_MS(FRAME_DELAY_MS);
	}

	// ���ڼ�¼��������
	if (framesSent % 100 == 0) {
		// ÿ����100֡����¼�ѷ���֡���͵�ǰʱ���
		RTMP_LogPrintf("Sent %u frames, timestamp: %ums",
			framesSent, timestamp);
	}

	RTMP_LogPrintf("Publishing completed. Total frames sent: %u", framesSent);
	return true;
}

bool RtmpStream::sendPacket(char* data, uint32_t dataSize, uint8_t type, uint32_t timestamp)
{
	// ��¼��ʼ������־
	RTMP_LogPrintf("Starting to sendPacket stream...");

	// ��¼��ʼʱ��
	uint32_t startTime = RTMP_GetTime();
	// ��¼��һ֡ʱ���
	uint32_t lastTimestamp = 0;
	// ��¼�ѷ���֡��
	uint32_t framesSent = 0;

	// ���RTMP�����Ƿ���Ȼ��Ч
	if (!RTMP_IsConnected(m_rtmp)) {
		// �����Ӷ�ʧ����¼������־
		RTMP_Log(RTMP_LOGERROR, "RTMP connection lost");
		return false;
	}

	// ���Խ����ݰ�д��RTMP����
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
		// ��д��ʧ�ܣ���¼������־
		RTMP_Log(RTMP_LOGERROR, "RTMP_Write failed");
		// �ͷ��ѷ�����ڴ�
		return false;
	}
	// �����ѷ��͵�֡������
	framesSent++;

	// ֡�ʿ����߼�
	// ����ӿ�ʼ���������ھ�����ʱ��
	uint32_t elapsed = RTMP_GetTime() - startTime;
	// �����ǰ��ǩ��ʱ��������ѹ�ȥ��ʱ�䣬�����ʵ��ӳ�
	if (timestamp > elapsed) {
		// ����ƽ̨�ض���˯�ߺ��������ӳ�
		SLEEP_MS(FRAME_DELAY_MS);
	}

	// ���ڼ�¼��������
	if (framesSent % 100 == 0) {
		// ÿ����100֡����¼�ѷ���֡���͵�ǰʱ���
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
