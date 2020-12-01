/* 
使用class project建立一个正则表达式匹配工程； 
可调用句柄： 
void project::read_reg(int reg_max_size, bool switch_debug)
bool project::read_reg(char *reg, bool switch_debug)
	功能：载入一个正则表达式，不传参数则由控制台输入，传参数则返回正则表达式是否合法。 
bool project::match(char *str)
	功能：字符串匹配，str串是否可以与正则表达式匹配。必须保证已经载入了正则表达式。 
void clear()
	功能：清空正则表达式，将project复原为原状态。 
*/
#include <iostream>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <map>
#include <set>
#include <queue>

using namespace std;

const int NFA_MAX_SIZE = 1002;
const int NFA_MAX_LSTSIZE = 21002;
const int MAX_CHARMAP_SIZE = 1005;
const int CHARPOOL_MAX_SIZE = 515;
const int DFA_MAX_SIZE = 1002;
const int MAX_REG_LENGTH = 1005; //正则表达式长度最长限制！ 

class NFA {
	public:
		NFA() {
			size = 1;
			cnt = 0;
			for(int i=0;i<NFA_MAX_SIZE;i++)
				head[i].clear();
			memset(pos,0,sizeof(pos));
			memset(nxt,0,sizeof(nxt));
			charpool.clear();
		}
		bool read_reg(char *reg, int L, int R, int start_point, int dest_point, bool is_birth) ;
		bool read_reg(char *reg, int L, int R, int start_point, int dest_point) ;
		void addedge(int stpoint, int destpoint, char dir) ;
		void addedge_without_adding_in_charpool(int stpoint, int destpoint, char dir);
		void debug() ;
		void clear() ;
		int size; //默认起始点st为节点0,dest节点为1 
		map<char,int> head[NFA_MAX_SIZE];  //link[i][A] = j : 状态i读入字符A后进入状态j，定义空包为'\0',else包为'\1' 
		//else包的定义：遇到了charpool外的任何字符时，走这条边
		int pos[NFA_MAX_LSTSIZE], nxt[NFA_MAX_LSTSIZE], cnt; //邻接表 
		set<char> charpool; //储存所有字符，特别地，'\1'属于charpool当且仅当存在排除运算 
};
class DFA {
	public:
		DFA() {
			size = 0;
			for(int i=0;i<DFA_MAX_SIZE;i++)
				state[i].clear();
			charpool.clear();
			memset(cancelled,0,sizeof(cancelled));
			memset(is_end,0,sizeof(is_end));
			memset(lnk,0,sizeof(lnk));
		}
		bool match(char *str);
		void transform_NFA_to_DFA(NFA &bas_nfa);
		void update() ;
		void debug() ; 
		void clear() ;
		int size; //默认起始点st=1,终结点dest=size 
		set<int> state[DFA_MAX_SIZE]; //每个节点代表的状态 
		int lnk[DFA_MAX_SIZE][CHARPOOL_MAX_SIZE]; //储存DFA的边 
		set<char> charpool; //储存所有字符，特别地，'\1'属于charpool当且仅当存在排除运算 
		bool cancelled[DFA_MAX_SIZE]; //简化DFA时：是否删点 
		bool is_end[DFA_MAX_SIZE]; //是否为接收态 
};

class project: private NFA, private DFA { //整个工程使用一个class，可调用正则表达式读入和匹配两个函数 
public: 
	bool read_reg(char *str, bool switch_debug) ;
	void read_reg(int reg_max_size, bool switch_debug) ;
	bool match(char *str) ;
	void clear() ;
};

void read_string(char *str);
bool check_reg(char *reg);

void NFA::clear() {
	size = 1;
	cnt = 0;
	for(int i=0;i<NFA_MAX_SIZE;i++)
		head[i].clear();
	memset(pos,0,sizeof(pos));
	memset(nxt,0,sizeof(nxt));
	charpool.clear();
}
void DFA::clear() {
	size = 0;
	for(int i=0;i<DFA_MAX_SIZE;i++)
		state[i].clear();
	charpool.clear();
	memset(cancelled,0,sizeof(cancelled));
	memset(is_end,0,sizeof(is_end));
	memset(lnk,0,sizeof(lnk));
}
void project::clear() {
	NFA::clear();
	DFA::clear(); 
}

void NFA::debug() {
	printf("====debug:NFA====\n  size = %d, cnt = %d\n",size,cnt);
	map<char,int>::iterator iter; char ch;
	set<char>::iterator charpool_iter;
	puts("Print the charmap..");
	for(charpool_iter = charpool.begin(); charpool_iter!= charpool.end(); charpool_iter++)
		printf("%d ",(int)*charpool_iter);
	putchar('\n');
	
	for(int i=0,j;i<=size;i++) {
		printf("  point %d: \n",i);
		if(head[i].empty())
			puts("    empty");
		else
			for(iter = head[i].begin();iter != head[i].end();iter++) {
				ch = iter->first;
				if(ch == '\0')
					printf("    [null]  ");
				else if(ch == '\1')
					printf("    [else]  ");
				else
					printf("    %d :  ",(int)ch);
				for(j=iter->second;j!=0;j=nxt[j])
					printf("%d ",pos[j]);
				putchar('\n');
			}
	}
	
}
void NFA::addedge(int stpoint, int destpoint, char dir) {
//由于NFA的每个边属性可能对应多个边，使用邻接表存储 
	cnt++;
	pos[cnt] = destpoint;
	nxt[cnt] = head[stpoint][dir];
	head[stpoint][dir] = cnt;
	if(dir != '\0'  &&  dir != '\1')
		charpool.insert(dir);
}
void NFA::addedge_without_adding_in_charpool(int stpoint, int destpoint, char dir) {
	cnt++;
	pos[cnt] = destpoint;
	nxt[cnt] = head[stpoint][dir];
	head[stpoint][dir] = cnt;
}
int get_next_pos(char *reg,int st) { //寻找与某左括号匹配的右括号，如果不是左括号则返回原位置 
	if(reg[st] != '(' || reg[st-1] == '\\')
		return st;
	int tmp = -1;
	while(tmp != 0) {
		st++;
		if(reg[st] == '(' && reg[st-1] != '\\')
			tmp--;
		else if(reg[st] == ')' && reg[st-1] != '\\')
			tmp++;
	}
	return st;
}
struct split_result {
	int mode; //0 - 无运算  1 - 连接  2 - 联合  3 - Kleene包  4 - 排除符号（未完待续） 
	int leftL,leftR, rightL,rightR;
	void maintain(char *reg) {
		while(reg[leftL] == '(' && get_next_pos(reg,leftL) == leftR)
			leftL++, leftR--;
		while(reg[rightL] == '(' && get_next_pos(reg,rightL) == rightR)
			rightL++, rightR--;
	}
	split_result() {
		mode = 0;
	}
	split_result(int _mode, int L,int R) {
		mode = _mode;
		leftL = L, leftR = R;
		rightL = rightR = -1;
	}
	split_result(int _mode, int lL,int lR, int rL,int rR) {
		mode = _mode;
		leftL = lL; rightL = rL;
		leftR = lR; rightR = rR;
	}
	void debug() {
		printf("mode = %d \n ret1 = %d - %d , \n ret2 = %d - %d",mode,leftL,leftR,rightL,rightR);
	}
};
split_result split(char *reg,int L,int R) {
	int tmp, cnt=0;
	if(reg[L] == '^' && reg[L-1] != '\\') //判断是否为排除运算：可以生效的排除运算符必然在L上 
		return split_result(4,L+1,R);
	int lkloc=-1, unloc=-1, klloc=-1;
	for(int i=L;i<=R;i++) {
		i = get_next_pos(reg,i); //将i前调至匹配右括号位置上（正好落在右括号上）或不改变i（当reg[i]不为真左括号） 
		if(reg[i] == '\\') //把一个字符记录一次 
			i++;
		cnt++; //找到一个元，计数 
		if(i>=R)
			break;
		if(reg[i+1] == '*' && reg[i] != '\\') { //找到第一个Kleene包 
			i++;
			if(klloc == -1)
				klloc = i;
		} //这里不再直接else if: 存在a*b类串 
		if(i>=R)
			break;
		if(reg[i+1] == '|' && reg[i] != '\\') { //找到第一个联合 
			i++; //保证再加一后跳过该符号 
			if(unloc == -1)
				unloc = i; //unloc位置在 | 符号上 
		}
		else { //找到第一个连接 
			if(lkloc == -1) {
				lkloc = i;
			}
		}
	}
//	cout<<lkloc<<' '<<unloc<<' '<<klloc<<endl;
//	cout<<L<<' '<<R<<endl;

	split_result ret;
	if(lkloc != -1) { //连接优先级高 
		//L ~ loc, loc+1 ~ R
		ret = split_result(1,L,lkloc,lkloc+1,R);
		ret.maintain(reg);
		return ret;
	}
	if(unloc != -1) { //联合优先级次中 
		//L ~ loc-1, loc+1 ~ R
		ret = split_result(2,L,unloc-1,unloc+1,R);
		ret.maintain(reg);
		return ret;
	}
	if(klloc != -1) { //Kleene包优先级最低 
		//L ~ loc-1
		ret = split_result(3,L,klloc-1);
		ret.maintain(reg);
		return ret;
	}
//	cout<<"ssssss"<<endl; 
	ret = split_result(0,L,R);
	ret.maintain(reg);
	return ret;
	//无运算 
}
struct hidden_edge_node {
	int left_bound, right_bound; //存储排除的字符：position in reg[]
	int u,v; //u->v连边 
};
queue<hidden_edge_node> hidden_edge;
bool NFA::read_reg (char *reg, int L, int R, int start_point, int dest_point, bool is_birth) {
	if(!check_reg(reg)) {
		return false;
	}
//此时认可reg串已经通过了拼写检测 
	if(is_birth) //初始化 
		while(!hidden_edge.empty())
			hidden_edge.pop();
	if(is_birth && reg[R] == '$' && reg[R-1] != '\\') //检测下标 
		R--;
	else if(is_birth) {
		int klpt1 = ++size, klpt2 = ++size, newstpt = ++size;
		
		hidden_edge_node current_edge;
		current_edge.u = klpt1;
		current_edge.v = klpt2;
		current_edge.left_bound = 2;
		current_edge.right_bound = 1;
		hidden_edge.push(current_edge);
		
		addedge(start_point, klpt1,'\0');
		addedge(klpt2, newstpt, '\0');
		addedge(start_point, newstpt, '\0');
		addedge(klpt2, klpt1, '\0');
		start_point = newstpt;
	}
	if(is_birth && reg[L] == '^') //检测上标 
		L++;
	else if(is_birth) {
		int klpt1 = ++size, klpt2 = ++size, newdespt = ++size;
		
		hidden_edge_node current_edge;
		current_edge.u = klpt1;
		current_edge.v = klpt2;
		current_edge.left_bound = 2;
		current_edge.right_bound = 1;
		hidden_edge.push(current_edge);
		
		addedge(newdespt, klpt1,'\0');
		addedge(klpt2, dest_point, '\0');
		addedge(newdespt, dest_point, '\0');
		addedge(klpt2, klpt1, '\0');
		dest_point = newdespt;
	}
	while(reg[L] == '(' && R == get_next_pos(reg,L)) //去括号 
		L++, R--;
	//from start_point to dest_point : [reg] route 
	split_result ret = split(reg,L,R);
	if(ret.mode == 0) { //单字符，开始连边 
		if(ret.leftL != ret.leftR && (ret.leftL != ret.leftR-1 || reg[ret.leftL] != '\\')) {
			puts("Error 1 in read_reg!!"); //多字符出现 
			exit(100);
		}
		addedge(start_point,dest_point,reg[ret.leftR]);
	}
	else if(ret.mode == 1) {
		int mid = ++size;
		// start_point --left--> mid, mid --right--> dest_point
		read_reg(reg, ret.leftL,ret.leftR, start_point,mid, false);
		read_reg(reg, ret.rightL,ret.rightR, mid,dest_point, false);
	}
	else if(ret.mode == 2) {
		int unpt1=++size, unpt2=++size, unpt3=++size, unpt4=++size;
		read_reg(reg, ret.leftL,ret.leftR, unpt1,unpt2, false);
		read_reg(reg, ret.rightL,ret.rightR, unpt3,unpt4, false);
		addedge(start_point,unpt1,'\0');
		addedge(start_point,unpt3,'\0');
		addedge(unpt2,dest_point,'\0');
		addedge(unpt4,dest_point,'\0');
	}
	else if(ret.mode == 3) {
		int klpt1 = ++size, klpt2 = ++size;
		read_reg(reg, ret.leftL,ret.leftR, klpt1, klpt2, false);
		addedge(start_point, klpt1,'\0');
		addedge(klpt2, dest_point, '\0');
		addedge(start_point, dest_point, '\0');
		addedge(klpt2, klpt1, '\0');
	}
	else if(ret.mode == 4) { //排除运算，压入隐边队列，并添加所有排除字符进入charpool集 
		/********///  最新完成部分，不保证正确 
		hidden_edge_node current_edge;
		current_edge.u = start_point;
		current_edge.v = dest_point;
		current_edge.left_bound = ret.leftL;
		current_edge.right_bound = ret.leftR;
		hidden_edge.push(current_edge);
		for(int i=ret.leftL;i<=ret.leftR;i++)
			charpool.insert(reg[i]);
		
		printf(" hidden edge signed:  %d %d %d %d\n", start_point, dest_point, ret.leftL, ret.leftR);
	}
	else {
		puts("ERROR in NFA::read_reg : invalid split return mode!");
	}
	/********///  最新完成部分，不保证正确 
	if(is_birth) { //连接隐边，清空队列 
		hidden_edge_node current_edge;
		set<char>::iterator charpool_iter;
		bool hidden_edge_flag, add_else_pack_flag = false;
		while(!hidden_edge.empty()) {
			add_else_pack_flag = true;
			current_edge = hidden_edge.front();
			hidden_edge.pop();
			for(charpool_iter = charpool.begin(); charpool_iter != charpool.end(); charpool_iter++) {
				hidden_edge_flag = true;
				for(int i=current_edge.left_bound; i<=current_edge.right_bound; i++) {
					if(*charpool_iter == reg[i])
						hidden_edge_flag = false;
				}
				if(hidden_edge_flag) { //表示*charpool_iter不为被排除项，可以连边 
					addedge(current_edge.u, current_edge.v, *charpool_iter);
				}
			}
			addedge_without_adding_in_charpool(current_edge.u, current_edge.v, '\1'); //连else边 
		}
		if(add_else_pack_flag)
			charpool.insert('\1');
	}
	return true;
}
bool NFA::read_reg (char *reg, int L, int R, int start_point, int dest_point) {
	return read_reg(reg,L,R, start_point, dest_point, true);
}

int get_next_pos_with_check(char *reg, int st, int tail) { 
//寻找与某左括号匹配的右括号，如果不是左括号则返回原位置 
	if(reg[st] != '(' || reg[st-1] == '\\')
		return st;
	int tmp = -1;
	while(tmp != 0) {
		st++;
		if(st > tail)
			return -1;
		if(reg[st] == '(' && reg[st-1] != '\\')
			tmp--;
		else if(reg[st] == ')' && reg[st-1] != '\\')
			tmp++;
	}
	return st;
}
bool check_reg(char *reg) { //拼写检查主函数 
	if(strlen(reg) >= MAX_REG_LENGTH) { //表达式过长 
		puts("The regular expression string is too long. Try to input a string with length less than 1000!");
		return false;
	}
	int len = strlen(reg), tmp;
	if(reg[len-1] == '\\') {
		puts("Found char \\ at the end!"); //段尾反斜杠 
		return false;
	}
	for(int i=0;i<=len-1;i++) {
		if(i>0 && reg[i] == '\\' && reg[i-1] == '\\') {
			puts("Found two adjacent \\s in the string!");
			return false;
		}
		if(i<len-1 && reg[i] == '$' && (i==0 || reg[i-1] != '\\')) {
			puts("Found $ in the string but not at the end!");
			return false;
		}
		if(reg[i] == '*' && (i==0 || ((reg[i-1] == '|' || reg[i-1] == '(' || reg[i-1] == '*') 
		&& (i-1==0 || reg[i-2] != '\\')))) {
			puts("The position of * is wrong!");
			return false;
		}
		if(reg[i] == '|' && ((i==0 || ((reg[i-1] == '|' || reg[i-1] == '(') && (i-1==0 || reg[i-2] != '\\'))) 
		|| (i==len-1 && reg[i+1] == ')'))) {
			puts("The position of | is wrong!");
			return false;
		}
	}
	static bool reg_clear[MAX_REG_LENGTH];
	memset(reg_clear,0,sizeof(reg_clear));
	for(int i=0;i<=len-1;i++) {
		if(reg[i] == ')' && !reg_clear[i] && (i==0 || reg[i-1] != '\\')) {
			puts("Brackets not matching #1!");
			return false;
		}
		if(reg[i] == '(' && (i==0 || reg[i-1] != '\\')) {
			if(reg[i+1] == ')') {
				puts("Exist empty brackets!");
				return false;
			}
			tmp = get_next_pos_with_check(reg,i,len-1);
			if(tmp == -1) {
				puts("Brackets not matching #2!");
				return false;
			}
			reg_clear[tmp] = true;
		}
	}
	if(reg[0] == '|' || (reg[len-1] == '|' && reg[len-2] != '\\')) {
		puts("Found | at the beginning or at the end!");
		return false;
	}
	for(int i=1;i<len;i++) {
		if(reg[i] == '^' && (i==0 || reg[i-1] != '\\')) {
			if(reg[i-1] != '(' || (i-2 >= 0 && reg[i-2] == '\\')) {
				puts("Operator ^ isn't after a (!");
				return false;
			}
			for(int j=i+1;j<len;j++) {
				if(reg[j] == ')' && reg[j-1] != '\\')
					break;
				if(reg[j] == '(' && reg[j-1] != '\\') {
					puts("Found another pair of brackets in function exclude!");
					return false;
				}
				if((reg[j-1] != '\\') && (reg[j] == '^' || reg[j] == '*' || reg[j] == '|')) {
					puts("Found other functions in function exclude!");
					return false;
				}
			}
		}
	}
	return true;
}
void read_string(char *str) { //输入一个字符串，忽略空格 
	char c = getchar(); int cnt=0;
	while(c == '\n' || c == '\r')
		c = getchar();
	while(c != '\n' && c != '\r') 
		str[cnt++] = c, c = getchar();
	str[cnt] = '\0';
}

bool DFA::match (char *str) {
	int len = strlen(str), current = 1;
	if(is_end[current]) //DFA是空的 
		return true;
	for(int i=0;i<len;i++) {
		if(lnk[current][str[i]] == 0) //如果符号未定义则走else包 
			current = lnk[current][('\1')];
		else
			current = lnk[current][str[i]]; //沿边行进 
	}
	return is_end[current]; //必须在最后时刻成功跑掉 
}
bool project::match(char *str) {
	return DFA::match(str);
}

void DFA::debug () {
	set<int>::iterator iter;
	set<char>::iterator charpool_iter;
	printf("======debug: DFA======\n  size = %d\n\n",size);
	for(int i=1;i<=size;i++) {
		printf("    state : %d\n      {",i);
		for(iter = state[i].begin();iter!=state[i].end();iter++) {
			printf("%d,",*iter);
		}
		puts("}\n      link:");
		for(charpool_iter = charpool.begin();charpool_iter != charpool.end(); charpool_iter++) {
			if(*charpool_iter == '\0')
				printf("      [null] ");
			else if(*charpool_iter == '\1')
				printf("      [else] ");
			else
				printf("      %d ",(int)*charpool_iter);
			printf(" %d\n",lnk[i][*charpool_iter]);
		}
		if(cancelled[i])
			puts("Caution: This State is cancelled..");
		if(is_end[i])
			puts("This point can be an end.");
		putchar('\n');
	}
	putchar('\n');
}

/*
简化DFA函数 
功能：
	拆除所有的空包节点 
*/ 
void DFA::update () {  
	set<char>::iterator charpool_iter;
	memset(cancelled,0,sizeof(cancelled));
	for(int i=1;i<=size;i++) 
		is_end[i] = (state[i].find(1) != state[i].end()); // is_end[i] = true: i状态为接收态 
	bool flag;
	for(int i=1;i<=size;i++) {
		if(is_end[i])
			continue;
		for(int j=i+1;j<=size;j++) {
			if(is_end[j])
				continue;
			flag = true;
			for(charpool_iter = charpool.begin();charpool_iter != charpool.end();charpool_iter++) {  
				if(lnk[i][*charpool_iter] != lnk[j][*charpool_iter]) //判断两个节点可否合并：是否拥有完全相同的出边 
					flag = false;
			}
			if(flag) { //删除 
				cancelled[j] = true; //打上删除标记 
				for(int _i=1;_i<=size;_i++)
					for(charpool_iter = charpool.begin();charpool_iter != charpool.end();charpool_iter++) 
						if(lnk[_i][*charpool_iter] == j) //把所有的入边合并到同一节点 
							lnk[_i][*charpool_iter] = i;
			}
		}
	}
}
/*
以下为NFA转DFA函数群 
【基本算法】
对于非确定自动机S，依次进行下列操作： 
	1) 从S起始节点0处寻找e(ε的替代符号，下同)闭包，将选定点集定义为生成的DFA自动机T的起始状态，将此状态入队Q 
	2) 取出Q队首的状态设为cur。 
	3) 将cur对一个可能的字符a进行move(a)操作，得到若干个状态new 
	4) 如果T中没有状态new，则将T中加入状态new，新状态new入队Q。若new为空或T中已有状态new则不进行处理 
	5) 连边：T的cur状态向new状态连接属性为a的边。特别地，若new为空则向起始状态连边。 
	6) 对一个状态cur，对每个可能有边（即在charpool中）的字符执行一遍操作(3)-(5)。 
	7) 重复操作(2)-(6)直至Q为空。
	8) 设置T中的每一个包含终止节点的状态为接受态。 
*/ 
set<int> get_closure(NFA &bas_nfa, set<int> &bas_set, char key0) { //寻找bas_set的key0闭包的函数（e闭包为'\0'闭包） 
	set<int> ret;
	set<int>::iterator iter;
	pair<set<int>::iterator,bool> getret;
	queue<int> bfsq;
	int tmp, u;
	for(iter = bas_set.begin();iter != bas_set.end(); iter++) {
//		cout<<"sssdfg  "<<*iter<<endl;
		if(ret.find(*iter) != ret.end()) {
			continue;
		}
		while(!bfsq.empty()) //queue居然没有clear()...气抖冷 
			bfsq.pop();
		bfsq.push(*iter);
		while(!bfsq.empty()) {
			tmp = bfsq.front();
			bfsq.pop();
			getret = ret.insert(tmp);
			if(getret.second == false)
				continue;
			for(u=bas_nfa.head[tmp][key0];u!=0;u=bas_nfa.nxt[u])
				bfsq.push(bas_nfa.pos[u]);
		}
	}
	return ret;
}
set<int> get_e_closure(NFA &bas_nfa, set<int> &bas_set) { //方便调用 
	return get_closure(bas_nfa, bas_set,'\0');
}
set<int> get_closure(NFA &bas_nfa, set<int> &bas_set, char key0, char key1) { //寻找双字符闭包的函数（未使用） 
	set<int> ret;
	set<int>::iterator iter;
	pair<set<int>::iterator,bool> getret;
	queue<int> bfsq;
	int tmp, u;
	for(iter = bas_set.begin();iter != bas_set.end(); iter++) {
//		cout<<"sssdfg  "<<*iter<<endl;
		if(ret.find(*iter) != ret.end()) {
			continue;
		}
		while(!bfsq.empty()) //queue居然没有clear()...气抖冷 
			bfsq.pop();
		bfsq.push(*iter);
		while(!bfsq.empty()) {
			tmp = bfsq.front();
			bfsq.pop();
			getret = ret.insert(tmp);
			if(getret.second == false)
				continue;
			for(u=bas_nfa.head[tmp][key0];u!=0;u=bas_nfa.nxt[u])
				bfsq.push(bas_nfa.pos[u]);
			for(u=bas_nfa.head[tmp][key1];u!=0;u=bas_nfa.nxt[u])
				bfsq.push(bas_nfa.pos[u]);
		}
	}
	return ret;
}
set<int> move(NFA &bas_nfa, set<int> &bas_set, char keyc) { 
//move(keyc)函数：得到bas_set点集的所有点跑一次keyc边之后的可能目的地形成的点集  
	set<int> ret;
	set<int>::iterator iter;
	int u;
	for(iter = bas_set.begin(); iter != bas_set.end(); iter++) { //集体入队 
		for(u=bas_nfa.head[*iter][keyc];u!=0;u=bas_nfa.nxt[u])
			ret.insert(bas_nfa.pos[u]);
	}
	return get_closure(bas_nfa, ret, '\0');
}

bool compset(set<int> x,set<int> y) { //判断两个set是否相等 
	if(x.size() != y.size())
		return false;
	set<int>::iterator iter;
	for(iter = x.begin();iter != x.end();iter++)
		if(y.find(*iter) == y.end())
			return false;
	return true;
}

void DFA::transform_NFA_to_DFA(NFA &bas_nfa) { //转换函数 
	size = 0;
	set<int> tmpset;
	set<char>::iterator charpool_iter;
	int tmp, reppos;
	queue<int> q;
	tmpset.insert(0);
	state[++size] = get_closure(bas_nfa, tmpset, '\0'); //将起始状态保存到DFA的state状态池中 
	q.push(size); //起始状态的编号入队 
	while(!q.empty()) {
		tmp = q.front();//本次处理的状态为state[tmp] 
		q.pop();
		for(charpool_iter = bas_nfa.charpool.begin(); charpool_iter != bas_nfa.charpool.end(); charpool_iter++) {
		//寻找所有合适的边属性 
			tmpset = move(bas_nfa,state[tmp],*charpool_iter); //跑一遍move 
		//寻找本DFA中是否已存在new状态 
			reppos = -1;
			for(int i=1;i<=size;i++)
				if(compset(tmpset,state[i])) {
					reppos = i;
					break;
				}
		//执行结果 
			if(reppos == -1 && tmpset.size() == 0) { //空状态，索引回1 
				lnk[tmp][*charpool_iter] = 1;
			}
			else if(reppos == -1) { //无重复，建立新状态保存之，建边，入队 
				state[++size] = tmpset;
				lnk[tmp][*charpool_iter] = size;
				q.push(size);
			}
			else { //已存在，建边即可 
				lnk[tmp][*charpool_iter] = reppos;
			}
		}
	} 
	charpool = bas_nfa.charpool; //继承NFA的charpool 
//	debug();
	update();
//	debug();
}

void project::read_reg (int reg_max_size, bool switch_debug) {
	project::clear();
	char *reg = new char[reg_max_size+2];
	read_string(reg);
	while(NFA::read_reg(reg,0,strlen(reg)-1,0,1,true) == false) {
		puts("Invalid regular expression!");
		read_string(reg);
	}
	if(switch_debug)
		NFA::debug(); 
	DFA::transform_NFA_to_DFA(*this);
	if(switch_debug)
		DFA::debug();
}
bool project::read_reg (char *reg, bool switch_debug) {
	if(NFA::read_reg(reg,0,strlen(reg)-1,0,1,true) == false)
		return false;
	if(switch_debug)
		NFA::debug();
	DFA::transform_NFA_to_DFA(*this);
	if(switch_debug)
		DFA::debug();
	return true;
}
