#include "FlvParse.h"
#include <iostream>
using namespace std;

FlvParse::FlvParse(const char* flvPath)
	:m_flvPath(flvPath)
{

}

FlvParse::~FlvParse()
{
	close();
}

bool FlvParse::open()
{
	// ��FLV�ļ�
	m_file = fopen(m_flvPath, "rb");
	if (!m_file) {
		// ��¼������־
		cerr << "Failed to open FLV file: " << m_flvPath << endl;
		return false;
	}

	// ����FLV�ļ�ͷ��9�ֽڣ��͵�һ��PreviousTagSize��4�ֽڣ�
	if (fseek(m_file, 13, SEEK_SET) != 0) {
		// ��¼������־
		cerr << "Failed to seek FLV file" << endl;
		return false;
	}

	return true;
}

void FlvParse::close()
{
	if (m_file)
	{
		fclose(m_file);
		m_file = nullptr;
	}
}

bool FlvParse::readTag(char** packet, uint32_t* packetSize, uint32_t* timestamp)
{
	bool bRes = false;
	do
	{
		// ��ȡ��ǩͷ��11�ֽڣ�
		uint8_t header[11];
		if (fread(header, 1, 11, m_file) != 11) break;

		// ������ǩ��Ϣ
		// ��ǩ����
		uint8_t tagType = header[0];
		// ���ݴ�С
		uint32_t dataSize = (header[1] << 16) | (header[2] << 8) | header[3];
		// ʱ���
		m_timestamp = (header[4] << 16) | (header[5] << 8) | header[6];
		// ��չʱ����������Ҫ��
		m_timestamp |= (header[7] << 24);

		// ��������Ƶ/��Ƶ��ǩ
		if (tagType != 0x08 && tagType != 0x09) {
			// �ƶ��ļ�ָ���������ݺ�PreviousTagSize
			fseek(m_file, dataSize + 4, SEEK_CUR);
			continue;
		}

		// ���仺��������������ǩ��ͷ + ���� + ǰһ����ǩ��С��
		uint32_t packetSize = 11 + dataSize + 4;
		cout << "packetSize: " << packetSize << endl;
		// ��̬�����ڴ��Դ洢���������ݰ�
		m_packet.resize(packetSize);

		// ���Ʊ�ǩͷ�����ݰ�������
		memcpy(m_packet.data(), header, 11);

		// ���ļ��ж�ȡ���ݲ��ֵ����ݰ�������
		if (fread(m_packet.data() + 11, 1, dataSize, m_file) != dataSize) {
			// ����ȡʧ�ܣ��ͷ��ѷ�����ڴ�
			m_packet.clear();
			break;
		}

		// �����ļ��е�ǰһ����ǩ��С�ֶΣ���Ϊ���ǽ��Լ����㲢���
		fseek(m_file, 4, SEEK_CUR);

		// ����ǰһ����ǩ�Ĵ�С
		uint32_t prevTagSize = 11 + dataSize;
		// ��ǰһ����ǩ��С�Դ���ֽ���д�����ݰ�ĩβ
		m_packet[11 + dataSize + 0] = (prevTagSize >> 24) & 0xFF;
		m_packet[11 + dataSize + 1] = (prevTagSize >> 16) & 0xFF;
		m_packet[11 + dataSize + 2] = (prevTagSize >> 8) & 0xFF;
		m_packet[11 + dataSize + 3] = prevTagSize & 0xFF;
		bRes = true;
		break;
	} while (1);
	if (packet && packetSize && timestamp)
	{
		*packet = (char*)m_packet.data();
		*packetSize = m_packet.size();
		*timestamp = m_timestamp;
	}

	return bRes;
}
