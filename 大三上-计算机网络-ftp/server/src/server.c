#include "utils.h"
#include "inttypes.h"
int main(int argc, char **argv) {
	int port=21;
	char root[MAX_DIR_LEN];
	strcpy(root, "/tmp\0");
	char PORT[] = "-port\0";
	char ROOT[] = "-root\0";
	char cwd[MAXLINE];
	getcwd(cwd, sizeof(cwd));
	for(int i=1;i<argc-1;i++){
		if(strcmp(argv[i],PORT)==0){
			port = atoi(argv[i+1]);
			if((port<1024) && (port!=21)){
				printf("port number less than 1024(except for 21) is illegal! We will use the default port 21 here!\n");
				port=21;
			}
		}else if(strcmp(argv[i], ROOT)==0){
			if(strlen(argv[i+1])>=MAX_DIR_LEN){
				printf("-root argument is too long!");
				exit(1);
			}else if(isDirectoryExists(argv[i+1], cwd, 1)==0){
				strcpy(root,cwd);
			}else{
				printf("-root argument is not a valid directory! We will use the default \"/tmp\" directory here!\n");
			}
		}
	}

	int listenfd, connfd;
	struct sockaddr_in addr;

	//创建socket
	if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	//设置socket的端口号等
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//绑定socket
	if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	//socket开始监听
	if (listen(listenfd, 10) == -1) {
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	pid_t fpid;

	//父进程通过循环不断处理监听到的请求
	while (1) {

		char buffer[MAXLINE];
		memset(buffer, 0, MAXLINE);

		//对于请求，生成一个文件描述符为connfd的socket来处理
		if ((connfd = accept(listenfd, NULL, NULL)) == -1) {
			printf("Error accept(): %s(%d)\n", strerror(errno), errno);
			continue;
		}

		//创建子进程处理connfd
		fpid = fork();

		if(fpid<0){//情况一：子进程创建失败
			printf("Error creating child process");
		}else if(fpid==0){//情况二：子进程
			close(listenfd);//子进程不需要用到listenfd，所以子进程关闭listenfd对应的socket，注意这里不影响父进程使用listenfd
			int n;

			//常用response
			char welcome_msg[] = "220 Anonymous FTP server ready.\r\n";
			char _502_msg[] = "502 detect invalid command.\r\n";

			//发送欢迎信息：
			write(connfd, welcome_msg, strlen(welcome_msg));

			struct connectionInfo info;
			info.status = WAITING_FOR_USER_CMD;
			strcpy(info.dir, root);//此处应该处理下root过长的情况
			struct historyInfo history_info;
			history_info.transferredBytesOfFile=0;
			history_info.transferredBytesTotal=0;
			history_info.transferredFileNum=0;
			history_info.transferNum=0;
			info.history_info=history_info;
			struct fileTransferStateInfo ftsInfo;
			ftsInfo.inPASVmode=0;
			ftsInfo.inPORTmode=0;
			info.ftsInfo=ftsInfo;
			struct renameInfo rnInfo;
			rnInfo.lastCMDisRNFR = 0;
			rnInfo.last_RNFR_Success = 0;
			info.rnInfo = rnInfo;
			info.connfd = connfd;
			
			while((n=read(connfd, buffer, MAXLINE-1))){
				if(n<0){
					printf("Error read(): %s(%d)\n", strerror(errno), errno);
					close(connfd);
					break;
				}else if(n==0){
					write(connfd, _502_msg, strlen(_502_msg));
				}else if(n>=MAXLINE){
					break;//待读取的数据超过了缓冲区容纳能力的上限
				}else{
					if(n>=2){
						if((buffer[n-1]=='\n') && (buffer[n-2]=='\r')){
							buffer[n] = '\0';
							processCMD(connfd, buffer, n, &info);
						}
					}else{
						write(connfd, _502_msg, strlen(_502_msg));
					}
				}
			}
			close(connfd);
			exit(0);
		}else{//情况三：父进程
			close(connfd);//父进程不需要用到connfd，所以父进程关闭connfd对应的socket，注意这里不影响子进程使用connfd
		}	
	}

	close(listenfd);
}