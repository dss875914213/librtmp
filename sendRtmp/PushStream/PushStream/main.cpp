/**
 * Optimized FLV to RTMP Publisher
 *
 * Improvements over original:
 * 1. Better memory management with RAII
 * 2. More accurate timing control
 * 3. Reduced file I/O operations
 * 4. Cleaner error handling
 * 5. Configurable parameters
 */

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <ctime>
#include "InitializerNetworkEnv.h"
#include "RtmpStream.h"
#include "FlvParse.h"

using namespace std;
 // 配置
 // 定义要发布的FLV文件路径
const char* FLV_FILE = "walking-dead.flv";
// 定义RTMP服务器的URL
const char* RTMP_URL = "rtmp://localhost/live/livestream";

int main(int argc, char* argv[]) {
	vector<uint8_t> data;
	data.resize(30000);

	try {
		// Initialize network (required for Windows)
		InitializerNetworkEnv netInit;

		// Create and initialize publisher
		RtmpStream publisher(RTMP_URL);
		if (!publisher.initialize()) {
			RTMP_Log(RTMP_LOGERROR, "Publisher initialization failed");
			return 1;
		}

		// Connect to RTMP server
		if (!publisher.connect()) {
			RTMP_Log(RTMP_LOGERROR, "Failed to connect to RTMP server");
			return 1;
		}

		FlvParse flvParse(FLV_FILE);
		flvParse.open();

		char* packet = NULL;
		uint32_t packetSize = 0;
		uint32_t timestamp = 0;

		while (flvParse.readTag(&packet, &packetSize, &timestamp))
		{
			// Start publishing
			if (!publisher.publish(packet, packetSize, timestamp)) {
				RTMP_Log(RTMP_LOGERROR, "Publishing failed");
				return 1;
			}
		}

	}
	catch (const std::exception& e) {
		RTMP_Log(RTMP_LOGERROR, "Exception: %s", e.what());
		return 1;
	}

	return 0;
}