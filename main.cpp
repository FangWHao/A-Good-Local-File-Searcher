#include <iostream>
#include <cstdio>
#include <windows.h>
#include <cstdlib>
#include <ShlObj.h>
#include <fstream>
#include <direct.h>
#include <sstream>
#include <streambuf>
#include <map>
#include <deque>
#include <vector>
#include <time.h>
#include <thread>
#include <conio.h>
#include <mutex>
#include "USN.h"
#include "reg_match.h"
#include "file.h"
#include "screen_settings.h"
using namespace std;
#define ull unsigned long long
#define MAXN 10000000
int disk_num=0,page=1;
USN last_USN[255];//读取到的上一次运行时各个硬盘的最后一个USN编号
USN USN_ID[255]; //每一个硬盘中的USN日志编号
bool flag_rescanned[30];//标记该磁盘是否被重新扫描过
vector<int>rm_pos[30];  //被删除的文件在文件中的位置
vector<int>rm,tmp_ull;  //被删除文件的USN编号
vector<dat>data[30],tmp_dat,crt,rnm;//保存的文件所有信息，创建的文件信息和重命名的信息
vector<string>new_name;//保存一个被重命名文件的最终名字
vector<File>result;
vector<string>egg;
bool removed[MAXN],exist[MAXN];//标记是否被删除，是否已经存在
int renamed[MAXN];//被重命名的文件的USN编号->new_name中的编号
char con_buffer[10000];
int bufferlen=-1;
extern int rpp=5;//result per page
project match;
struct Page{
	int l,r;
	Page(){
		l=0,r=0;
	}
	Page(int l,int r):l(l),r(r){}
};
vector<Page>pa;
void read_egg(){
	srand(time(0));
	string tmp;
	ifstream fin("database\\egg.db");
	while(getline(fin,tmp)){
		egg.emplace_back(tmp);
	}
	fin.close();
	return;
}
void show_egg(){
	if(egg.size()==0)return;
	if(rand()<=5000){
		SetTitle(egg[rand()%egg.size()].c_str());
	}
	return;
}
void exit()
{
	printf("Press any key to exit");
	getchar();
	exit(0);
}
void admincheck()//检查是否以管理员权限运行
{
	bool flag = IsUserAnAdmin();
	if (!flag)
	{
		SetColor(0x04,0);
		printf("Fatal Error : Please run as Administrator.\n");
		SetColor(0xF,0);
		exit();
	}
}
bool check_local(){
	//检查本地文件是否存在，此处为简便只检查setting和info
	fstream _file;
	_file.open("database\\settings.db", ios::in);
	if (!_file)
		return 1;
	_file.close();
	_file.open("database\\info.db", ios::in);
	if (!_file)
		return 1;
	_file.close();
	return 0;
}

void read_settings(){//读取本地存储的设置文件

}
int check_disk(){//检查本地磁盘数量
	//可能存在的潜在bug:ntfs格式磁盘不连续时会跳过末尾的磁盘
	int tot=0;
	for(char dst[]="C";;dst[0]++){
		if(is_ntfs(dst))tot++;
		else break;
	}
	return tot;
}
USN write_MFT(const char* disk_path,const char* data_dst,vector<dat>&v){
	//输出指定磁盘的所有信息并返回该磁盘USN的ID
	USN now_USN;
	now_USN=GET_MFT(data_dst,disk_path,0,v);
	return now_USN;
}
void read_info(){
	ifstream fin("database\\info.db");
	fin>>disk_num;
	for(int i=0;i<disk_num;i++)
		fin>>USN_ID[i];
	for(int i=0;i<disk_num;i++)
		fin>>last_USN[i];
	fin.close();
}
bool check_info(){
	//检查信息是否合理
	bool flag=0;
	char disk_path[]="C",data_dst[]="database\\c.db";
	for(int i=0;i<disk_num;i++,disk_path[0]++,data_dst[9]++){
		USN now_USN=GET_MFT(data_dst,disk_path,1,tmp_dat);
		if(now_USN!=USN_ID[i]){
			//若现有的USN的ID与上一次运行时的ID不符，说明USN日志遭到了改动
			USN_ID[i]=now_USN;
			flag_rescanned[i]=1;
			SetColor(0x04,0);
			printf("Fatal Error: Change in USN Journal of %s detected. Please wait the program to re-scan the disk. This may take several minutes.\n",disk_path);
			SetColor(0xF,0);
			printf("Rescanning %s: please wait.\n",disk_path);
			write_MFT(disk_path,data_dst,data[i]);
			SetColor(0x02,0);
			printf("Successfully rescanned %s.\n",disk_path);
			SetColor(0xF,0);
		}
	}
}
bool update_info(){
	ofstream fout("database\\info.db");
	fout<<disk_num<<endl;
	for(int i=0;i<disk_num;i++)
		fout<<USN_ID[i]<<endl;
	char disk_path[]="C";
	for(int i=0;i<disk_num;i++)
		fout<<last_USN[i]<<endl;
	fout.close();
}
void first_run(){
	mkdir("database");//创建文件夹
	/****************Debug************/
	ofstream fout("database\\settings.db");
	fout.close();
	char disk_path[] ="C",data_dst[]="database\\c.db";
	disk_num=check_disk();
	for(int i=0;i<disk_num;i++,disk_path[0]++,data_dst[9]++){
		flag_rescanned[i]=1;
		USN_ID[i]=write_MFT(disk_path,data_dst,data[i]);
		last_USN[i]=GET_USN(disk_path,0,0,tmp_ull,tmp_dat,new_name,renamed);
		register int* tmp;
		tmp=new int[MAXN];
		for(int j=0;j<data[i].size();j++)
			tmp[data[i][j].rnum]=j;
		for(int j=0;j<data[i].size();j++)
			data[i][j].ppos=tmp[data[i][j].prnum];
		SetColor(0x02,0);
		printf("Successfully scanned %s:\n", disk_path);
		SetColor(0xF,0);
		delete[] tmp;
	}
}
void update_database(){
	char disk_path[]="C";
	char data_dst[] = "database\\c.db";
	for(int x=0;x<disk_num;x++,disk_path[0]++,data_dst[9]++){
		printf("Now updating disk %s, please wait.\n",disk_path);
		if(flag_rescanned[x])continue;
		crt.clear();
		last_USN[x]=GET_USN(disk_path,last_USN[x],1,rm,crt,new_name,renamed);
		register dat buffer;
		ifstream fin(data_dst, std::ios::binary);
		while(1){
			fin.read((char*)&buffer.rnum, sizeof(int));
			if(buffer.rnum==-1)break;
			fin.read((char*)&buffer.prnum, sizeof(int));
			fin.read((char*)&buffer.len, sizeof(int));
			buffer.filename=new char[buffer.len];
			fin.read(buffer.filename, sizeof(char) * buffer.len);
			if(!removed[buffer.rnum]){
				data[x].push_back(buffer);
				exist[buffer.rnum]=1;
			}
		}
		fin.close();
		cout << "read database: " << (double)clock() /CLOCKS_PER_SEC<< "s" << endl;
		for(int i=0;i<crt.size();i++){
			if(!removed[crt[i].rnum]&&!exist[crt[i].rnum]){
				data[x].push_back(crt[i]);
				exist[crt[i].rnum]=1;
			}
		}
		register int* tmp;
		tmp=new int[MAXN];
		memset(tmp,0,sizeof(tmp));
		cout << "read usn : " << (double)clock() /CLOCKS_PER_SEC<< "s" << endl;
		ofstream fout(data_dst, std::ios::binary);
		for(register int i=0;i<data[x].size();i++){
			if(renamed[data[x][i].rnum]){
				register int nlen=new_name[renamed[data[x][i].rnum]-1].length();
				delete[] data[x][i].filename;
				data[x][i].filename=new char[nlen+1];
				data[x][i].len=nlen+1;
				strcpy(data[x][i].filename,new_name[renamed[data[x][i].rnum]-1].c_str());
				*(data[x][i].filename+nlen)='\0';
			}
			fout.write((char*)&data[x][i].rnum, sizeof(int));
         	fout.write((char*)&data[x][i].prnum, sizeof(int));
         	fout.write((char*)&data[x][i].len, sizeof(int));
         	fout.write(data[x][i].filename, sizeof(char) * data[x][i].len);
			tmp[data[x][i].rnum]=i;
		}
		int end=-1;
		fout.write((char*)&end,sizeof(int));
		fout.close();
		cout << "write database : " << (double)clock() /CLOCKS_PER_SEC<< "s" << endl;
		/***建立文件索引路径***/
		for(int i=0;i<data[x].size();i++){
			data[x][i].ppos=tmp[data[x][i].prnum];
		}
		delete[] tmp;
		/***初始化***/
		rm.clear();
		crt.clear();
		new_name.clear();
		memset(removed,0,sizeof(removed));
		memset(renamed,0,sizeof(renamed));
		memset(exist,0,sizeof(exist));
		SetColor(0x02,0);
		printf("Successfully updated disk %s.\n",disk_path);
		SetColor(0xF,0);
	}
}
string get_path(int x,int pos){
	string path;
	path=data[x][pos].filename;
	register int fa=data[x][pos].ppos;
	while(1){
		if(fa==0)break;
		//cout<<data[x][fa].filename<<' '<<data[x][fa].rnum<<' '<<data[x][fa].prnum<<endl;
		path="\\"+path;
		path=data[x][fa].filename+path;
		fa=data[x][fa].ppos;
	}
	return path;
}
void print_res(const File& tmp){
	if(tmp.filesize == -1)
		SetColor(0xB,0);
	else
		SetColor(0xF,0);
	cout<<"NAME: "<<tmp.filepath;
	if(tmp.filesize!=-1)
	cout<<"\nSIZE: "<<tmp.filesize<<"KB\n";
	else cout<<'\n';
	cout<<"Creat Time             Change Time            Access Time\n";
	print_time(tmp.CreatT);
	cout<<"    ";
	print_time(tmp.WriteT);
	cout<<"    ";
	print_time(tmp.AccessT);
	cout<<"\n\n";
	SetColor(0xF,0);
}
int restart_searching=0; //是否有字符的变化
recursive_mutex mu; //线程互斥锁
bool finished=0;
bool port_searching = 0, unport_searching = 0;
int cnt=0,on_screen=0,new_result=0;//总结果数，屏幕上的结果数
int page_tot=0,page_now=1;  //总页数，当前页
void sear();
thread task01(sear);
void init_files() {
	admincheck();
	bool is_first_run=check_local();//若本地文件不存在，则判定为首次运行
	/*初始化信息*/
	//is_first_run=1;
	//cin>>is_first_run;
	if(is_first_run){
		printf("This is your first run, program needs some time to initialize.\n");
		printf("Scanning, please wait several minutes.\n");
	}
	else {//判定是否出现不合法信息，若出现则当作第一次运行处理
		read_settings();//读入设置信息
		read_info();    //读入USN64位ID和上次的最后一个USN
		if(disk_num!=check_disk()){ //磁盘数量改变属于严重错误，应重新初始化
			SetColor(0x04,0);
			cout<<"Fatal Error: Change in disk number detected.\n";
			cout<<"The program will rescan all your disks, this will take several minutes.\n";
			SetColor(0xF,0);
			is_first_run=1;
		}
		else {
			check_info(); //检查每块磁盘的USN64位ID是否改变，若改变则更新一下
		}
	}
	if(is_first_run)first_run();
	else update_database();//先分析USN记录，记录要删除的文件再在读入时标记
	update_info();
	cout << "Totle Time : " << (double)clock() /CLOCKS_PER_SEC<< "s" << endl;
	//Sleep(1000);
	cout<<"Press any key to continue\n";
	getche();
	system("cls");
	register char c;
	cout<<"Input any word to start searching\n";
	/********************************************************/
	//十分重要！首先要把搜索线程分离出去，不然会卡死！！！！！！！！

    task01.detach();              //分离线程
}

/*******************************/
//mainwindow.cpp
/*******************************/

const int INFO_MAX_SIZE = 256;
const int FILTERS_MAX_SIZE = 256;
class info {
public:
	void print();
	void add_string(string adding_str);
	void clear() { size = cur_pt = 0; }
	void setPos(int Posx, int Posy) { posx = Posx; posy = Posy; }
	int get_choice() {
		if(cur_pt <= 0)
			cur_pt = 1;
		if(cur_pt >= size)
			cur_pt = size;
		return cur_pt;
	}
	void operator += (int i) { cur_pt+=i; }
	void operator -= (int i) { cur_pt-=i; }
	int run();
	info() { size = cur_pt = posx = posy = 0; }
private:
	int size, cur_pt, posx, posy;
	string info_str[INFO_MAX_SIZE];
} window_main, window_filters;
void info::print() {
	if(size == 0) {
		puts("Error in info::print()");
		return ;
	}
	if(cur_pt <= 0)
		cur_pt = 1;
	if(cur_pt >= size)
		cur_pt = size;
	for(int i=1;i<=size;i++) {
		SetPos(posx, posy+i-1);
		if(cur_pt == i){
			SetColor(0xB,0);
			putchar('>');
			cout<<info_str[i];
			putchar('<');
			SetColor(0xF,0);
		}
		else cout<<info_str[i];
		putchar('\n');
	}
}
void info::add_string(string adding_str) {
	info_str[++size] = adding_str;
}
int info::run() {
	char ch;
	while(1) {
		system("cls");
		print();
		ch = getch();
		if(ch == '\n' || ch == '\r')
			return get_choice();
		if(ch == 80 || ch == 77)
			(*this)+=1;
		if(ch == 72 || ch == 75)
			(*this)-=1;
	}
}
struct filter {
/*
	filter_type = 1. 忽略文件夹
	filter_type = 2. 忽略文件
	filter_type = 3. 屏蔽带有filter_str后缀的文件
	filter_type = 4. 屏蔽filter_str文件目录下的所有文件
	filter_type = 5. 只搜索filter_str文件目录下的文件
*/
	int filter_type;
	string filter_str;
	filter() { filter_type = 0; filter_str.clear(); }
};
class filters_package {
public:
	filter filters[FILTERS_MAX_SIZE];
	int size;
	void addfilter(int filter_type, string filter_str) {
		if(filter_type == 5) {
			for(int i=1;i<=size;i++)
				if(filters[i].filter_type == 5) {
					filters[i].filter_str = filter_str;
					return ;
				}
		}
		filters[++size].filter_type = filter_type;
		filters[size].filter_str = filter_str;
	}
	void addfilter(int filter_type) {
		for(int i=1;i<=size;i++)
			if(filters[i].filter_type == filter_type)
				return ;
		filters[++size].filter_type = filter_type;
		filters[size].filter_str.clear();
	}
	void print() {
		puts("Current Filters:");
		if(size == 0)
			puts("None!");
		for(int i=1;i<=size;i++)
			switch(filters[i].filter_type) {
				case 1: puts("Ignore folders.\n"); break;
				case 2: puts("Ignore files.\n"); break;
				case 3: cout<<"Ignore suffix: "<<filters[i].filter_str<<"\n\n"; break;
				case 4: cout<<"Ignore path: "<<filters[i].filter_str<<"\n\n"; break;
				case 5: cout<<"Searching path: "<<filters[i].filter_str<<"\n\n"; break;
			}
		getch();
	}
	bool match(File matching_file);
	void clear() { size = 0; }
	filters_package() { size = 0; }
} filters;
int get_public_suffix_length(string x,string y) {
	int lenx = x.length(), leny = y.length();
	for(int i=1, maxi = min(lenx,leny);i<=maxi;i++)
		if(x[lenx-i] != y[leny-i])
			return i-1;
	return min(lenx,leny);
}
int get_public_suffix_length(char *x,string y) {
	int lenx = strlen(x), leny = y.length();
	for(int i=1, maxi = min(lenx,leny);i<=maxi;i++)
		if(x[lenx-i] != y[leny-i])
			return i-1;
	return min(lenx,leny);
}
int get_public_prefix_length(string x,string y) {
	int lenx = x.length(), leny = y.length();
	for(int i=1, maxi = min(lenx,leny);i<=maxi;i++)
		if(x[i-1] != y[i-1])
			return i-1;
	return min(lenx,leny);
}
bool filters_package::match(File matching_file) {
	// cout<<matching_file.filepath<<' '<<matching_file.filename<<' '<<matching_file.filesize<<endl;
	for(int i=1;i<=size;i++) {
		if(filters[i].filter_type == 1) {
			if(matching_file.filesize == -1ll)
				return false;
		}
		else if(filters[i].filter_type == 2) {
			if(matching_file.filesize != -1ll)
				return false;
		}
		else if(filters[i].filter_type == 3) {
			// cout<<"\n\n\n"<<get_public_suffix_length(matching_file.filename,filters[i].filter_str)<<' '<<filters[i].filter_str<<endl;
			if(get_public_suffix_length(matching_file.filename,filters[i].filter_str) == filters[i].filter_str.length())
				return false;
		}
		else if(filters[i].filter_type == 4) {
			// cout<<"\n\n\n"<<get_public_prefix_length(matching_file.filepath,filters[i].filter_str)<<' '<<filters[i].filter_str<<endl;
			if(get_public_prefix_length(matching_file.filepath,filters[i].filter_str) == filters[i].filter_str.length())
				return false;
		}
		else {
			if(get_public_prefix_length(matching_file.filepath,filters[i].filter_str) != filters[i].filter_str.length())
				return false;
		}
	}
	return true;
}
void sear(){
	PORT:
	while(!unport_searching);
	FINISH:
	//cout<<"FINISHED"<<endl;
	while(!restart_searching); //开始时等待信号
	RESTART:
	//cout<<"RESTART"<<endl;
	/**********初始化************/
	restart_searching=0;
	result.clear(); //清空一下现有的结果
	pa.clear();//清空现有的页码记录
	pa.emplace_back(0,0);//从1开始
	cnt=0,on_screen=0;
	page_now=1,page_tot=0;
	new_result=0;
	/***************************/
	if(!match.read_reg(con_buffer,0))
		goto FINISH;
	string disk="C:\\",filepath;
	for(int x=0;x<disk_num;x++,disk[0]++){
		for(int i=0;i<data[x].size();i++){
			if(restart_searching){  //每次判断是否有字符的增加或删除，若有，则重新搜索
				goto RESTART;
			}
			if(port_searching) {
				goto PORT;
			}
			if(new_result==rpp){ //第一次输出特殊判断一下
				new_result=0;
				page_tot++;
				pa.emplace_back(cnt-rpp,cnt);
			}
			if(match.match(data[x][i].filename)){
				File tmp(data[x][i],x,disk+get_path(x,i));
				if(!filters.match(tmp))continue;
				cnt++;
				result.push_back(tmp);
				if(on_screen<rpp)print_res(tmp);
				on_screen++;
				new_result++;
			}
		}
	}
	if(cnt!=pa[page_tot].r){ //最后一页可能有不完整的结果，特殊判断
		page_tot++;
		pa.emplace_back(pa[page_tot-1].r,cnt);
	}
	goto FINISH;//万一搜索完了就返回第一行，不能退出！
}
void start_searching() {
	port_searching = 0;
	unport_searching = 1;
	char c;
	page=1;
    system("cls");
	SetColor(0xB,0);
	cout<<"Input any word to start searching\n";
	SetColor(0xF,0);
	while(c=getch()) { //删除操作
		show_egg();
		bool is_changed=0;
        if(c==8&&bufferlen!=-1) { //删除
            con_buffer[bufferlen]='\0';
            bufferlen--;
            page=1;
            if(bufferlen>=0){ //中文类型需要特殊判断，完整删除两个ASCII
            	if(con_buffer[bufferlen]<0){
            		con_buffer[bufferlen]='\0';
           		 	bufferlen--;
            	}
            }
            if(bufferlen==-1){//删干净了
            	page=1;
            	system("cls");
				SetColor(0xB,0);
            	cout<<"Input any word to start searching\n";
				SetColor(0xF,0);
            	continue;
            }
            else {
            	page=1;
            	system("cls");
				SetColor(0x02,0);
            	cout<<con_buffer<<endl;
				SetColor(0xF,0);
            	cout<<"PAGE: "<<page<<endl;
            	restart_searching=1;
            }
        }
        else if(c==8&&bufferlen==-1){  //防止溢出
            continue;
        }
        else if(c==-32){//翻页操作
        	char direction=getch();
        	if(bufferlen==-1)continue;
        	if(direction==80||direction==77){//向下翻页
        		if(page_now==page_tot)continue; //当前是最后一页时忽略该命令
        		else{   //翻页操作
        			page_now++;
        			system("cls");
					SetColor(0x02,0);
        			cout<<con_buffer<<endl;
					SetColor(0xF,0);
        			cout<<"PAGE: "<<page_now<<endl;
        			for(int i=pa[page_now].l;i<pa[page_now].r;i++){
        				print_res(result[i]);
        			}
        			continue;
        		}
        	}
        	else if(direction==72||direction==75){ //向上翻页
        		if(page_now!=1){ //当前页不是第一页才能执行翻页操作
        			system("cls");
					SetColor(0x02,0);
        			cout<<con_buffer<<endl;
					SetColor(0xF,0);
        			page_now--;
        			cout<<"PAGE: "<<page_now<<endl;
        			for(int i=pa[page_now].l;i<pa[page_now].r;i++){
						print_res(result[i]);
					}
        		}
        		else continue; //当前是第一页时忽略翻页操作
        	}
        }
        else {  //更新关键字重新搜索
        	con_buffer[++bufferlen]=c;
        	page_now=1;
        	if(c<0){  //若当前输入的是中文，要同时读进两个字符，减少bug可能
        		c=getch();
        		con_buffer[++bufferlen]=c;
        	}
        	system("cls");
			SetColor(0x02,0);
        	cout<<con_buffer<<endl;
			SetColor(0xF,0);
        	cout<<"PAGE: "<<page_now<<endl;
        	restart_searching=1;
        }
    }
    unport_searching = 0;
	port_searching = 1;
}
void set_searching_filter() {
	int window_result;
	string tmp;
	while(1) {
		window_result = window_filters.run();
		if(window_result == 1 || window_result == 2)
			filters.addfilter(window_result);
		else if(window_result == 3 || window_result == 4 || window_result == 5) {
			puts("Please input the corresponding string..");
			cin>>tmp;
			filters.addfilter(window_result,tmp);
		}
		else if(window_result == 6)
			filters.clear();
		else
			return ;
		filters.print();
	}
}
void other_settings() {

}
void init_windows() {
	SetTitle("A GOOD LOCAL FILE SEARCHER");
	SetSize(140,34);
	HideConsoleCursor();
	// main window:
	window_main.add_string("Start Searching");
	window_main.add_string("Set Searching Fliter");
	window_main.add_string("Other Settings");
	window_main.add_string("Quit");
	window_main.setPos(60, 14);
	// filters settings window:
	window_filters.add_string("Ignore Folders");
	window_filters.add_string("Ignore Files");
	window_filters.add_string("Ignore a Specific Suffix");
	window_filters.add_string("Ignore a Specific Path");
	window_filters.add_string("Only search in a Specific Path(will cover the last filter!)");
	window_filters.add_string("Erase all");
	window_filters.add_string("Back");
	window_filters.setPos(30, 13);
}
void mainwindow() {
	read_egg();
	int window_result;
	while(1) {
		window_result = window_main.run();
		switch(window_result) {
			case 1: start_searching(); break;
			case 2: set_searching_filter(); break;
			case 3: other_settings(); break;
			default:
				return ;
		}
	}
}

signed main() {
	// string a,b;while(1)
	// cin>>a>>b,
	// cout<<get_public_suffix_length(a,b)<<' '<<get_public_prefix_length(a,b)<<endl;


	init_windows();
	init_files();
	mainwindow();
}

