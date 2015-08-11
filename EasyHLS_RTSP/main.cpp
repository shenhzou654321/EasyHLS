/*
	Copyright (c) 2013-2014 EasyDarwin.ORG.  All rights reserved.
	Github: https://github.com/EasyDarwin
	WEChat: EasyDarwin
	Website: http://www.EasyDarwin.org
*/
#define _CRTDBG_MAP_ALLOC
#include <stdio.h>
#include "EasyHLSAPI.h"
#include "EasyNVSourceAPI.h"
#include <windows.h>

#define RTSPURL "rtsp://admin:admin@192.168.1.108/"

#define PLAYLIST_CAPACITY	4
#define	ALLOW_CACHE			false
#define	M3U8_VERSION		3
#define TARGET_DURATION		4
#define HLS_ROOT_DIR		"./"
#define HLS_SESSION_NAME	"rtsp"
#define HTTP_ROOT_URL		"http://www.easydarwin.org/easyhls/"

Easy_HLS_Handle fHlsHandle = 0;
Easy_NVS_Handle fNVSHandle = 0;

/* NVSource从RTSPClient获取数据后回调给上层 */
int Easy_APICALL __NVSourceCallBack( int _chid, int *_chPtr, int _mediatype, char *pbuf, NVS_FRAME_INFO *frameinfo)
{
	if (NULL != frameinfo)
	{
		if (frameinfo->height==1088)		frameinfo->height=1080;
		else if (frameinfo->height==544)	frameinfo->height=540;
	}

	if(NULL == fHlsHandle) return -1;

	if (_mediatype == MEDIA_TYPE_VIDEO)
	{
		printf("Get %s Video Len:%d tm:%d rtp:%d\n",frameinfo->type==FRAMETYPE_I?"I":"P", frameinfo->length, frameinfo->timestamp_sec, frameinfo->rtptimestamp);
	
		unsigned int uiFrameType = 0;
		if (frameinfo->type == FRAMETYPE_I)
		{
			uiFrameType = TS_TYPE_PES_VIDEO_I_FRAME;
		}
		else
		{
			uiFrameType = TS_TYPE_PES_VIDEO_P_FRAME;
		}

		EasyHLS_VideoMux(fHlsHandle, uiFrameType, (unsigned char*)pbuf, frameinfo->length, frameinfo->rtptimestamp, frameinfo->rtptimestamp, frameinfo->rtptimestamp);
	}
	else if (_mediatype == MEDIA_TYPE_AUDIO)
	{
		printf("Get Audio Len:%d tm:%d rtp:%d\n", frameinfo->length, frameinfo->timestamp_sec, frameinfo->rtptimestamp);
		// 暂时不对音频进行处理
	}
	else if (_mediatype == MEDIA_TYPE_EVENT)
	{
		if (NULL == pbuf && NULL == frameinfo)
		{
			printf("Connecting:%s ...\n", RTSPURL);
		}
		else if (NULL!=frameinfo && frameinfo->type==0xF1)
		{
			printf("Lose Packet:%s ...\n", RTSPURL);
		}
	}
	return 0;
}

int main()
{
	//创建NVSource
	EasyNVS_Init(&fNVSHandle);
	if (NULL == fNVSHandle) return 0;

	unsigned int mediaType = MEDIA_TYPE_VIDEO;
	//mediaType |= MEDIA_TYPE_AUDIO;	//换为NVSource, 屏蔽声音
	
	//设置数据回调处理
	EasyNVS_SetCallback(fNVSHandle, __NVSourceCallBack);
	//打开RTSP流
	EasyNVS_OpenStream(fNVSHandle, 0, RTSPURL, RTP_OVER_TCP, mediaType, 0, 0, NULL, 1000, 0);

	//创建EasyHLS Session
	fHlsHandle = EasyHLS_Session_Create(PLAYLIST_CAPACITY, ALLOW_CACHE, M3U8_VERSION);

	char subDir[64] = { 0 };
	sprintf(subDir,"%s/",HLS_SESSION_NAME);
	EasyHLS_ResetStreamCache(fHlsHandle, HLS_ROOT_DIR, subDir, HLS_SESSION_NAME, TARGET_DURATION);

	printf("HLS URL:%s%s/%s.m3u8", HTTP_ROOT_URL, HLS_SESSION_NAME, HLS_SESSION_NAME);

	while(1)
	{
		Sleep(10);	
	};

    EasyHLS_Session_Release(fHlsHandle);
    fHlsHandle = 0;
   
	EasyNVS_CloseStream(fNVSHandle);
	EasyNVS_Deinit(&fNVSHandle);
	fNVSHandle = NULL;

    return 0;
}