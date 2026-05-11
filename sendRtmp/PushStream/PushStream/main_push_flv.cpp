///**
// * Push FLV File to RTMP
// *
// * Pushes a complete FLV file to RTMP server without modification.
// */
//
//#include <iostream>
//#include <cstdio>
//#include <cstdlib>
//#include <cstring>
//#include <cstdint>
//#include <ctime>
//#include "InitializerNetworkEnv.h"
//#include "RtmpStream.h"
//
//using namespace std;
//
//// 配置
//const char* FLV_FILE = "output_video_only.flv";
//const char* RTMP_URL = "rtmp://127.0.0.1:1935/rtmp/stream1";
//
//bool PushFlvFile(const char* flvPath, RtmpStream& publisher) {
//    FILE* fp = fopen(flvPath, "rb");
//    if (!fp) {
//        cerr << "Failed to open FLV file: " << flvPath << endl;
//        return false;
//    }
//    
//    // Skip FLV header (9 bytes) + first PreviousTagSize (4 bytes)
//    fseek(fp, 13, SEEK_SET);
//    
//    uint8_t tagHeader[11];
//    int packetCount = 0;
//    int videoCount = 0;
//    int audioCount = 0;
//    
//    cout << "Starting to push FLV file..." << endl;
//    
//    while (fread(tagHeader, 1, 11, fp) == 11) {
//        uint8_t tagType = tagHeader[0];
//        uint32_t dataSize = (tagHeader[1] << 16) | (tagHeader[2] << 8) | tagHeader[3];
//        uint32_t timestamp = (tagHeader[4] << 16) | (tagHeader[5] << 8) | tagHeader[6];
//        timestamp |= (tagHeader[7] << 24);
//        
//        // Read tag data
//        vector<uint8_t> tagData(dataSize);
//        if (fread(tagData.data(), 1, dataSize, fp) != dataSize) {
//            break;
//        }
//        
//        // Read PreviousTagSize (we'll skip it)
//        uint32_t prevTagSize;
//        fread(&prevTagSize, 4, 1, fp);
//        
//        packetCount++;
//        
//        // Determine RTMP packet type
//        uint8_t rtmpPacketType;
//        if (tagType == 0x08) {
//            rtmpPacketType = RTMP_PACKET_TYPE_AUDIO;
//            audioCount++;
//        } else if (tagType == 0x09) {
//            rtmpPacketType = RTMP_PACKET_TYPE_VIDEO;
//            videoCount++;
//        } else {
//            // Skip other tag types (script data, etc.)
//            continue;
//        }
//        
//        // Build complete FLV tag for RTMP (header + data)
//        vector<uint8_t> rtmpData(11 + dataSize);
//        memcpy(rtmpData.data(), tagHeader, 11);
//        memcpy(rtmpData.data() + 11, tagData.data(), dataSize);
//        
//        // Send packet (using RTMP_Write which expects full FLV tag)
//        // OR use sendPacket with just the data part
//        if (!publisher.writePacket((char*)rtmpData.data(), rtmpData.size(), timestamp)) {
//            cerr << "Failed to send packet " << packetCount << endl;
//            fclose(fp);
//            return false;
//        }
//        
//        if (packetCount % 100 == 0) {
//            cout << "Sent " << packetCount << " packets (video: " << videoCount << ", audio: " << audioCount << ")" << endl;
//        }
//    }
//    
//    fclose(fp);
//    
//    cout << "\n========== Push Complete ==========" << endl;
//    cout << "Total packets: " << packetCount << endl;
//    cout << "Video packets: " << videoCount << endl;
//    cout << "Audio packets: " << audioCount << endl;
//    
//    return true;
//}
//
//int main(int argc, char* argv[]) {
//    try {
//        InitializerNetworkEnv netInit;
//        
//        RtmpStream publisher(RTMP_URL);
//        if (!publisher.initialize()) {
//            cerr << "Publisher initialization failed" << endl;
//            return 1;
//        }
//        
//        if (!publisher.connect()) {
//            cerr << "Failed to connect to RTMP server" << endl;
//            return 1;
//        }
//        
//        cout << "Connected to RTMP server: " << RTMP_URL << endl;
//        
//        if (!PushFlvFile(FLV_FILE, publisher)) {
//            cerr << "Failed to push FLV file" << endl;
//            return 1;
//        }
//        
//    } catch (const exception& e) {
//        cerr << "Exception: " << e.what() << endl;
//        return 1;
//    }
//    
//    return 0;
//}
