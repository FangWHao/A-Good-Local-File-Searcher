#ifndef _FILE_H
#define _FILE_H
#include<iostream>
#include<cstdio>
#include<cstring>
#include<windows.h>
#include<windef.h>
#include"USN.h"
#define ll long long
using namespace std;
LARGE_INTEGER get_size(const char* filepath);
class File;
void get_time(File& fl);
class dat{
public:
	char* filename;  //文件名
	int len; //文件名的长度+1(\0)
	int rnum, prnum,ppos; //reference number，parent reference number，parent在vector中的位置
	dat(){}
	dat(char* s,int r,int pr=0,int l=0,int pp=0):rnum(r),prnum(pr),len(l),ppos(pp){
		filename=new char[l]; //深层复制
		strcpy(filename,s);
	}
};
class File:public dat{
public:
	string filepath; //文件的整体路径
	ll filesize;//文件的大小 -1代表文件夹,可当作flag使用
	FILETIME CreatT,AccessT,WriteT;//创建时间，上次访问时间，上次修改时间
	int disk;//所在磁盘
	File(){}
	File(dat x,int disk,string filepath):disk(disk),filepath(filepath){ //初始化，注意深层复制！！！
		filename=new char[x.len];
		strcpy(filename,x.filename);
		len=x.len;
		rnum=x.rnum,prnum=x.prnum,ppos=x.ppos;
		filesize=get_size(filepath.c_str()).QuadPart;
		get_time(*this);
	}
	~File(){
		delete[] filename;
	}
};
LARGE_INTEGER get_size(const char* filepath){
	HANDLE hFile=CreateFileA(filepath,FILE_READ_EA,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
	LARGE_INTEGER size;
	size.QuadPart=-1;
	if(hFile!=INVALID_HANDLE_VALUE){
		GetFileSizeEx(hFile,&size);
		CloseHandle(hFile);
		return size;
	}
	return size;
}
void get_time(File& fl){
	HANDLE hFile = CreateFile(fl.filepath.c_str(),
								GENERIC_READ|GENERIC_WRITE,
								FILE_SHARE_READ | FILE_SHARE_DELETE,
								NULL,
								OPEN_EXISTING,
								FILE_FLAG_BACKUP_SEMANTICS,
								NULL);
	GetFileTime(hFile,&fl.CreatT,&fl.AccessT,&fl.WriteT);
	CloseHandle(hFile);
}
void print_time(FILETIME ftime){
    char str[50];
    SYSTEMTIME rtime;
    FILETIME  ltime;
    memset(str,0,50);
    FileTimeToLocalFileTime(&ftime,&ltime);
    FileTimeToSystemTime(&ltime,&rtime);        //将文件时间转化为系统时间
    sprintf(str, "%04u-%02u-%02u %02u:%02u:%02u",rtime.wYear, rtime.wMonth, rtime.wDay, rtime.wHour, rtime.wMinute, rtime.wSecond);
    printf("%s",str);
}
bool Time_cmp(const FILETIME a,const FILETIME b){ // <
	if(a.dwHighDateTime==b.dwHighDateTime)return a.dwLowDateTime<b.dwLowDateTime;
	else return a.dwHighDateTime<b.dwHighDateTime;
}
bool size_cmp_l(File a,File b){ //由大到小
	if(a.filesize==b.filesize)return a.filename>b.filename;
	return a.filesize>b.filesize;
}
bool size_cmp_s(File a,File b){ //由小到大
	return a.filesize<b.filesize;
}
bool CreatT_cmp_l(File a,File b){
	return !Time_cmp(a.CreatT,b.CreatT);
}
bool CreatT_cmp_s(File a,File b){
	return Time_cmp(a.CreatT,b.CreatT);
}
bool AccessT_cmp_l(File a,File b){
	return !Time_cmp(a.AccessT,b.AccessT);
}
bool AccessT_cmp_s(File a,File b){
	return Time_cmp(a.AccessT,b.AccessT);
}
bool WriteT_cmp_l(File a,File b){
	return !Time_cmp(a.WriteT,b.WriteT);
}
bool WriteT_cmp_s(File a,File b){
	return Time_cmp(a.WriteT,b.WriteT);
}
#endif
