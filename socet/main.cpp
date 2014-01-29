#define  _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <Windows.h>
#pragma comment(lib,"ws2_32.lib")

//size kbyte
#define SIZE 400
#define SIZE_DEC 200
#define KOL_BLOCK 4

char *memory;
char *memoryAsci;

char* mwrite(char *mem,char *block,int len){
	long i = 0;
	for(i=0;i<len;i++){
		mem[i] = block[i];
	}
	return &mem[len];
}
// Функция для загрузки страницы с адресом 
int loadxml(char *adr,char *page,char *memory){
    WSADATA wsaData;
	SOCKET Socket;
	struct hostent *host;

    WSAStartup(MAKEWORD(2,2), &wsaData);
    Socket=socket(AF_INET,SOCK_STREAM,0); //�������� (int) , ��� ������ (int) , 0 (����� �� ���������)
    host = gethostbyname(adr);
	printf("%s \n",host->h_name);

    SOCKADDR_IN SockAddr;
	memset(&SockAddr,0,sizeof(SockAddr));
    SockAddr.sin_port=htons(80);
    SockAddr.sin_family=AF_INET;
    SockAddr.sin_addr.s_addr = *((unsigned long*)host->h_addr);

    if(connect(Socket,(SOCKADDR*)(&SockAddr),sizeof(SockAddr)) != 0){
        printf("Could not connect\n");
        system("pause");
        return 1;
    }
	char st[200];
	sprintf(st,"GET %s HTTP/1.0\r\nHost: %s\r\n\r\n",page,adr);

    send(Socket,st, strlen(st),0);

    char buffer[200];
	char *newMem;
    int nDataLength;

	char cbuf = 0,prevc = 0,prc = 0;
	bool end = 0;
	int i = 0;

	
	//Вывод заголовка
	while(((nDataLength = recv(Socket,&cbuf, sizeof(cbuf),0)) > 0) && (end != 1)){
		printf("%c",cbuf);
		if( (prevc == 13) && (cbuf == 10) && (prc == 10) )
				end=1;
		prc = prevc;
		prevc = cbuf;
	}
	//Запись в память 
	newMem=mwrite(memory,&cbuf,sizeof(cbuf));
	while((nDataLength = recv(Socket,buffer, sizeof(buffer),0)) > 0){
		//fwrite(&buffer,1,nDataLength,f);
		newMem = mwrite(newMem,buffer,nDataLength);
	}
	//fclose(f);
	newMem = mwrite(newMem,"\0",1);
    closesocket(Socket);
	WSACleanup();
	return 0;
}
// Функция для перекодирования из кодировки УТФ8 в АСКИ
void utfToAsci(char *utf,char *asci){
	bool done = 0,stOut = 0;
	int lon,i,k,numUt,numAs;
	long code,codeShift = 0x430-'�';
	char buf;
	char enc[] = {"encoding=\"windows-1251\""};
	char *s;

	lon = 0;
	numUt = 0;
	numAs = 0;
	s = strstr(utf,enc);
	if(s!=NULL){
		numUt = 0;
		while(utf[numUt] != '\0'){
			asci[numUt] = utf[numUt];
			numUt++;
		}
		numAs = numUt;
	}else{
		while(1){
			while(!done){
				buf=utf[numUt];
				numUt++;
				if(buf == '\0'){
					stOut = 1;
					break;
				}
				for(i=0;i<4;i++){
					if((buf&(0x80>>i))>>(3-i) == 0){
						done = 1;
						break;
					}else{
							lon++;
					}
				}
			}
			if(stOut == 1){
				break;
			}
			if(lon == 2){
				code = 0;
				k=10;
				for(i=3;i<8;i++){
					code = code | ((buf&(0x80>>i))>>7-i)<<k;
					k--;
				}
				buf = utf[numUt];
					numUt++;
				if(buf == '\0'){
					break;
				}
				for(i=2;i<8;i++){
					code = code | ((buf&(0x80>>i))>>7-i)<<k;
					k--;
				}
				asci[numAs]=code-codeShift;
				numAs++;
			}
			if(lon == 0){
				asci[numAs]=buf;
				numAs++;
			}
			lon = 0;
			done = 0;
		}
	}
	asci[numAs] = '\0';
}

struct block{
	char title[90];
	char link[60];
	char desc[300];
};

// Функция для поиска новостей , состоит из заголовка ссылки и описания 
void parce(char *mem,block *b){
	int i,j,k;
	char title[] = {"<title>"};
	char link[] = {"<link>"};
	char desc[] = {"<description>"};
	char item[] = {"<item>"};
	char *fou;
	char *mem1;
	i=0;
	mem1 = mem;
	fou = strstr(mem1,item);
	mem1 = &fou[strlen(item)];
	for(i=0;i<KOL_BLOCK;i++){
		fou = strstr(mem1,title);
		j=0;
		k=0;
		fou = &fou[strlen(title)];
		while((fou[j] != '<') || (fou[j+1] != '/')){
			if(fou[j] == '&')
				j += 6;
			else{
				b[i].title[k] = fou[j];
				j++;
				k++;
			}
		}
		b[i].title[k] = '\0';
		fou = strstr(mem1,link);
		fou = &fou[strlen(link)];
		j=0;
		while(fou[j] != '<'){
			b[i].link[j] = fou[j];
			j++;
		}
		b[i].link[j] = '\0';
		fou = strstr(mem1,desc);
		fou = &fou[strlen(desc)];
		j=0;
		k=0;
		while((fou[j] != '<') || (fou[j+1] != '/')){
			if(fou[j] == '&')
				j += 6;
			else{
				b[i].desc[k] = fou[j];
				j++;
				k++;
			}
		}
		b[i].desc[k] = '\0';
		mem1 = &fou[j+strlen(desc)];
	}
}
struct adr{
	char ad[50];
	char page[120];
	int time;
};
// Функция для отделения адресса на состовляющие 
void getAdr(char *st,int t,adr *a){
	int i,j;
	i=0;
	j=0;
	while(st[i]!='/'){
		a[0].ad[i] = st[i];
		i++;
	}
	a[0].ad[i] = '\0';
	while(st[i]!='\0'){
		a[0].page[j] = st[i];
		j++;
		i++;
	}
	a[0].page[j] = '\0';
	a[0].time = t;
}

int readinfo(char *name,adr *a){
	FILE *f;
	char st[400];
	int t,kol,i;

	f = fopen(name,"rt");
	fscanf(f,"%d",&kol);
	for(i=0;i<kol;i++){
		fscanf(f,"%s",&st);
		if(feof(f))
			break;
		fscanf(f,"%d",&t);
		getAdr(st,t,&a[i]);
	}
	fclose(f);
	return kol;
}
//Основная функция 
int main (){
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
	block news[KOL_BLOCK];
	DWORD lastCall = 0;
	HWND hwnd;
	int i,kol;
	adr *a;

	hwnd = FindWindow("ConsoleWindowClass", NULL);
	memory = (char *)malloc(SIZE*1024);
	memoryAsci = (char *)malloc(SIZE_DEC*1024);
	a = (adr*)malloc(sizeof(adr)*10);

	kol = readinfo("sites.txt",a);
	//Бесконечный цикл
	while(1){
		//Цикл повторяется столько раз сколько сайтов с новостями 
		for(i=0;i<kol;i++){
			if((time(NULL) - lastCall >= a[i].time)){//обновление информации 
				ShowWindow(hwnd,SW_MAXIMIZE);
				lastCall = time(NULL);
				loadxml(a[i].ad,a[i].page,memory);//Чтение страницы
				system("cls");
				utfToAsci(memory,memoryAsci);//Перевод 
				parce(memoryAsci,news);//Выделние новостей 
				for(i=0;i<KOL_BLOCK;i++){
					printf("title:%s\n\n",news[i].title);
					printf("desc:%s\n\n",news[i].desc);
					printf("==========================================\n");
				}
				system("pause");
				ShowWindow(hwnd,SW_MINIMIZE);
			}
			Sleep(30);
		}
	}
	free(a);
	free(memory);
	free(memoryAsci);
    return 0;
}
