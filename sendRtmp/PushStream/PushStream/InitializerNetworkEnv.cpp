#include "InitializerNetworkEnv.h"
#include <iostream>
// ƽ̨�ض�ͷ�ļ�
// �������Windowsƽ̨
#ifndef _WIN32
// ����Unixϵͳ���ÿ⣬�ṩϵͳ������
#include <unistd.h>
// �����Windowsƽ̨
#else
// ����Windows�׽��ֿ⣬����������
#include <winsock2.h>
// ����Windows�׽��ֿ�
#pragma comment(lib, "ws2_32.lib")
#endif
InitializerNetworkEnv::InitializerNetworkEnv()
{
#ifdef _WIN32
	// ����Windows�׽������ݽṹ
	WSADATA wsaData;
	// ��ʼ��Windows�׽��ֿ�
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		// �׳��쳣��ʾ�����ʼ��ʧ��
		throw std::runtime_error("Network initialization failed");
	}
#endif
}

InitializerNetworkEnv::~InitializerNetworkEnv()
{
#ifdef _WIN32
	// ����Windows�׽��ֿ�
	WSACleanup();
#endif
}
