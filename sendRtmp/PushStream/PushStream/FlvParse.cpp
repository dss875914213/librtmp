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
	// 打开FLV文件
	m_file = fopen(m_flvPath, "rb");
	if (!m_file) {
		// 记录错误日志
		cerr << "Failed to open FLV file: " << m_flvPath << endl;
		return false;
	}

	// 跳过FLV文件头（9字节）和第一个PreviousTagSize（4字节）
	if (fseek(m_file, 13, SEEK_SET) != 0) {
		// 记录错误日志
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
		// 读取标签头（11字节）
		uint8_t header[11];
		if (fread(header, 1, 11, m_file) != 11) break;

		// 解析标签信息
		// 标签类型
		uint8_t tagType = header[0];
		// 数据大小
		uint32_t dataSize = (header[1] << 16) | (header[2] << 8) | header[3];
		// 时间戳
		m_timestamp = (header[4] << 16) | (header[5] << 8) | header[6];
		// 扩展时间戳（如果需要）
		m_timestamp |= (header[7] << 24);

		// 跳过非音频/视频标签
		if (tagType != 0x08 && tagType != 0x09) {
			// 移动文件指针跳过数据和PreviousTagSize
			fseek(m_file, dataSize + 4, SEEK_CUR);
			continue;
		}

		// 分配缓冲区用于完整标签（头 + 数据 + 前一个标签大小）
		uint32_t packetSize = 11 + dataSize + 4;
		cout << "packetSize: " << packetSize << endl;
		// 动态分配内存以存储完整的数据包
		m_packet.resize(packetSize);

		// 复制标签头到数据包缓冲区
		memcpy(m_packet.data(), header, 11);

		// 从文件中读取数据部分到数据包缓冲区
		if (fread(m_packet.data() + 11, 1, dataSize, m_file) != dataSize) {
			// 若读取失败，释放已分配的内存
			m_packet.clear();
			break;
		}

		// 跳过文件中的前一个标签大小字段，因为我们将自己计算并添加
		fseek(m_file, 4, SEEK_CUR);

		// 计算前一个标签的大小
		uint32_t prevTagSize = 11 + dataSize;
		// 将前一个标签大小以大端字节序写入数据包末尾
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
