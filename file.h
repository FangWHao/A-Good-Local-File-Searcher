#ifndef _FILE_H
#define _FILE_H
#include <iostream>
#include <cstdio>
#include <cstring>
#include <windows.h>
#include <windef.h>
#include <math.h>
#include <map>
#include <string>
#include <vector>
#define ll long long
typedef unsigned long long file_ref_t;
using namespace std;

bool fileh_switch_debug;

LARGE_INTEGER get_size(const char *filepath);
map<char, int> mp1, mp2, mp3, mp_empty;
map<char, int>::iterator iter;
class File;
void get_time(File &fl);
class dat
{
public:
	char *filename;		   //文件名
	int len;			   //文件名的长度+1(\0)
	file_ref_t rnum, prnum; //reference number，parent reference number
	int ppos;			   //parent在vector中的位置
	dat() : filename(NULL), len(0), rnum(0), prnum(0), ppos(0) {}
	dat(const char *s, file_ref_t r, file_ref_t pr = 0, int l = 0, int pp = 0) : filename(NULL), len(l), rnum(r), prnum(pr), ppos(pp)
	{
		if (s != NULL && l > 0)
		{
			filename = new char[l]; //深层复制
			strcpy(filename, s);
		}
	}
	dat &operator=(const dat &a)
	{
		if (this == &a)
			return *this;
		char *new_filename = NULL;
		if (a.filename != NULL && a.len > 0)
		{
			new_filename = new char[a.len];
			strcpy(new_filename, a.filename);
		}
		if (filename != NULL)
			delete[] filename;
		filename = new_filename;
		len = a.len;
		rnum = a.rnum;
		prnum = a.prnum;
		ppos = a.ppos;
		return *this;
	}
	dat(const dat &a) : filename(NULL), len(0), rnum(0), prnum(0), ppos(0)
	{
		*this = a;
	}
	~dat()
	{
		delete[] filename;
	}
	double Cosine_Similarity(const char *);
};
double dat::Cosine_Similarity(const char *s)
{
	mp1.clear();
	mp2.clear();
	mp3.clear(); //初始化清空一下
	int l1 = len;
	int l2 = strlen(s);
	if (l1 <= 1 || l2 == 0)
		return 0;
	for (int i = 0; i < l1 - 1; i++)
	{
		if (filename[i] == ' ')
			continue;
		mp1[filename[i]]++;
		mp3[filename[i]]++;
	}
	for (int i = 0; i < l2; i++)
	{
		if (s[i] == ' ')
			continue;
		mp2[s[i]]++;
		mp3[s[i]]++;
	}
	double len = 0;
	double len1 = 0, len2 = 0;
	for (iter = mp3.begin(); iter != mp3.end(); iter++)
	{
		len += mp1[iter->first] * mp2[iter->first];
		len1 += mp1[iter->first] * mp1[iter->first];
		len2 += mp2[iter->first] * mp2[iter->first];
	}
	if (len1 == 0 || len2 == 0)
		return 0;
	double div = len / sqrt(len1) / sqrt(len2);
	return div;
}
class File : public dat
{
public:
	string filepath;				  //文件的整体路径
	ll filesize;					  //文件的大小 -1代表文件夹,可当作flag使用
	FILETIME CreatT, AccessT, WriteT; //创建时间，上次访问时间，上次修改时间
	int disk;						  //所在磁盘
	File() : dat(), filepath(), filesize(-1), disk(0)
	{
		memset(&CreatT, 0, sizeof(CreatT));
		memset(&AccessT, 0, sizeof(AccessT));
		memset(&WriteT, 0, sizeof(WriteT));
	}
	File(const dat &x, int disk, string filepath) : dat(x), filepath(filepath), filesize(-1), disk(disk)
	{ //初始化，注意深层复制！！！
		filesize = get_size(filepath.c_str()).QuadPart;
		get_time(*this);
	}
	File(const File &a) : dat(a)
	{
		filepath = a.filepath;
		filesize = a.filesize;
		CreatT = a.CreatT;
		AccessT = a.AccessT;
		WriteT = a.WriteT;
		disk = a.disk;
	}
	File &operator=(const File &a)
	{
		if (this == &a)
			return *this;
		dat::operator=(a);
		filepath = a.filepath;
		filesize = a.filesize;
		CreatT = a.CreatT;
		AccessT = a.AccessT;
		WriteT = a.WriteT;
		disk = a.disk;
		return *this;
	}
};
LARGE_INTEGER get_size(const char *filepath)
{
	HANDLE hFile = CreateFileA(filepath, FILE_READ_EA, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	LARGE_INTEGER size;
	size.QuadPart = -1;
	if (hFile != INVALID_HANDLE_VALUE)
	{
		GetFileSizeEx(hFile, &size);
		CloseHandle(hFile);
		return size;
	}
	return size;
}
void get_time(File &fl)
{
	HANDLE hFile = CreateFileA(fl.filepath.c_str(),
							  GENERIC_READ | GENERIC_WRITE,
							  FILE_SHARE_READ | FILE_SHARE_DELETE,
							  NULL,
							  OPEN_EXISTING,
							  FILE_FLAG_BACKUP_SEMANTICS,
							  NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		memset(&fl.CreatT, 0, sizeof(fl.CreatT));
		memset(&fl.AccessT, 0, sizeof(fl.AccessT));
		memset(&fl.WriteT, 0, sizeof(fl.WriteT));
		return;
	}
	GetFileTime(hFile, &fl.CreatT, &fl.AccessT, &fl.WriteT);
	CloseHandle(hFile);
}
void print_time(FILETIME ftime)
{
	char str[50];
	SYSTEMTIME rtime;
	FILETIME ltime;
	memset(str, 0, 50);
	FileTimeToLocalFileTime(&ftime, &ltime);
	FileTimeToSystemTime(&ltime, &rtime); //将文件时间转化为系统时间
	//cout<<rtime.wYear<<' '<<rtime.wMonth;
	sprintf(str, "%04u-%02u-%02u %02u:%02u:%02u", rtime.wYear, rtime.wMonth, rtime.wDay, rtime.wHour, rtime.wMinute, rtime.wSecond);
	printf("%s", str);
}
string GetClipboard()
{
	string ret;
	if (::OpenClipboard(NULL))
	{ //获得剪贴板数据
		HGLOBAL hMem = GetClipboardData(CF_TEXT);
		if (NULL != hMem)
		{
			char *lpStr = (char *)::GlobalLock(hMem);
			if (NULL != lpStr)
			{
				ret = lpStr;
				::GlobalUnlock(hMem);
			}
		}
		::CloseClipboard();
	}
	return ret;
}
bool Time_cmp(const FILETIME a, const FILETIME b)
{ // <
	if (a.dwHighDateTime == b.dwHighDateTime)
		return a.dwLowDateTime < b.dwLowDateTime;
	else
		return a.dwHighDateTime < b.dwHighDateTime;
}
bool size_cmp_l(File a, File b)
{ //由大到小
	if (a.filesize == b.filesize)
		return a.filename > b.filename;
	return a.filesize > b.filesize;
}
bool size_cmp_s(File a, File b)
{ //由小到大
	if (a.filesize == b.filesize)
		return a.filename < b.filename;
	return a.filesize < b.filesize;
}
bool CreatT_cmp_l(File a, File b)
{
	return !Time_cmp(a.CreatT, b.CreatT);
}
bool CreatT_cmp_s(File a, File b)
{
	return Time_cmp(a.CreatT, b.CreatT);
}
bool AccessT_cmp_l(File a, File b)
{
	return !Time_cmp(a.AccessT, b.AccessT);
}
bool AccessT_cmp_s(File a, File b)
{
	return Time_cmp(a.AccessT, b.AccessT);
}
bool WriteT_cmp_l(File a, File b)
{
	return !Time_cmp(a.WriteT, b.WriteT);
}
bool WriteT_cmp_s(File a, File b)
{
	return Time_cmp(a.WriteT, b.WriteT);
}
bool Name_cmp_s(File a, File b) {
	return strcmp(a.filename, b.filename) < 0;
}
void merge_sort1(int l, int r, bool cmp(File, File), vector<File> &vec);
File *tmp;
void merge_sort(vector<File> &vec, bool cmp(File, File))
{
	if (vec.empty())
		return;
	//cout<<"Fuck"<<endl;
	tmp = new File[vec.size()];
	//cout<<"Holy 1"<<endl;
	merge_sort1(0, vec.size() - 1, cmp, vec);
	//cout<<"shit"<<endl;
	delete[] tmp;
}
void merge_sort1(int l, int r, bool cmp(File, File), vector<File> &vec)
{
	//cout<<l<<' '<<r<<endl;
	//getchar();
	if (l == r)
	{
		//cout<<"Holy 2"<<endl;
		tmp[l] = vec[l];
		return;
	}
	int m = (l + r) >> 1;
	merge_sort1(l, m, cmp, vec);
	merge_sort1(m + 1, r, cmp, vec);
	int lp = l, rp = m + 1, p = l;
	while (lp <= m && rp <= r)
	{
		if (cmp(tmp[lp], tmp[rp]))
		{
			//cout<<"Holy 3"<<endl;
			vec[p++] = tmp[lp++];
		}
		else //cout<<"Holy 4"<<endl,
			vec[p++] = tmp[rp++];
	}
	while (lp <= m) //cout<<"Holy 5"<<endl,
		vec[p++] = tmp[lp++];
	while (rp <= r) //cout<<"Holy 6"<<endl,
		vec[p++] = tmp[rp++];
	for (int i = l; i <= r; i++) //cout<<"Holy 7"<<endl,
		tmp[i] = vec[i];
}
// void merge_sort(vector<File> &vec, bool cmp(File,File)) {
// 				puts("Errr1!");
//     int len = vec.size();
// 	File *arr = new File[len];
// 	for(int i=0;i<len;i++)
// 		arr[i] = vec[i];
// 				puts("Errr2!");
// 	File *a = arr;
//     File *b = new File[len];
//     for (int seg = 1; seg < len; seg += seg) {
//         for (int start = 0; start < len; start += seg + seg) {
//             int low = start, mid = min(start + seg, len), high = min(start + seg + seg, len);
//             int k = low;
//             int start1 = low, end1 = mid;
//             int start2 = mid, end2 = high;
//             while (start1 < end1 && start2 < end2)
//                 b[k++] = cmp(a[start1], a[start2]) ? a[start1++] : a[start2++];
//             while (start1 < end1)
//                 b[k++] = a[start1++];
//             while (start2 < end2)
//                 b[k++] = a[start2++];
//         }
//         File *temp = a;
//         a = b;
//         b = temp;
//     }
// 			puts("Errr3!");
//     if (a != arr) {
//         for (int i = 0; i < len; i++)
//             b[i] = a[i];
//         b = a;
//     }
// 	for(int i=0;i<len;i++)
// 		vec[i] = arr[i];
// 			puts("Errr4!");
//     delete[] b;
// 	delete[] arr;
// 			puts("Errr5!");
// }
#endif
