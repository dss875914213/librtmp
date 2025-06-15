#pragma once
#include <stdio.h>
#include <vector>
class FlvParse
{
public:
	FlvParse(const char* flvPath);
	~FlvParse();

public:
	bool open();
	void close();
	bool readTag(char** packet, uint32_t* packetSize, uint32_t* timestamp);
	bool readTagData(char** packet, uint32_t* packetSize, uint8_t* tagType, uint32_t* timestamp);

private:
	const char* m_flvPath{ nullptr };
	FILE* m_file{ nullptr };
	std::vector<uint8_t>  m_packet;
	uint32_t m_timestamp{ 0 };
	uint8_t m_tagType{ 0 };
};

