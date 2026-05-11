/**
 * Save FLV to File (Video Only)
 *
 * Reads FLV file, filters to keep only video, and saves to new FLV file.
 */

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
#include "FlvParse.h"

using namespace std;

// 配置
const char* INPUT_FLV = "walking-dead.flv";
const char* OUTPUT_FLV = "output_video_only.flv";

// 写入 FLV 文件头
void WriteFlvHeader(FILE* fp) {
    // FLV header: "FLV" + version(1) + flags(1, video only) + header_size(4)
    unsigned char header[9] = {
        'F', 'L', 'V',           // Signature
        0x01,                      // Version
        0x01,                      // Flags (0x01 = video only, 0x04 = audio only, 0x05 = both)
        0x00, 0x00, 0x00, 0x09    // Header size
    };
    fwrite(header, 1, 9, fp);
    
    // PreviousTagSize 0 (first tag)
    unsigned char prev_tag_size[4] = {0x00, 0x00, 0x00, 0x00};
    fwrite(prev_tag_size, 1, 4, fp);
}

// 写入一个 FLV Tag
void WriteFlvTag(FILE* fp, uint8_t tagType, uint32_t dataSize, uint32_t timestamp, unsigned char* data) {
    // Tag header (11 bytes)
    unsigned char tagHeader[11];
    tagHeader[0] = tagType;
    tagHeader[1] = (dataSize >> 16) & 0xFF;
    tagHeader[2] = (dataSize >> 8) & 0xFF;
    tagHeader[3] = dataSize & 0xFF;
    tagHeader[4] = (timestamp >> 16) & 0xFF;
    tagHeader[5] = (timestamp >> 8) & 0xFF;
    tagHeader[6] = timestamp & 0xFF;
    tagHeader[7] = (timestamp >> 24) & 0xFF;
    tagHeader[8] = 0x00;  // StreamID (always 0)
    tagHeader[9] = 0x00;
    tagHeader[10] = 0x00;
    
    fwrite(tagHeader, 1, 11, fp);
    fwrite(data, 1, dataSize, fp);
    
    // PreviousTagSize (11 + dataSize)
    uint32_t prevTagSize = 11 + dataSize;
    unsigned char prevTagBytes[4];
    prevTagBytes[0] = (prevTagSize >> 24) & 0xFF;
    prevTagBytes[1] = (prevTagSize >> 16) & 0xFF;
    prevTagBytes[2] = (prevTagSize >> 8) & 0xFF;
    prevTagBytes[3] = prevTagSize & 0xFF;
    fwrite(prevTagBytes, 1, 4, fp);
}

void PrintHexData(const char* label, unsigned char* data, int size, int max_print = 32)
{
    printf("\n========== %s ==========\n", label);
    printf("Size: %d bytes\n", size);
    printf("Data (hex): ");
    for (int i = 0; i < size && i < max_print; i++)
    {
        printf("%02X ", data[i]);
    }
    if (size > max_print)
    {
        printf("... (truncated)");
    }
    printf("\n");
}

int main(int argc, char* argv[]) {
    cout << "Starting FLV video extraction..." << endl;
    
    // Open input FLV
    FlvParse flvParse(INPUT_FLV);
    if (!flvParse.open()) {
        cerr << "Failed to open input FLV file: " << INPUT_FLV << endl;
        return 1;
    }
    
    // Open output FLV
    FILE* fp_out = fopen(OUTPUT_FLV, "wb");
    if (!fp_out) {
        cerr << "Failed to create output FLV file: " << OUTPUT_FLV << endl;
        flvParse.close();
        return 1;
    }
    
    // Write FLV header
    WriteFlvHeader(fp_out);
    cout << "FLV header written" << endl;
    
    char* packet = NULL;
    uint32_t packetSize = 0;
    uint8_t tagType = 0;
    uint32_t timestamp = 0;
    int tagCount = 0;
    int videoTagCount = 0;
    
    while (flvParse.readTagData(&packet, &packetSize, &tagType, &timestamp))
    {
        tagCount++;
        
        if (tagType == 0x09) {  // Video tag
            videoTagCount++;
            
            char label[64];
            sprintf(label, "Video Tag #%d, timestamp: %ums", videoTagCount, timestamp);
            PrintHexData(label, (unsigned char*)packet, packetSize, 48);
            
            // Parse and print SPS/PPS if it's AVC sequence header
            if (packetSize >= 2) {
                unsigned char avcPacketType = (unsigned char)packet[1];
                if (avcPacketType == 0) {
                    printf("\n========== Found AVC Sequence Header (SPS/PPS) ==========\n");
                    if (packetSize >= 10) {
                        int idx = 5;
                        idx += 5;  // Skip config
                        
                        // SPS
                        if (idx < packetSize) {
                            unsigned char numSps = (unsigned char)packet[idx++] & 0x1F;
                            for (int i = 0; i < numSps && idx + 2 < packetSize; i++) {
                                unsigned short spsLen = ((unsigned char)packet[idx] << 8) | (unsigned char)packet[idx + 1];
                                idx += 2;
                                if (idx + spsLen <= packetSize) {
                                    char spsLabel[32];
                                    sprintf(spsLabel, "SPS #%d", i + 1);
                                    PrintHexData(spsLabel, (unsigned char*)&packet[idx], spsLen);
                                    idx += spsLen;
                                }
                            }
                        }
                        
                        // PPS
                        if (idx < packetSize) {
                            unsigned char numPps = (unsigned char)packet[idx++];
                            for (int i = 0; i < numPps && idx + 2 < packetSize; i++) {
                                unsigned short ppsLen = ((unsigned char)packet[idx] << 8) | (unsigned char)packet[idx + 1];
                                idx += 2;
                                if (idx + ppsLen <= packetSize) {
                                    char ppsLabel[32];
                                    sprintf(ppsLabel, "PPS #%d", i + 1);
                                    PrintHexData(ppsLabel, (unsigned char*)&packet[idx], ppsLen);
                                    idx += ppsLen;
                                }
                            }
                        }
                    }
                }
            }
            
            // Write video tag to output FLV
            WriteFlvTag(fp_out, 0x09, packetSize, timestamp, (unsigned char*)packet);
        }
    }
    
    flvParse.close();
    fclose(fp_out);
    
    cout << "\n========== Summary ==========" << endl;
    cout << "Total tags read: " << tagCount << endl;
    cout << "Video tags saved: " << videoTagCount << endl;
    cout << "Output FLV: " << OUTPUT_FLV << endl;
    cout << "Done!" << endl;
    
    return 0;
}
