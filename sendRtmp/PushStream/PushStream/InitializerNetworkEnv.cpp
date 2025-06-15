#include "InitializerNetworkEnv.h"
#include <iostream>
// 平台特定头文件
// 如果不是Windows平台
#ifndef _WIN32
// 包含Unix系统调用库，提供系统级功能
#include <unistd.h>
// 如果是Windows平台
#else
// 包含Windows套接字库，用于网络编程
#include <winsock2.h>
// 链接Windows套接字库
#pragma comment(lib, "ws2_32.lib")
#endif
InitializerNetworkEnv::InitializerNetworkEnv()
{
#ifdef _WIN32
	// 定义Windows套接字数据结构
	WSADATA wsaData;
	// 初始化Windows套接字库
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		// 抛出异常表示网络初始化失败
		throw std::runtime_error("Network initialization failed");
	}
#endif
}

InitializerNetworkEnv::~InitializerNetworkEnv()
{
#ifdef _WIN32
	// 清理Windows套接字库
	WSACleanup();
#endif
}
