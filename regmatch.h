/* 
ʹ��class project����һ��������ʽƥ�乤�̣� 
�ɵ��þ���� 
void project::read_reg(int reg_max_size, bool switch_debug)
bool project::read_reg(char *reg, bool switch_debug)
	���ܣ�����һ��������ʽ�������������ɿ���̨���룬�������򷵻�������ʽ�Ƿ�Ϸ��� 
bool project::match(char *str)
	���ܣ��ַ���ƥ�䣬str���Ƿ������������ʽƥ�䡣���뱣֤�Ѿ�������������ʽ�� 
void clear()
	���ܣ����������ʽ����project��ԭΪԭ״̬�� 
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
const int MAX_REG_LENGTH = 1005; //������ʽ��������ƣ� 

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
		int size; //Ĭ����ʼ��stΪ�ڵ�0,dest�ڵ�Ϊ1 
		map<char,int> head[NFA_MAX_SIZE];  //link[i][A] = j : ״̬i�����ַ�A�����״̬j������հ�Ϊ'\0',else��Ϊ'\1' 
		//else���Ķ��壺������charpool����κ��ַ�ʱ����������
		int pos[NFA_MAX_LSTSIZE], nxt[NFA_MAX_LSTSIZE], cnt; //�ڽӱ� 
		set<char> charpool; //���������ַ����ر�أ�'\1'����charpool���ҽ��������ų����� 
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
		int size; //Ĭ����ʼ��st=1,�ս��dest=size 
		set<int> state[DFA_MAX_SIZE]; //ÿ���ڵ�����״̬ 
		int lnk[DFA_MAX_SIZE][CHARPOOL_MAX_SIZE]; //����DFA�ı� 
		set<char> charpool; //���������ַ����ر�أ�'\1'����charpool���ҽ��������ų����� 
		bool cancelled[DFA_MAX_SIZE]; //��DFAʱ���Ƿ�ɾ�� 
		bool is_end[DFA_MAX_SIZE]; //�Ƿ�Ϊ����̬ 
};

class project: private NFA, private DFA { //��������ʹ��һ��class���ɵ���������ʽ�����ƥ���������� 
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
//����NFA��ÿ�������Կ��ܶ�Ӧ����ߣ�ʹ���ڽӱ�洢 
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
int get_next_pos(char *reg,int st) { //Ѱ����ĳ������ƥ��������ţ���������������򷵻�ԭλ�� 
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
	int mode; //0 - ������  1 - ����  2 - ����  3 - Kleene��  4 - �ų����ţ�δ������� 
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
	if(reg[L] == '^' && reg[L-1] != '\\') //�ж��Ƿ�Ϊ�ų����㣺������Ч���ų��������Ȼ��L�� 
		return split_result(4,L+1,R);
	int lkloc=-1, unloc=-1, klloc=-1;
	for(int i=L;i<=R;i++) {
		i = get_next_pos(reg,i); //��iǰ����ƥ��������λ���ϣ����������������ϣ��򲻸ı�i����reg[i]��Ϊ�������ţ� 
		if(reg[i] == '\\') //��һ���ַ���¼һ�� 
			i++;
		cnt++; //�ҵ�һ��Ԫ������ 
		if(i>=R)
			break;
		if(reg[i+1] == '*' && reg[i] != '\\') { //�ҵ���һ��Kleene�� 
			i++;
			if(klloc == -1)
				klloc = i;
		} //���ﲻ��ֱ��else if: ����a*b�മ 
		if(i>=R)
			break;
		if(reg[i+1] == '|' && reg[i] != '\\') { //�ҵ���һ������ 
			i++; //��֤�ټ�һ�������÷��� 
			if(unloc == -1)
				unloc = i; //unlocλ���� | ������ 
		}
		else { //�ҵ���һ������ 
			if(lkloc == -1) {
				lkloc = i;
			}
		}
	}
//	cout<<lkloc<<' '<<unloc<<' '<<klloc<<endl;
//	cout<<L<<' '<<R<<endl;

	split_result ret;
	if(lkloc != -1) { //�������ȼ��� 
		//L ~ loc, loc+1 ~ R
		ret = split_result(1,L,lkloc,lkloc+1,R);
		ret.maintain(reg);
		return ret;
	}
	if(unloc != -1) { //�������ȼ����� 
		//L ~ loc-1, loc+1 ~ R
		ret = split_result(2,L,unloc-1,unloc+1,R);
		ret.maintain(reg);
		return ret;
	}
	if(klloc != -1) { //Kleene�����ȼ���� 
		//L ~ loc-1
		ret = split_result(3,L,klloc-1);
		ret.maintain(reg);
		return ret;
	}
//	cout<<"ssssss"<<endl; 
	ret = split_result(0,L,R);
	ret.maintain(reg);
	return ret;
	//������ 
}
struct hidden_edge_node {
	int left_bound, right_bound; //�洢�ų����ַ���position in reg[]
	int u,v; //u->v���� 
};
queue<hidden_edge_node> hidden_edge;
bool NFA::read_reg (char *reg, int L, int R, int start_point, int dest_point, bool is_birth) {
	if(!check_reg(reg)) {
		return false;
	}
//��ʱ�Ͽ�reg���Ѿ�ͨ����ƴд��� 
	if(is_birth) //��ʼ�� 
		while(!hidden_edge.empty())
			hidden_edge.pop();
	if(is_birth && reg[R] == '$' && reg[R-1] != '\\') //����±� 
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
	if(is_birth && reg[L] == '^') //����ϱ� 
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
	while(reg[L] == '(' && R == get_next_pos(reg,L)) //ȥ���� 
		L++, R--;
	//from start_point to dest_point : [reg] route 
	split_result ret = split(reg,L,R);
	if(ret.mode == 0) { //���ַ�����ʼ���� 
		if(ret.leftL != ret.leftR && (ret.leftL != ret.leftR-1 || reg[ret.leftL] != '\\')) {
			puts("Error 1 in read_reg!!"); //���ַ����� 
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
	else if(ret.mode == 4) { //�ų����㣬ѹ�����߶��У�����������ų��ַ�����charpool�� 
		/********///  ������ɲ��֣�����֤��ȷ 
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
	/********///  ������ɲ��֣�����֤��ȷ 
	if(is_birth) { //�������ߣ���ն��� 
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
				if(hidden_edge_flag) { //��ʾ*charpool_iter��Ϊ���ų���������� 
					addedge(current_edge.u, current_edge.v, *charpool_iter);
				}
			}
			addedge_without_adding_in_charpool(current_edge.u, current_edge.v, '\1'); //��else�� 
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
//Ѱ����ĳ������ƥ��������ţ���������������򷵻�ԭλ�� 
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
bool check_reg(char *reg) { //ƴд��������� 
	if(strlen(reg) >= MAX_REG_LENGTH) { //���ʽ���� 
		puts("The regular expression string is too long. Try to input a string with length less than 1000!");
		return false;
	}
	int len = strlen(reg), tmp;
	if(reg[len-1] == '\\') {
		puts("Found char \\ at the end!"); //��β��б�� 
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
void read_string(char *str) { //����һ���ַ��������Կո� 
	char c = getchar(); int cnt=0;
	while(c == '\n' || c == '\r')
		c = getchar();
	while(c != '\n' && c != '\r') 
		str[cnt++] = c, c = getchar();
	str[cnt] = '\0';
}

bool DFA::match (char *str) {
	int len = strlen(str), current = 1;
	if(is_end[current]) //DFA�ǿյ� 
		return true;
	for(int i=0;i<len;i++) {
		if(lnk[current][str[i]] == 0) //�������δ��������else�� 
			current = lnk[current][('\1')];
		else
			current = lnk[current][str[i]]; //�ر��н� 
	}
	return is_end[current]; //���������ʱ�̳ɹ��ܵ� 
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
��DFA���� 
���ܣ�
	������еĿհ��ڵ� 
*/ 
void DFA::update () {  
	set<char>::iterator charpool_iter;
	memset(cancelled,0,sizeof(cancelled));
	for(int i=1;i<=size;i++) 
		is_end[i] = (state[i].find(1) != state[i].end()); // is_end[i] = true: i״̬Ϊ����̬ 
	bool flag;
	for(int i=1;i<=size;i++) {
		if(is_end[i])
			continue;
		for(int j=i+1;j<=size;j++) {
			if(is_end[j])
				continue;
			flag = true;
			for(charpool_iter = charpool.begin();charpool_iter != charpool.end();charpool_iter++) {  
				if(lnk[i][*charpool_iter] != lnk[j][*charpool_iter]) //�ж������ڵ�ɷ�ϲ����Ƿ�ӵ����ȫ��ͬ�ĳ��� 
					flag = false;
			}
			if(flag) { //ɾ�� 
				cancelled[j] = true; //����ɾ����� 
				for(int _i=1;_i<=size;_i++)
					for(charpool_iter = charpool.begin();charpool_iter != charpool.end();charpool_iter++) 
						if(lnk[_i][*charpool_iter] == j) //�����е���ߺϲ���ͬһ�ڵ� 
							lnk[_i][*charpool_iter] = i;
			}
		}
	}
}
/*
����ΪNFAתDFA����Ⱥ 
�������㷨��
���ڷ�ȷ���Զ���S�����ν������в����� 
	1) ��S��ʼ�ڵ�0��Ѱ��e(�ŵ�������ţ���ͬ)�հ�����ѡ���㼯����Ϊ���ɵ�DFA�Զ���T����ʼ״̬������״̬���Q 
	2) ȡ��Q���׵�״̬��Ϊcur�� 
	3) ��cur��һ�����ܵ��ַ�a����move(a)�������õ����ɸ�״̬new 
	4) ���T��û��״̬new����T�м���״̬new����״̬new���Q����newΪ�ջ�T������״̬new�򲻽��д��� 
	5) ���ߣ�T��cur״̬��new״̬��������Ϊa�ıߡ��ر�أ���newΪ��������ʼ״̬���ߡ� 
	6) ��һ��״̬cur����ÿ�������бߣ�����charpool�У����ַ�ִ��һ�����(3)-(5)�� 
	7) �ظ�����(2)-(6)ֱ��QΪ�ա�
	8) ����T�е�ÿһ��������ֹ�ڵ��״̬Ϊ����̬�� 
*/ 
set<int> get_closure(NFA &bas_nfa, set<int> &bas_set, char key0) { //Ѱ��bas_set��key0�հ��ĺ�����e�հ�Ϊ'\0'�հ��� 
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
		while(!bfsq.empty()) //queue��Ȼû��clear()...������ 
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
set<int> get_e_closure(NFA &bas_nfa, set<int> &bas_set) { //������� 
	return get_closure(bas_nfa, bas_set,'\0');
}
set<int> get_closure(NFA &bas_nfa, set<int> &bas_set, char key0, char key1) { //Ѱ��˫�ַ��հ��ĺ�����δʹ�ã� 
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
		while(!bfsq.empty()) //queue��Ȼû��clear()...������ 
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
//move(keyc)�������õ�bas_set�㼯�����е���һ��keyc��֮��Ŀ���Ŀ�ĵ��γɵĵ㼯  
	set<int> ret;
	set<int>::iterator iter;
	int u;
	for(iter = bas_set.begin(); iter != bas_set.end(); iter++) { //������� 
		for(u=bas_nfa.head[*iter][keyc];u!=0;u=bas_nfa.nxt[u])
			ret.insert(bas_nfa.pos[u]);
	}
	return get_closure(bas_nfa, ret, '\0');
}

bool compset(set<int> x,set<int> y) { //�ж�����set�Ƿ���� 
	if(x.size() != y.size())
		return false;
	set<int>::iterator iter;
	for(iter = x.begin();iter != x.end();iter++)
		if(y.find(*iter) == y.end())
			return false;
	return true;
}

void DFA::transform_NFA_to_DFA(NFA &bas_nfa) { //ת������ 
	size = 0;
	set<int> tmpset;
	set<char>::iterator charpool_iter;
	int tmp, reppos;
	queue<int> q;
	tmpset.insert(0);
	state[++size] = get_closure(bas_nfa, tmpset, '\0'); //����ʼ״̬���浽DFA��state״̬���� 
	q.push(size); //��ʼ״̬�ı����� 
	while(!q.empty()) {
		tmp = q.front();//���δ����״̬Ϊstate[tmp] 
		q.pop();
		for(charpool_iter = bas_nfa.charpool.begin(); charpool_iter != bas_nfa.charpool.end(); charpool_iter++) {
		//Ѱ�����к��ʵı����� 
			tmpset = move(bas_nfa,state[tmp],*charpool_iter); //��һ��move 
		//Ѱ�ұ�DFA���Ƿ��Ѵ���new״̬ 
			reppos = -1;
			for(int i=1;i<=size;i++)
				if(compset(tmpset,state[i])) {
					reppos = i;
					break;
				}
		//ִ�н�� 
			if(reppos == -1 && tmpset.size() == 0) { //��״̬��������1 
				lnk[tmp][*charpool_iter] = 1;
			}
			else if(reppos == -1) { //���ظ���������״̬����֮�����ߣ���� 
				state[++size] = tmpset;
				lnk[tmp][*charpool_iter] = size;
				q.push(size);
			}
			else { //�Ѵ��ڣ����߼��� 
				lnk[tmp][*charpool_iter] = reppos;
			}
		}
	} 
	charpool = bas_nfa.charpool; //�̳�NFA��charpool 
//	debug();
	update();
//	debug();
}

void project::read_reg (int reg_max_size, bool switch_debug) {
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
