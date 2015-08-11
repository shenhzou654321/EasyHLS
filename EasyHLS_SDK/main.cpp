/*
	Copyright (c) 2013-2014 EasyDarwin.ORG.  All rights reserved.
	Github: https://github.com/EasyDarwin
	WEChat: EasyDarwin
	Website: http://www.EasyDarwin.org
*/
#include <stdio.h>
#include "EasyHLSAPI.h"
#include <windows.h>
#include "hi_type.h"
#include "hi_net_dev_sdk.h"
#include "hi_net_dev_errors.h"

#define CHOST	"192.168.1.106"		//EasyCamera摄像机IP地址
#define CPORT	80					//EasyCamera摄像机端口
#define CNAME	"admin"
#define CPWORD	"admin"

HI_U32 u32Handle = 0;


#define PLAYLIST_CAPACITY	4
#define	ALLOW_CACHE			false
#define	M3U8_VERSION		3
#define TARGET_DURATION		4
#define HLS_ROOT_DIR		"./"
#define HLS_SESSION_NAME	"camera"
#define HTTP_ROOT_URL		"http://www.easydarwin.org/easyhls/"

Easy_HLS_Handle fHlsHandle = 0;


HI_S32 OnEventCallback(HI_U32 u32Handle, /* 句柄 */
                                HI_U32 u32Event,      /* 事件 */
                                HI_VOID* pUserData  /* 用户数据*/
                                )
{
	return HI_SUCCESS;
}


HI_S32 NETSDK_APICALL OnStreamCallback(HI_U32 u32Handle, /* 句柄 */
                                HI_U32 u32DataType,     /* 数据类型，视频或音频数据或音视频复合数据 */
                                HI_U8*  pu8Buffer,      /* 数据包含帧头 */
                                HI_U32 u32Length,      /* 数据长度 */
                                HI_VOID* pUserData    /* 用户数据*/
                                )
{

	HI_S_AVFrame* pstruAV = HI_NULL;
	HI_S_SysHeader* pstruSys = HI_NULL;
	

	if (u32DataType == HI_NET_DEV_AV_DATA)
	{
		pstruAV = (HI_S_AVFrame*)pu8Buffer;

		if (pstruAV->u32AVFrameFlag == HI_NET_DEV_VIDEO_FRAME_FLAG)
		{
			if(fHlsHandle == 0 ) return 0;
					
			printf("Get Video Len:%d Timestamp:%d \n", pstruAV->u32AVFrameLen, pstruAV->u32AVFramePTS);
	
			unsigned int uiFrameType = 0;
			if (pstruAV->u32VFrameType == HI_NET_DEV_VIDEO_FRAME_I)
			{
				uiFrameType = TS_TYPE_PES_VIDEO_I_FRAME;
			}
			else
			{
				uiFrameType = TS_TYPE_PES_VIDEO_P_FRAME;
			}

			EasyHLS_VideoMux(fHlsHandle, uiFrameType, (unsigned char*)pu8Buffer+sizeof(HI_S_AVFrame), pstruAV->u32AVFrameLen, pstruAV->u32AVFramePTS*90, pstruAV->u32AVFramePTS*90, pstruAV->u32AVFramePTS*90);

		}
		else
		if (pstruAV->u32AVFrameFlag == HI_NET_DEV_AUDIO_FRAME_FLAG)
		{
			//printf("Audio %u PTS: %u \n", pstruAV->u32AVFrameLen, pstruAV->u32AVFramePTS);	
		}
	}
	else if (u32DataType == HI_NET_DEV_SYS_DATA)
	{
		pstruSys = (HI_S_SysHeader*)pu8Buffer;
		printf("Video W:%u H:%u Audio: %u \n", pstruSys->struVHeader.u32Width, pstruSys->struVHeader.u32Height, pstruSys->struAHeader.u32Format);
	} 

	return HI_SUCCESS;
}

HI_S32 OnDataCallback(HI_U32 u32Handle, /* 句柄 */
                                HI_U32 u32DataType,       /* 数据类型*/
                                HI_U8*  pu8Buffer,      /* 数据 */
                                HI_U32 u32Length,      /* 数据长度 */
                                HI_VOID* pUserData    /* 用户数据*/
                                )
{
	return HI_SUCCESS;
}

int main()
{
    HI_S32 s32Ret = HI_SUCCESS;
    HI_S_STREAM_INFO struStreamInfo;
    HI_U32 a;
    
    HI_NET_DEV_Init();
    
    s32Ret = HI_NET_DEV_Login(&u32Handle, CNAME, CPWORD, CHOST, CPORT);
    if (s32Ret != HI_SUCCESS)
    {
        HI_NET_DEV_DeInit();
		return -1;
    }
    
	//HI_NET_DEV_SetEventCallBack(u32Handle, OnEventCallback, &a);
	HI_NET_DEV_SetStreamCallBack(u32Handle, (HI_ON_STREAM_CALLBACK)OnStreamCallback, &a);
	//HI_NET_DEV_SetDataCallBack(u32Handle, OnDataCallback, &a);

	struStreamInfo.u32Channel = HI_NET_DEV_CHANNEL_1;
	struStreamInfo.blFlag = HI_FALSE;//HI_FALSE;
	struStreamInfo.u32Mode = HI_NET_DEV_STREAM_MODE_TCP;
	struStreamInfo.u8Type = HI_NET_DEV_STREAM_ALL;
	s32Ret = HI_NET_DEV_StartStream(u32Handle, &struStreamInfo);
	if (s32Ret != HI_SUCCESS)
	{
		HI_NET_DEV_Logout(u32Handle);
		u32Handle = 0;
		return -1;
	}    
	
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
   
    HI_NET_DEV_StopStream(u32Handle);
    HI_NET_DEV_Logout(u32Handle);
    
    HI_NET_DEV_DeInit();

    return 0;
}