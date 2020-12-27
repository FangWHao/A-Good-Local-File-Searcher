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
#include <string>
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
USN GET_MFT(string data_dst, string s, bool flag, vector<dat> &v)
{
	/*获取驱动句柄*/
	s = "\\\\.\\" + s + ":";
	const char *dv = s.c_str();
	HANDLE hVol = CreateFile(dv, GENERIC_READ | GENERIC_WRITE,
							FILE_SHARE_READ | FILE_SHARE_WRITE,						//CreateFile只能打开已经存在的对象
							NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL); //忽略文件安全选项
	if (INVALID_HANDLE_VALUE == hVol)
		return -1;

	/*初始化USN日志文件*/
	CREATE_USN_JOURNAL_DATA cujd; //创建USN日志数据
	cujd.MaximumSize = 0;
	cujd.AllocationDelta = 0;
	bool ret;
	DWORD br; //byte returned
	ret = DeviceIoControl(hVol,
						  FSCTL_CREATE_USN_JOURNAL, //CTL_CODE 创建USN日志，若有则返回已有的
						  &cujd,					//Input Buffer
						  sizeof(cujd),				//Input Buffer size
						  NULL,
						  0,
						  &br, //实际输出数据的bytes
						  NULL);
	if (0 == ret)
		return -1;

	/*获取USN文件的基本信息*/
	USN_JOURNAL_DATA UsnInfo; //USN日志的基础信息，包括64位ID，最小最大USN等信息（边界），在winioctl.h中可见定义
	ret = DeviceIoControl(hVol,
						  FSCTL_QUERY_USN_JOURNAL,
						  NULL, //不需要输入信息
						  0,
						  &UsnInfo,		   //输出信息存储在UsnInfo中
						  sizeof(UsnInfo), //大小
						  &br,			   //实际大小，貌似并无卵用
						  NULL);
	if (0 == ret)
		return -1;
	if (flag) //此处只需要返回64位ID以作校验即可
	{
		CloseHandle(hVol); //不要忘了关闭句柄
		return UsnInfo.UsnJournalID;
	}

	MFT_ENUM_DATA med; //遍历MFT信息，存储了
	med.StartFileReferenceNumber = 0;
	med.LowUsn = 0;				   //UsnInfo.FirstUsn;
	med.HighUsn = UsnInfo.NextUsn; //此处的NextUsn指的是下一条可以写入的USN，也就是最大的USN，区分MaxUsn表示Change Journal能够表示的最大USN
	CHAR buffer[BUF_LEN];		   //缓冲区
	DWORD usnDataSize;
	PUSN_RECORD UsnRecord; //定义指针类型USNRecord,从Buffer中读取数据
	ofstream fout(data_dst.c_str(), ios::binary);
	while (0 != DeviceIoControl(hVol,
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
			//将wchar的FileName转换成char数组，便于存储
			int rnum = (int)UsnRecord->FileReferenceNumber, prnum = (int)UsnRecord->ParentFileReferenceNumber;
			int len = strlen(fileName) + 1;
			fout.write((char *)&rnum, sizeof(int));
			fout.write((char *)&prnum, sizeof(int));
			fout.write((char *)&len, sizeof(int));
			fout.write(fileName, sizeof(char) * len);
			v.emplace_back(fileName, (int)UsnRecord->FileReferenceNumber, (int)UsnRecord->ParentFileReferenceNumber, len);
			char *pathBuffer[BUF_LEN];
			DWORD recordLen = UsnRecord->RecordLength;
			dwRetBytes -= recordLen;
			UsnRecord = (PUSN_RECORD)(((PCHAR)UsnRecord) + recordLen); //指针偏移recordLen的量，访问下一条记录
		}
		med.StartFileReferenceNumber = *(USN *)&buffer; //更新读取的起始USN
	}
	int end = -1;
	fout.write((char *)&end, sizeof(int));
	CloseHandle(hVol); //
	fout.close();
	return UsnInfo.UsnJournalID;
}
bool is_ntfs(const char *drv)
{
	bool ret = 0;
	HANDLE hVol = INVALID_HANDLE_VALUE;
	char FileSystemName[MAX_PATH + 1];
	DWORD MaximumComponentLength;
	if (GetVolumeInformationA((std::string(drv) + ":\\").c_str(),
							  0,
							  0,
							  0,
							  &MaximumComponentLength,
							  0,
							  FileSystemName, MAX_PATH + 1) &&
		0 == strcmp(FileSystemName, "NTFS"))
		ret = 1;
	CloseHandle(hVol);
	return ret;
}
USN GET_USN(const char *drvname, USN startUSN, bool flag, bool* removed, vector<dat> &cr, vector<string> &new_name, vector<int> &new_prnum, int *renamed)
{
	int cnt = 0;
	HANDLE hVol = INVALID_HANDLE_VALUE;
	USN ret = 0;
	char FileSystemName[MAX_PATH + 1];
	DWORD MaximumComponentLength;
	if (!GetVolumeInformationA((std::string(drvname) + ":\\").c_str(),
							   0,
							   0,
							   0,
							   &MaximumComponentLength,
							   0,
							   FileSystemName,
							   MAX_PATH + 1) &&
		0 == strcmp(FileSystemName, "NTFS")) // 判断是否为 NTFS 格式
		return 0;
	hVol = CreateFileA((std::string("\\\\.\\") + drvname + ":").c_str(),
					   GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hVol == INVALID_HANDLE_VALUE)
		return 0;
	DWORD br;
	USN_JOURNAL_DATA qujd;
	if (!DeviceIoControl(hVol, FSCTL_QUERY_USN_JOURNAL, NULL, 0, &qujd, sizeof(qujd), &br, NULL))
		return 0;
	char buffer[BUF_LEN];
	DWORD BytesReturned;
	READ_USN_JOURNAL_DATA rujd = {startUSN,4294967295,0,0,0,qujd.UsnJournalID};
	while (DeviceIoControl(hVol,
						   FSCTL_READ_USN_JOURNAL,
						   &rujd,
						   sizeof(rujd),
						   buffer,
						   BUF_LEN,
						   &BytesReturned, NULL))
	{
		DWORD dwRetBytes = BytesReturned - sizeof(USN);
		PUSN_RECORD UsnRecord = (PUSN_RECORD)((PCHAR)buffer + sizeof(USN));
		if (dwRetBytes == 0)
			break;
		while (dwRetBytes > 0)
		{
			const int strLen = UsnRecord->FileNameLength;
			char fileName[MAX_PATH] = {0};
			WideCharToMultiByte(CP_OEMCP, NULL, UsnRecord->FileName, strLen / 2, fileName, strLen, NULL, FALSE);
			ret = UsnRecord->Usn;
			//cout<<"*********************************"<<endl;
			//Reason 256 表示创建 512 表示真正的删除！！！！ 4096 表示重命名时的旧名字  8192表示重命名时的新名字：
			//注意 删除一个文件时使用8192放到新的地方，同时还有可能更改parentreferencenumber，代表移动到了新的地方，但是都会放在同一个磁盘
			//司马剪切操作居然是先删除然后再重命名
			//cout<<fileName<<' '<<UsnRecord->Reason<<' '<<(int)UsnRecord->FileReferenceNumber<<' '<<(int)UsnRecord->ParentFileReferenceNumber<<endl;
			if (flag)
			{
				if (UsnRecord->Reason == 256){
					cr.emplace_back(fileName, (int)UsnRecord->FileReferenceNumber, (int)UsnRecord->ParentFileReferenceNumber, strLen + 1, 0);
				}
				else if (UsnRecord->Reason == 512 || UsnRecord->Reason == 2147484160){
					removed[(int)UsnRecord->FileReferenceNumber]=1;
					//rm.push_back((int)UsnRecord->FileReferenceNumber);
				}
				else if (UsnRecord->Reason == 4096){
				 	//这个旧名字用处不大，不管了
				}
				else if (UsnRecord->Reason == 8192)
				{
					int tmp = renamed[(int)UsnRecord->FileReferenceNumber];
					if (!tmp)
					{
						renamed[(int)UsnRecord->FileReferenceNumber] = ++cnt;
						tmp = cnt;
						new_name.emplace_back(fileName);
						new_prnum.emplace_back((int)UsnRecord->ParentFileReferenceNumber);
					}
					else{
						new_name[tmp - 1] = fileName;
						new_prnum[tmp - 1] = (int)UsnRecord->ParentFileReferenceNumber;
					}
				}
			}
			dwRetBytes -= UsnRecord->RecordLength;
			UsnRecord = (PUSN_RECORD)((PCHAR)UsnRecord + UsnRecord->RecordLength);
		}
		rujd.StartUsn = *(USN *)&buffer;
	}
	CloseHandle(hVol);

	return ret;
}
#endif
