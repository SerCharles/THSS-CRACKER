#ifndef _UTILS_H_

#ifndef _GNU_SOURCE
  #define _GNU_SOURCE
#endif

#ifndef _RESULT_MACRO
  #define _RESULT_MACRO 0
  #define _FILE_EXIST 50
  #define _NAME_START_WITH_ILLEGAL_DOUBLE_DOT 100
  #define _DATA_TRANSFER_SUCCESS 200
  #define _DATA_TRANSFER_FAILURE 250
  #define _FILE_NOT_EXIST 300
#endif

#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <dirent.h>
#include <arpa/inet.h>


#define MAXLINE 4096 /*max text line length*/
#define MAX_DIR_LEN 4096
#define WAITING_FOR_USER_CMD 2
#define WAITING_FOR_PASS_CMD 3
#define LOGGED_IN 4

struct historyInfo{
	int transferredFileNum;
	int transferredBytesOfFile;
	int transferredBytesTotal;
	int transferNum;
};

struct PORT_modeInfo // 存储PORT模式下的信息
{
	int ip[4]; // 客户端指定的客户端ip
	int port; // 客户端的客户端数据传输的端口号
	int dataSocketFD; // file descriptor of the socket for data transfer on the server side
};

struct PASV_modeInfo // 存储PASV模式下的信息
{
	int dataSocketFD; // file descriptor of the socket for data transfer on the server side
};

struct fileTransferStateInfo{
	int inPASVmode;
	int inPORTmode;
	struct PASV_modeInfo PASV_INFO; // 存储PASV模式下的信息
	struct PORT_modeInfo PORT_INFO; // 存储PORT模式下的信息
};

struct renameInfo{
	char originName[MAXLINE];
	int lastCMDisRNFR; // equals to 1 if last command is "RNFR"; else 0
	int last_RNFR_Success;
};

struct connectionInfo{
	char dir[MAX_DIR_LEN];//服务器端当前的工作目录
	int status;//表示服务器端当前的状态
	struct historyInfo history_info;
	struct fileTransferStateInfo ftsInfo;
	struct renameInfo rnInfo;
	int64_t start_position; //用于支持断点续传
	int connfd; // file descriptor of the socket used for command transfer
};


void processCMD(int connfd, char* buf, int len, struct connectionInfo* info);
int checkEmailAddr(char* buf);
char* itoa(int num, char* str, int base); // This function is not defined in ANSI-C, so we implement this function in our code.
int isDirectoryExists(const char *path, char* wd, int changewd);
int startsWith(const char* data, const char* head);
int listFiles( int dataSocektFD, const char* directory);
int getFileProperties(char* fn, int dataSocketFD, const char* working_directory);
int gen_port(int *fd, int listenOrNor);
int getip(int sockfd, int* ip);
int getAbsoluteFilePath(const char* fn, const char* working_directory, char* buffer);

// handles for ftp command processing:
void ftp_quit(char* buf, struct connectionInfo* info);
void ftp_user(char* buf, struct connectionInfo* info);
void ftp_pass(char* buf, struct connectionInfo* info);
void ftp_syst(char* buf, struct connectionInfo* info);
void ftp_rnfr(char* buf, struct connectionInfo* info);
void ftp_rnto(char* buf, struct connectionInfo* info);
void ftp_type(char* buf, struct connectionInfo* info);
void ftp_pwd(char* buf, struct connectionInfo* info);
void ftp_cwd(char* buf, struct connectionInfo* info);
void ftp_mkd(char* buf, struct connectionInfo* info);
void ftp_pasv(char* buf, struct connectionInfo* info);
void ftp_port(char* buf, struct connectionInfo* info);
void ftp_rest(char* buf, struct connectionInfo* info);
void ftp_rmd(char* buf, struct connectionInfo* info);

//handles for ftp data transfer over the data connection
void ftp_transfer_data(char* buf, struct connectionInfo* info);
int ftp_list(int dataTransferConnectionFD, char* buf, struct connectionInfo* info);
int ftp_retr(int dataTransferConnectionFD, char* buf, struct connectionInfo* info);
int ftp_stor(int dataTransferConnectionFD, char* buf, struct connectionInfo* info);


void process_QUITorABOR_CMD(int connfd, char* buf, int len, struct connectionInfo* info);
void process_SYST_CMD(int connfd, char* buf, int len, struct connectionInfo* info);

#endif
