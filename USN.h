#ifndef _USN_H
#define _USN_H
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <windef.h>
#include <winioctl.h>
#include <windows.h>
#include <vector>
#include <deque>
#include <fstream>
#include "file.h"
using namespace std;
#define BUF_LEN 4096
#define ull unsigned long long
struct MY_USN_RECORD
{
	DWORDLONG FileReferenceNumber;
	DWORDLONG ParentFileReferenceNumber;
	LARGE_INTEGER TimeStamp;
	DWORD Reason;
	WCHAR FileName[MAX_PATH];
	USN usn;
};
USN GET_MFT(string data_dst,string s,bool flag,vector<dat>&v)
{
	/*获取驱动句柄*/
	s="\\\\.\\"+s+":";
	const char* dv=s.c_str();
	HANDLE hcj = CreateFile(dv, GENERIC_READ | GENERIC_WRITE,
							FILE_SHARE_READ | FILE_SHARE_WRITE,
							NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (INVALID_HANDLE_VALUE == hcj)
		return -1;
	/*初始化USN日志文件*/
	CREATE_USN_JOURNAL_DATA cujd;
	cujd.MaximumSize = 0;
	cujd.AllocationDelta = 0;
	bool ret;
	DWORD br;
	ret = DeviceIoControl(hcj,
						  FSCTL_CREATE_USN_JOURNAL,
						  &cujd,
						  sizeof(cujd),
						  NULL,
						  0,
						  &br,
						  NULL);
	bool initUsnJournalSuccess;
	if (0 == ret)
		return -1;
	/*获取USN文件的基本信息*/
	bool getBasicInfoSuccess = false;
	USN_JOURNAL_DATA UsnInfo;
	ret = DeviceIoControl(hcj,
						  FSCTL_QUERY_USN_JOURNAL,
						  NULL,
						  0,
						  &UsnInfo,
						  sizeof(UsnInfo),
						  &br,
						  NULL);
	if (0 == ret)
		return -1;
	if(flag)return UsnInfo.UsnJournalID;
	MFT_ENUM_DATA med;
	med.StartFileReferenceNumber = 0;
	med.LowUsn = 0; //UsnInfo.FirstUsn;
	med.HighUsn = UsnInfo.NextUsn;
	CHAR buffer[BUF_LEN]; //用于储存记录的缓冲,尽量足够地大
	DWORD usnDataSize;
	PUSN_RECORD UsnRecord;
	long long cnt = 0;
	std::ofstream fout(data_dst, std::ios::binary);
	while (0 != DeviceIoControl(hcj,
								FSCTL_ENUM_USN_DATA,
								&med,
								sizeof(med),
								buffer,
								BUF_LEN,
								&usnDataSize,
								NULL))
	{
		DWORD dwRetBytes = usnDataSize - sizeof(USN);
		UsnRecord = (PUSN_RECORD)(((PCHAR)buffer) + sizeof(USN));
		while (dwRetBytes > 0)
		{
			int strLen = UsnRecord->FileNameLength;
			char fileName[MAX_PATH] = {0};
			WideCharToMultiByte(CP_OEMCP, NULL, UsnRecord->FileName, strLen / 2, fileName, strLen, NULL, FALSE);
			int rnum=(int)UsnRecord->FileReferenceNumber,prnum=(int)UsnRecord->ParentFileReferenceNumber;
			int len=strlen(fileName)+1;
			fout.write((char*)&rnum, sizeof(int));
        	fout.write((char*)&prnum, sizeof(int));
        	fout.write((char*)&len, sizeof(int));
        	fout.write(fileName, sizeof(char) * len);
			v.emplace_back(fileName,(int)UsnRecord->FileReferenceNumber,(int)UsnRecord->ParentFileReferenceNumber,len);
			char *pathBuffer[BUF_LEN];
			DWORD recordLen = UsnRecord->RecordLength;
			dwRetBytes -= recordLen;
			UsnRecord = (PUSN_RECORD)(((PCHAR)UsnRecord) + recordLen);
		}
		med.StartFileReferenceNumber = *(USN *)&buffer;
	}
	int end=-1;
	fout.write((char*)&end,sizeof(int));
	CloseHandle(hcj);
	fout.close();
	return UsnInfo.UsnJournalID;
}
bool is_ntfs(const char* drv){
	bool ret=0;
	HANDLE hVol = INVALID_HANDLE_VALUE;
	char FileSystemName[MAX_PATH+1];
	DWORD MaximumComponentLength;
	if(GetVolumeInformationA((std::string(drv)+":\\").c_str(),0,0,0,&MaximumComponentLength,0,FileSystemName,MAX_PATH+1)&&0==strcmp(FileSystemName,"NTFS"))ret=1;
	CloseHandle(hVol);
	return ret;
}
USN GET_USN(const char* drvname,USN startUSN,bool flag,vector<int>&rm,vector<dat>&cr,vector<string>&new_name,int* renamed)
{
	int cnt=0;
	HANDLE hVol = INVALID_HANDLE_VALUE;
	USN ret=0;
	char FileSystemName[MAX_PATH+1];
	DWORD MaximumComponentLength;
	if( GetVolumeInformationA( (std::string(drvname)+":\\").c_str(),0,0,0,&MaximumComponentLength,0,FileSystemName,MAX_PATH+1)
		&& 0==strcmp(FileSystemName,"NTFS") ) // 判断是否为 NTFS 格式
	{
		hVol = CreateFileA( (std::string("\\\\.\\")+drvname+":").c_str() // 需要管理员权限，无奈
			, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if( hVol != INVALID_HANDLE_VALUE )
		{
			DWORD br;
			USN_JOURNAL_DATA qujd;
			if( DeviceIoControl( hVol, FSCTL_QUERY_USN_JOURNAL, NULL, 0, &qujd, sizeof(qujd), &br, NULL ) )
			{
				char buffer[0x1000];
				DWORD BytesReturned;
				{
					READ_USN_JOURNAL_DATA rujd = { startUSN, -1, 0, 0, 0, qujd.UsnJournalID };
					for( ; DeviceIoControl(hVol,FSCTL_READ_USN_JOURNAL,&rujd,sizeof(rujd),buffer,_countof(buffer),&BytesReturned,NULL); rujd.StartUsn=*(USN*)&buffer )
					{
						DWORD dwRetBytes = BytesReturned - sizeof(USN);
						PUSN_RECORD UsnRecord = (PUSN_RECORD)((PCHAR)buffer+sizeof(USN));
						if( dwRetBytes==0 )break;
						while( dwRetBytes > 0 )
						{
							const int strLen = UsnRecord->FileNameLength;
							char fileName[MAX_PATH] = {0};
							WideCharToMultiByte(CP_OEMCP, NULL, UsnRecord->FileName, strLen / 2, fileName, strLen, NULL, FALSE);
							ret=UsnRecord->Usn;
							//Reason 256 表示创建 4096 表示删除
							if(flag){
								if(UsnRecord->Reason==256)
									cr.emplace_back(fileName,(int)UsnRecord->FileReferenceNumber,(int)UsnRecord->ParentFileReferenceNumber,strLen+1,0);
								else if(UsnRecord->Reason==4096)
									rm.push_back((int)UsnRecord->FileReferenceNumber);
								else if(UsnRecord->Reason==8192){
									int tmp=renamed[(int)UsnRecord->FileReferenceNumber];
									if(!tmp){
										renamed[(int)UsnRecord->FileReferenceNumber]=++cnt;
										tmp=cnt;
										new_name.push_back(fileName);
									}
									else new_name[tmp-1]=fileName;
								}
							}
							dwRetBytes -= UsnRecord->RecordLength;
							UsnRecord = (PUSN_RECORD)( (PCHAR)UsnRecord + UsnRecord->RecordLength);
						}
					}
				}
			}
			CloseHandle( hVol );
		}
	}
	
	return ret;
}
#endif
