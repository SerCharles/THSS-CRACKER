#include "utils.h"



void processCMD(int connfd, char* buf, int len, struct connectionInfo* info){
	// Stage 1:先判断是否为QUIT或ABOR指令
	if(startsWith(buf, "QUIT") || startsWith(buf, "ABOR")){
		/**
		 * 处理QUIT、ABOR指令。这些指令在任何情况都能调用，所以最先处理。
		*/
		ftp_quit(buf, info);
		return;
	}

	// Stage 2: 处理其他指令
	switch (info->status){
		case WAITING_FOR_USER_CMD:{
			if(startsWith(buf, "USER")){
				ftp_user(buf, info);
			}else if(startsWith(buf, "SYST")){
				ftp_syst(buf, info);
			}else{
				char res[] = "530 Not logged in.\r\n";
				write(connfd, res, strlen(res));
			}
			break;
		}
		case WAITING_FOR_PASS_CMD:{
			if(startsWith(buf, "PASS")){
				ftp_pass(buf, info);
			}else if(startsWith(buf, "SYST")){
				ftp_syst(buf, info);
			}else{
				char res[] ="530 Not logged in.\r\n";
				write(connfd, res, strlen(res));
			}
			break;
		}
		case LOGGED_IN:{
            //首先判断是否为RNFR，若不是则修改info->rnInfo.lastCMDisRNFR
			if(startsWith(buf, "RNFR")){
				ftp_rnfr(buf, info);
				return; // no further process in this case!
			}else if(startsWith(buf, "RNTO")){
				ftp_rnto(buf, info);
                return; // no further process in this case!
			}else{
				info->rnInfo.lastCMDisRNFR = 0; // further process later!
			}

			if(startsWith(buf, "SYST")){ 
				/**
				 * 处理SYST指令
				*/
				ftp_syst(buf, info);
			}else if(startsWith(buf, "TYPE")){
				/**
				 * 处理TYPE指令
				*/
				ftp_type(buf, info);
			}else if(startsWith(buf, "PWD")){
				/**
				 * 处理PWD指令
				*/
				ftp_pwd(buf, info);
			}else if(startsWith(buf, "CWD")){
				/**
				 * 处理CWD指令
				*/
				ftp_cwd(buf, info);
			}else if(startsWith(buf, "MKD")){
				/** 
				 * 处理MKD指令：MKD <sp> <pathname> <CRLF>
				*/
				ftp_mkd(buf, info);
			}else if(startsWith(buf, "PASV")){
				ftp_pasv(buf, info);
			}else if(startsWith(buf, "PORT")){
				ftp_port(buf, info);
			}else if(startsWith(buf, "RETR") || startsWith(buf, "STOR") || startsWith(buf, "LIST")){
				ftp_transfer_data(buf, info);
			}else if(startsWith(buf, "REST")){
				ftp_rest(buf, info);
			}else if(startsWith(buf, "RMD")){
				ftp_rmd(buf, info);
			}else{
				char res[] = "500 Syntax error, command unrecognized.\r\n";
				write(connfd, res, strlen(res));
				break;
			}
			break;
		}
		default:{
			char res[] = "500 Syntax error, command unrecognized.\r\n";
			write(connfd, res, strlen(res));
			break;
		}
	}
}

int checkEmailAddr(char* buf){
	/**	检测邮箱是否合法，基于如下正则表达式检测：^[a-zA-Z0-9_-]+@[a-zA-Z0-9_-]+(\.[a-zA-Z0-9_-]+)+$
	 * @param buf 以'\r\n‘结尾的char数组。
	 * @return 合法返回1，不合法返回0。
	*/
	if(strcmp(buf, "anonymous@\r\n")==0){
		return 1;
	}
	if(strlen(buf)<=2){
		return 0;
	}
	int pos=-1;
	for(int i = 0;i<strlen(buf)-2;i++){
		if(buf[i]=='@'){
			pos=i;
		}
	}
	if(pos==-1 || pos==0 || pos==(strlen(buf)-3)){
		return 0;
	}
	for(int i=0;i<pos;i++){
		if((buf[i]>='a' && buf[i]<='z')||(buf[i]>='A' && buf[i]<='Z')||(buf[i]>='0' && buf[i]<='9')||(buf[i]=='-')||(buf[i]=='_')){
			continue;
		}
		return 0;
	}
	int status1=2;//表示之前匹配到1个‘.’的状态
	int status2=3;//表示之前匹配到1个[a-zA-Z0-9_-]的状态
	int dot_found=0;//表示是否曾匹配到过'.'
	int status=0;
	if((buf[pos+1]>='a' && buf[pos+1]<='z')||(buf[pos+1]>='A' && buf[pos+1]<='Z')||(buf[pos+1]>='0' && buf[pos+1]<='9')||(buf[pos+1]=='-')||(buf[pos+1]=='_')){
		status= status2;
	}else{
		return 0;
	}
	for(int i=pos+2;i<strlen(buf)-2;i++){
		if(status==status1){
			if((buf[i]>='a' && buf[i]<='z')||(buf[i]>='A' && buf[i]<='Z')||(buf[i]>='0' && buf[i]<='9')||(buf[i]=='-')||(buf[i]=='_')){
				status=status2;
			}else{
				return 0;
			}
		}else if(status==status2){
			if(buf[i]=='.'){
				status=status1;
				dot_found=1;
			}else if((buf[i]>='a' && buf[i]<='z')||(buf[i]>='A' && buf[i]<='Z')||(buf[i]>='0' && buf[i]<='9')||(buf[i]=='-')||(buf[i]=='_')){
				status=status2;
			}else{
				return 0;
			}
		}else{
			return 0;
		}
	}
	if(status==status2){
		if(dot_found){
			return 1;
		}
	}
	return 0;
}

void ftp_user(char* buf, struct connectionInfo* info){
	if(strcmp("USER anonymous\r\n", buf)==0){
		char res[] ="331 Guest login ok, send your complete e-mail address as password.\r\n";
		info->status = WAITING_FOR_PASS_CMD;
		write(info->connfd, res, strlen(res));
	}else{
		char res[] ="530 the username is not acceptable.\r\n";
		write(info->connfd, res, strlen(res));
	}
}

void ftp_pass(char* buf, struct connectionInfo* info){
	if(checkEmailAddr(buf+5)){
		info->status = LOGGED_IN;
		char res[] = "230-\r\n230-Welcome to\r\n230- School of Software\r\n230- FTP Archives at ftp.ssast.org\r\n230-\r\n230-This site is provided as a public service by School of\r\n230-Software. Use in violation of any applicable laws is strictly\r\n230-prohibited. We make no guarantees, explicit or implicit, about the\r\n230-contents of this site. Use at your own risk.\r\n230-\r\n230 Guest login ok, access restrictions apply.\r\n";
		write(info->connfd, res, strlen(res));
	}else{
		char res[] ="530 the password is not acceptable(a valid email address is expected).\r\n";
		write(info->connfd, res, strlen(res));
	}
}

char* itoa(int num, char* str, int base) { 
    int i = 0; 
    int isNegative = 0; 
  
    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (num == 0) 
    { 
        str[i++] = '0'; 
        str[i] = '\0'; 
        return str; 
    } 
  
    // In standard itoa(), negative numbers are handled only with  
    // base 10. Otherwise numbers are considered unsigned. 
    if (num < 0 && base == 10) 
    { 
        isNegative = 1; 
        num = -num; 
    } 
  
    // Process individual digits 
    while (num != 0) 
    { 
        int remain = num % base; 
        str[i++] = (remain > 9)? (remain-10) + 'a' : remain + '0'; 
        num = num/base; 
    } 
  
    // If number is negative, append '-' 
    if (isNegative) 
        str[i++] = '-'; 
  
    str[i] = '\0'; // Append string terminator 
  
    // Reverse the string 
    char* buffer = (char*) malloc (strlen(str)+1);
	strcpy(buffer, str);
	const int len = strlen(str);
	for(int i=0;i<len;i++){
		str[i]=buffer[len-1-i];
	}
	free(buffer);
  
    return str; 
}

int isDirectoryExists(const char *path, char* wd, int changewd){
	/**
	 * check if directory exists; if it exists and changewd!=0, change wd to new workding directory
	 * @param path may be relative or absolute
	 * @param wd working directory
	 * @return return 0 if success; return -1 if path starts with "../"; return -2 if directory not exists
	*/
	if(path[0]=='.' && path[1]=='.' && path[2]=='/'){
		return -1;
	}
	char* buffer = (char*) malloc (2*MAXLINE);
	if(path[0]=='/'){
		strcpy(buffer, path);
	}else{
		strcpy(buffer, wd);
		if(buffer[strlen(buffer)-1]!='/'){
			strcat(buffer, "/");
		}
		strcat(buffer, path);
	}
    struct stat stats;
    stat(buffer, &stats);
    if (S_ISDIR(stats.st_mode)){
		if(changewd){
			strcpy(wd, buffer);
		}
		free(buffer);
        return 0;
	}
	free(buffer);
    return -2;
}

int startsWith(const char* data, const char* head){
	for(int i=0;i<strlen(head);i++){
		if(data[i]!=head[i]){
			return 0;
		}
	}
	if(strlen(data)>(strlen(head)+2)){ // 2为"\r\n"的长度
		if(data[strlen(head)]!=' '){
			return 0;
		}
	}
	return 1;
}


int listFiles(int dataSocketFD, const char* directory){
	/**
	 * @return 若directory不是文件夹名，返回_DATA_TRANSFER_FAILURE；若操作成功，返回_DATA_TRANSFER_SUCCESS
	*/

	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (directory)) != NULL) {
		char line[MAXLINE];
		// print all the files and directories within directory
		while ((ent = readdir (dir)) != NULL) {
			struct stat st;
			char filename[MAXLINE];
			snprintf(filename, sizeof(filename), "%s/%s", directory, ent->d_name);
			lstat(filename, &st);
			if((strcmp(ent->d_name, ".")==0) || (strcmp(ent->d_name, "..")==0)){
				continue;
			}
			if(S_ISDIR(st.st_mode)){
				strcpy(line,"d");
			}else if(S_ISLNK(st.st_mode)){
				strcpy(line,"l");
			}else{
				strcpy(line,"-");
			}
			strcat(line, (st.st_mode & S_IRUSR) ?"r":"-");
			strcat(line, (st.st_mode & S_IWUSR) ?"w":"-");
			strcat(line, (st.st_mode & S_IXUSR) ?"x":"-");
			strcat(line, (st.st_mode & S_IRGRP) ?"r":"-");
			strcat(line, (st.st_mode & S_IWGRP) ?"w":"-");
			strcat(line, (st.st_mode & S_IXGRP) ?"x":"-");
			strcat(line, (st.st_mode & S_IROTH) ?"r":"-");
			strcat(line, (st.st_mode & S_IWOTH) ?"w":"-");
			strcat(line, (st.st_mode & S_IXOTH) ?"x":"-");
			strcat(line, " ");
			char buf[200];
			memset(buf, 0, sizeof(buf));
			sprintf(buf, "%jd", st.st_size);
			strcat(line, buf);
			strcat(line, " ");
			strcat(line, getpwuid(st.st_uid)->pw_name);
			strcat(line, " ");
			strcat(line, getgrgid(st.st_gid)->gr_name);
			strcat(line, " ");
			memset(buf, 0, sizeof(buf));
			struct tm* t;
			t = localtime(&(st.st_mtime));
			strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M", t);
			strcat(line, buf);
			strcat(line, " ");
			strcat(line, ent->d_name);
			strcat(line,"\r\n");
			write(dataSocketFD, line, strlen(line));
		}
		closedir(dir);
		return _DATA_TRANSFER_SUCCESS;
	} else {
		// could not open directory
		return _DATA_TRANSFER_FAILURE;
	}
}

int isFileExists(const char* filename){
	struct stat buffer;
	if(stat(filename, &buffer)==0){
		return 1;
	}else{
		return 0;
	}
}

int getAbsoluteFilePath(const char* fn, const char* working_directory, char* buffer){
	/**
	 * 得到绝对路径并存储在buffer中
	 * @param fn 文件路径，可能相对或绝对
	 * @param working_directory 工作目录
	 * @param buffer 存储实验结果
	 * @return 若操作成功，返回_FILE_EXIST；若fn以"../"开头，返回_NAME_START_WITH_ILLEGAL_DOUBLE_DOT；若文件不存在，返回_FILE_NOT_EXIST
	*/
	if(fn[0]=='/'){ // absolute file path
		if(isFileExists(fn)){
			strcpy(buffer, fn);
			return _FILE_EXIST;
		}else{
			return _FILE_NOT_EXIST;
		}
	}else{
		if(fn[0]=='.' && fn[1]=='.' && fn[2]=='/'){
			return _NAME_START_WITH_ILLEGAL_DOUBLE_DOT;
		}
		strcpy(buffer, working_directory);
		if(buffer[strlen(buffer)-1]!='/'){
			strcat(buffer, "/");
		}
		strcat(buffer, fn);
		if(isFileExists(buffer)){
			return _FILE_EXIST;
		}else{
			return _FILE_NOT_EXIST;
		}
	}
}

int getFileProperties(char* fn, int dataSocketFD, const char* working_directory){
	/**
	 * list fn，其中working_directory为工作目录。
	 * @return 若操作成功，返回_DATA_TRANSFER_SUCCESS；若失败，返回_DATA_TRANSFER_FAILURE
	*/

	char* filename = (char*) malloc (strlen(fn)+strlen(working_directory)+20);;
	int result = getAbsoluteFilePath(fn, working_directory, filename);
	if(result!=_FILE_EXIST){
		free(filename);
		return _DATA_TRANSFER_FAILURE;
	}

	struct stat st;
	if(stat(filename, &st)!=0){
		free(filename);
		return _DATA_TRANSFER_FAILURE;
	}

	char line[MAXLINE];
	if(S_ISDIR(st.st_mode)){
		strcpy(line,"d");
	}else if(S_ISLNK(st.st_mode)){
		strcpy(line,"l");
	}else{
		strcpy(line,"-");
	}

	strcat(line, (st.st_mode & S_IRUSR) ?"r":"-");
	strcat(line, (st.st_mode & S_IWUSR) ?"w":"-");
	strcat(line, (st.st_mode & S_IXUSR) ?"x":"-");
	strcat(line, (st.st_mode & S_IRGRP) ?"r":"-");
	strcat(line, (st.st_mode & S_IWGRP) ?"w":"-");
	strcat(line, (st.st_mode & S_IXGRP) ?"x":"-");
	strcat(line, (st.st_mode & S_IROTH) ?"r":"-");
	strcat(line, (st.st_mode & S_IWOTH) ?"w":"-");
	strcat(line, (st.st_mode & S_IXOTH) ?"x":"-");
	strcat(line, " ");
	char buf[200];
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%jd", st.st_size);
	strcat(line, buf);
	strcat(line, " ");
	strcat(line, getpwuid(st.st_uid)->pw_name);
	strcat(line, " ");
	strcat(line, getgrgid(st.st_gid)->gr_name);
	strcat(line, " ");
	memset(buf, 0, sizeof(buf));
	struct tm* t;
	t = localtime(&(st.st_mtime));
	strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M", t);
	strcat(line, buf);
	strcat(line, " ");
	strcat(line, fn);
	strcat(line,"\r\n");
	write(dataSocketFD, line, strlen(line));
    return _DATA_TRANSFER_SUCCESS;
};


void ftp_quit(char* buf, struct connectionInfo* info){
	int isQuit = (buf[0]=='Q');
	int isAbor = (buf[0]=='A');
	if(strcmp(buf, "QUIT\r\n")==0 || strcmp(buf, "ABOR\r\n")==0){ // Case 1: [QUIT <CRLF>] | [ABOR <CRLF>]
		char res[1000];
		char tmp[200];
		res[0] = '\0';
		strcat(res, "221-You have transferred ");
		itoa(info->history_info.transferredBytesOfFile, tmp, 10);
		strcat(res, tmp);
		strcat(res, " bytes in ");
		itoa(info->history_info.transferredFileNum, tmp, 10);
		strcat(res, tmp);
		strcat(res, " files.\r\n221-Total traffic for this session was ");
		itoa(info->history_info.transferredBytesTotal, tmp, 10);
		strcat(res, tmp);
		strcat(res, " bytes in ");
		itoa(info->history_info.transferNum, tmp, 10);
		strcat(res, tmp);
		strcat(res, " transfers.\r\n221-Thank you for using the FTP service on ftp.ssast.org.\r\n221 Goodbye.\r\n");
		write(info->connfd, res, strlen(res));
		close(info->connfd);
		if(info->ftsInfo.inPASVmode){
			close(info->ftsInfo.PASV_INFO.dataSocketFD);
		}
		if(info->ftsInfo.inPORTmode){
			close(info->ftsInfo.PORT_INFO.dataSocketFD);
		}
		exit(0);
	}else{
		char tmp[5];
		if(isQuit){
			strcpy(tmp, "QUIT");
		}
		if(isAbor){
			strcpy(tmp, "ABOR");
		}
		char res[300];
		memset(res, 0, 300);
		strcpy(res, "501 invalid argument for \"");
		strcat(res, tmp);
		strcat(res, "\" command detected(no argument expected for \"");
		strcat(res, tmp);
		strcat(res, "\" command).\r\n");
		write(info->connfd, res, strlen(res));
	}
}

void ftp_syst(char* buf, struct connectionInfo* info){
	/**
	 * 处理SYST指令
	*/
	if(strcmp(buf, "SYST\r\n")==0){ // Case 1: SYST <CRLF>
		char res[] = "215 UNIX Type: L8\r\n";
		write(info->connfd, res, strlen(res));
	}else{
		char res[] ="501 invalid argument for \"SYST\" command detected(no argument expected for \"SYST\" command).\r\n";
		write(info->connfd, res, strlen(res));
	}
}

void ftp_rnfr(char* buf, struct connectionInfo* info){
	info->rnInfo.lastCMDisRNFR = 1;
	strcpy(info->rnInfo.originName, buf+5);
	info->rnInfo.originName[strlen(info->rnInfo.originName)-2]='\0';
	char buffer[2*MAXLINE];
	int result = getAbsoluteFilePath(info->rnInfo.originName, info->dir, buffer);
	for(int i=0;i<strlen(info->rnInfo.originName);i++){
		if(info->rnInfo.originName[i]=='/'){
			char res[] = "450 Requested file should not contain \"/\".\r\n";
			info->rnInfo.last_RNFR_Success = 0;
			write(info->connfd, res, strlen(res));
			return;
		}
	}
	if(result==_FILE_EXIST){
		char res[] = "350 Requested file exist\r\n";
		info->rnInfo.last_RNFR_Success = 1;
		write(info->connfd, res, strlen(res));
	}else if(result==_NAME_START_WITH_ILLEGAL_DOUBLE_DOT){
		char res[] = "550 Requested file should not start with \"../\".\r\n";
		info->rnInfo.last_RNFR_Success = 0;
		write(info->connfd, res, strlen(res));
	}else{
		char res[] = "550 Requested file not exist\r\n";
		info->rnInfo.last_RNFR_Success = 0;
		write(info->connfd, res, strlen(res));
	}
}

void ftp_rnto(char* buf, struct connectionInfo* info){
	if(!info->rnInfo.lastCMDisRNFR){
		info->rnInfo.lastCMDisRNFR=0;
		char res[] = "503 last command not \"RNFR\".\r\n";
		write(info->connfd, res, strlen(res));
	}else{
		info->rnInfo.lastCMDisRNFR=0;
		if(info->rnInfo.last_RNFR_Success){
			for(int i=5;i<strlen(buf);i++){
				if(buf[i]=='/'){
					char res[] = "550 new name should not contain\"/\".\r\n";
					write(info->connfd, res, strlen(res));
					return;
				}
			}
			char buffer1[2*MAXLINE];
			char buffer2[2*MAXLINE];
			if(getAbsoluteFilePath(info->rnInfo.originName, info->dir, buffer1)==_NAME_START_WITH_ILLEGAL_DOUBLE_DOT){
				char res[] = "550 last \"RNTO\" command failed because argument for RNFR starts with illegal \"../\".\r\n";
				write(info->connfd, res, strlen(res));
				return;
			}
			strcpy(info->rnInfo.originName, buf+5);
			info->rnInfo.originName[strlen(info->rnInfo.originName)-2]='\0';
			if(getAbsoluteFilePath(info->rnInfo.originName, info->dir, buffer2)==_NAME_START_WITH_ILLEGAL_DOUBLE_DOT){
				char res[] = "550 last \"RNTO\" command failed because argument for RNTO starts with illegal \"../\".\r\n";
				write(info->connfd, res, strlen(res));
				return;
			}
			int result = rename(buffer1, buffer2);
			if(result==0){
				char res[] = "250 rename success.\r\n";
				write(info->connfd, res, strlen(res));
			}else{
				char res[] = "550 last \"RNTO\" command failed.\r\n";
				write(info->connfd, res, strlen(res));
			}
			return;
		}else{
			char res[] = "550 last \"RNTO\" command failed.\r\n";
			write(info->connfd, res, strlen(res));
		}
	}
}

void ftp_rmd(char* buf, struct connectionInfo* info){
	char folder[MAXLINE];
	strcpy(folder, buf+4);
	folder[strlen(folder)-2]='\0';
	char buffer[2*MAXLINE];
	if(getAbsoluteFilePath(folder, info->dir,buffer)==_NAME_START_WITH_ILLEGAL_DOUBLE_DOT){
		char res[] = "550 \"RMD\" command failed because argument for RMD starts with illegal \"../\".\r\n";
		write(info->connfd, res, strlen(res));
		return;
	}
	if(remove(buffer)!=0){
		char res[] = "550 \"RMD\" command failed.\r\n";
		write(info->connfd, res, strlen(res));
	}else{
		char res[] = "250 RMD okay.\r\n";
		write(info->connfd, res, strlen(res));
	}
}

void ftp_type(char* buf, struct connectionInfo* info){
	if(strcmp(buf, "TYPE I\r\n")==0){ // Case 1: TYPE <sp> I <CRLF>
		char res[] = "200 Type set to I.\r\n";
		write(info->connfd, res, strlen(res));
	}else{
		char res[] ="504 invalid argument for \"TYPE\" command detected(argument \"I\" expected for \"TYPE\" command).\r\n";
		write(info->connfd, res, strlen(res));
	}
}

void ftp_pwd(char* buf, struct connectionInfo* info){
	if(strcmp(buf, "PWD\r\n")==0){ // Case 1: PWD <CRLF>
		char head[] = "257 current working directory: \"";
		char res[MAX_DIR_LEN+2+strlen(head)];
		strcpy(res, head);
		strcat(res, info->dir);
		strcat(res, "\".\r\n");
		write(info->connfd, res, strlen(res));
	}else{
		char res[] = "550 invalid argument for \"PWD\" command detected(no argument expected for \"PWD\" command).\r\n";
		write(info->connfd, res, strlen(res));
	}	
}

void ftp_cwd(char* buf, struct connectionInfo* info){
	if(strlen(buf)<=6){ // Case 1: CWD [<sp>] <CRLF>
		char res[] = "550 no parameter is used for \"CWD\" command.\r\n";
		write(info->connfd, res, strlen(res));
	}else{
		char* tmp = (char*) malloc (strlen(buf)-3);
		strcpy(tmp, buf+4);
		tmp[strlen(tmp)-2]='\0';
		char* buffer = (char*) malloc (strlen(buf)+strlen(tmp)+100);
		int result = isDirectoryExists(tmp, info->dir, 1);
		if(result==0){
			char res[] = "250 CWD Okay.\r\n";
			write(info->connfd, res, strlen(res));
		}else if(result==-1){
			char res[] = "550 Requested directory should not start with \"../\".\r\n";
			write(info->connfd, res, strlen(res));
		}else{
			char res[] = "550 Requested directory not exist.\r\n";
			write(info->connfd, res, strlen(res));
		}
		free(buffer);
		free(tmp);
	}
}

void ftp_rest(char* buf, struct connectionInfo* info){
	char tmp[MAXLINE];
	if(strlen(buf)>=MAXLINE){
		char res[] = "500 argument out of range.\r\n";
		write(info->connfd, res, strlen(res));
	}else{
		strcpy(tmp, buf+5);
		tmp[strlen(tmp)-2] = '\0';
		info->start_position = strtoll(tmp, NULL, 10);
		char res[] = "350 rest okay.\r\n";
		write(info->connfd, res, strlen(res));
	}
}

void ftp_pasv(char* buf, struct connectionInfo* info){
	if(info->ftsInfo.inPASVmode){
		info->ftsInfo.inPASVmode = 0;
		close(info->ftsInfo.PASV_INFO.dataSocketFD);
	}
	if(info->ftsInfo.inPORTmode){
		info->ftsInfo.inPORTmode = 0;
		close(info->ftsInfo.PORT_INFO.dataSocketFD);
	}
	int dataSocketFD;
	int port = gen_port(&dataSocketFD,1);
	if(port<0){
		char res[] = "501 PASV command failed, too many data ports in use.\r\n";
		write(info->connfd, res, strlen(res));
	}else{
		int ip[4];
		if(getip(info->connfd, ip)!=0){
			char res[] = "501 PASV command failed, unable to get server's ip address.\r\n";
			close(dataSocketFD);
			write(info->connfd, res, strlen(res));
		}else{
			info->ftsInfo.inPASVmode = 1;
			info->ftsInfo.inPORTmode = 0;
			info->ftsInfo.PASV_INFO.dataSocketFD = dataSocketFD;
			char res[150];
			sprintf(res, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n", ip[0], ip[1], ip[2], ip[3], port/256, port%256);
			write(info->connfd, res, strlen(res));
		}
	}
	info->start_position = 0;
}

void ftp_port(char* buf, struct connectionInfo* info){
	int ip[4];
	ip[0] = -1;
	ip[1] = -1;
	ip[2] = -1;
	ip[3] = -1;
	int port[2];
	port[0] = -1;
	port[1] = -1;
	sscanf(buf, "PORT %d,%d,%d,%d,%d,%d\r\n", &ip[0], &ip[1], &ip[2], &ip[3], &port[0], &port[1]);
	if((ip[0]<0)||(ip[1]<0)||(ip[2]<0)||(ip[3]<0)||(port[0]<0)||(port[1]<0)){
		char res[] = "501 invalid argument format for \"PORT\" command detected(no argument expected for \"PASV\" command).\r\n";
		write(info->connfd, res, strlen(res));
	}else{
		if(info->ftsInfo.inPASVmode || info->ftsInfo.inPORTmode){
			if(info->ftsInfo.inPASVmode){
				info->ftsInfo.inPASVmode = 0;
				close(info->ftsInfo.PASV_INFO.dataSocketFD);
			}
			if(info->ftsInfo.inPORTmode){
				info->ftsInfo.inPORTmode = 0;
				close(info->ftsInfo.PORT_INFO.dataSocketFD);
			}
		}
		char res[] = "200 PORT command successful.\r\n";
		info->ftsInfo.inPORTmode = 1;
		for(int i=0;i<4;i++){
			info->ftsInfo.PORT_INFO.ip[i] = ip[i];
		}
		info->ftsInfo.PORT_INFO.port = 256*port[0]+port[1];
		write(info->connfd, res, strlen(res));
	}
	info->start_position = 0;
}

void ftp_mkd(char* buf, struct connectionInfo* info){
	if(strlen(buf)<=6){ // Case 1
		char res[] = "501 invalid argument for \"MKD\" command detected.\r\n";
		write(info->connfd, res, strlen(res));
	}else{ // Case 2: MKD <sp> <pathname> <CRLF>
		char* tmp = (char*) malloc (strlen(buf)+MAXLINE+100);
		memset(tmp, 0, strlen(buf)+MAXLINE+100);
		if(buf[4]!='/'){
			strcpy(tmp, info->dir);
		}
		if(tmp[strlen(tmp)-1]!='/'){
			strcat(tmp,"/");
		}
		strcat(tmp, buf+4);
		tmp[strlen(tmp)-2]='\0';
		struct stat st = {0};
		if(stat(tmp, &st) == -1) {
			if(mkdir(tmp, 0777)==0){
				char res[] = "257 directory created.\r\n";
				write(info->connfd, res, strlen(res));
			}else{
				char res[] = "550 MDK failed.\r\n";
				write(info->connfd, res, strlen(res));
			}
		}else{
			char res[] = "550 directory already exists.\r\n";
			write(info->connfd, res, strlen(res));
		}
		free(tmp);
	}
}

void ftp_transfer_data(char* buf, struct connectionInfo* info){
	// first stage:
	if(startsWith(buf, "LIST")){
		char res[] = "150 opening data connection for LIST command.\r\n";
		write(info->connfd, res, strlen(res));
	}else{
		char res[MAX_DIR_LEN+300];
		strcpy(res, "150 opening BINARY data connection for ");
		strcat(res, buf+5);
		write(info->connfd, res, strlen(res));
	}
	
	// second stage
	if(fork()==0){
		if(info->ftsInfo.inPASVmode || info->ftsInfo.inPORTmode){
			int dataTransferConnectionFD;
			if(info->ftsInfo.inPASVmode){ // 等待客户端发起TCP连接

				dataTransferConnectionFD = accept(info->ftsInfo.PASV_INFO.dataSocketFD, NULL, NULL);
				if(dataTransferConnectionFD==-1){
					char res[] = "425 no TCP connection established for data transfer.\r\n";
					write(info->connfd, res, strlen(res));
					exit(0); //跳过数据传输阶段
				}
			}else if(info->ftsInfo.inPORTmode){ // 服务器端发起TCP连接
				struct sockaddr_in addr;
				memset(&addr, 0, sizeof(struct sockaddr_in));
				addr.sin_port = htons(info->ftsInfo.PORT_INFO.port);
				char buffer[100];
				sprintf(buffer, "%d.%d.%d.%d", info->ftsInfo.PORT_INFO.ip[0], info->ftsInfo.PORT_INFO.ip[1], info->ftsInfo.PORT_INFO.ip[2], info->ftsInfo.PORT_INFO.ip[3]);
				inet_aton(buffer, &addr.sin_addr);
				addr.sin_family = AF_INET;
				info->ftsInfo.PORT_INFO.dataSocketFD = socket(AF_INET, SOCK_STREAM, 0);
				if(info->ftsInfo.PORT_INFO.dataSocketFD==-1 || connect(info->ftsInfo.PORT_INFO.dataSocketFD, (struct sockaddr*)&addr, sizeof(addr))==-1){
					char res[] = "425 no TCP connection established for data transfer.\r\n";
					write(info->connfd, res, strlen(res));
					exit(0); //跳过数据传输阶段
				}
				dataTransferConnectionFD = info->ftsInfo.PORT_INFO.dataSocketFD;
			}

			/**
			 * 数据传输阶段
			*/
			int result;
			if(startsWith(buf, "LIST")){
				result = ftp_list(dataTransferConnectionFD, buf, info);
			}else if(startsWith(buf, "RETR")){
				result = ftp_retr(dataTransferConnectionFD, buf, info);
			}else if(startsWith(buf, "STOR")){
				result = ftp_stor(dataTransferConnectionFD, buf, info);
			}
			switch (result)
			{
				case _DATA_TRANSFER_SUCCESS:{
					char res[] = "226 transfer completed.\r\n";
					write(info->connfd, res, strlen(res));
					break;
				}
				case _NAME_START_WITH_ILLEGAL_DOUBLE_DOT:{
					char res[] = "451 file name should not start with \"../\"\r\n";
					write(info->connfd, res, strlen(res));
					break;
				}
				case _DATA_TRANSFER_FAILURE:{
					char res[150];
					if(startsWith(buf, "RETR")){
						strcpy(res, "451 file not exist.\r\n");
					}else if(startsWith(buf, "STOR")){
						strcpy(res, "451 the server had trouble writing to the disk.\r\n");
					}else if(startsWith(buf, "LIST")){
						strcpy(res, "451 the server had trouble reading from the disk.\r\n");
					}
					write(info->connfd, res, strlen(res));
					break;
				}
				default:{
					break;
				}
			}
			close(dataTransferConnectionFD);
			exit(0);
		}else{
			char res[] = "425 no TCP connection established for data transfer.\r\n";
			write(info->connfd, res, strlen(res));
			exit(0);
		}
	}

	//third stage
	if(info->ftsInfo.inPASVmode || info->ftsInfo.inPORTmode){
		if(info->ftsInfo.inPASVmode){
			info->ftsInfo.inPASVmode = 0;
			close(info->ftsInfo.PASV_INFO.dataSocketFD);
		}
		if(info->ftsInfo.inPORTmode){
			info->ftsInfo.inPORTmode = 0;
			close(info->ftsInfo.PORT_INFO.dataSocketFD);
		}
	}
}

int gen_port(int *sockfd, int listenOrNot){
	/**
	 * 找到一个可用的端口号，在其上绑定相应的socket并返回
	 * @param sockfd 若绑定成功，sockfd所指向的int
	 * 便被设为新绑定的socket的file descriptor
	 * @return 若绑定成功，返回端口号；否则返回-1
	*/
	int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in addr;

	// random permutation of available port numbers
	const int num = 65535-20000+1;
	int port[num];
	for(int i=0;i<num;i++){
		port[i]=20000+i;
	}
	int n = num;
	srand(time(NULL));
	while(n>0){
		int index = rand()%n;
		int tmp = port[n-1];
		port[n-1] = port[index];
		port[index] = tmp;
		n--;
	}

	// try every available port number until success
	for (int i = 0; i < num; i++){
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port[i]); 
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		if(bind(fd, (struct sockaddr*)&addr, sizeof(addr))==-1){ //bind failure
			continue;
		}else{

			if(listenOrNot){
				if(listen(fd, 10)==-1){ // listen failure
					close(fd);
					continue;
				}else{
					*sockfd = fd;
					return port[i];
				}
			}else{
				*sockfd = fd;
				return port[i];
			}
		}
	}

	// all ports fail
	return -1;
}

int getip(int sockfd, int* ip){
	/**
	 * 获取sockfd对应的ip地址
	 * @param ip 4维int数组，用于存储ip地址
	 * @return 获取成功则返回0，否则返回1。
	*/
	socklen_t addr_size = sizeof(struct sockaddr_in);
	struct sockaddr_in addr;
	if(getsockname(sockfd, (struct sockaddr *)&addr, &addr_size)!=0){
		return 1;
	}
	
	char* host = inet_ntoa(addr.sin_addr);
	sscanf(host,"%d.%d.%d.%d",&ip[0],&ip[1],&ip[2],&ip[3]);
	return 0;
}

int ftp_retr(int dataTransferSockFD, char* buf, struct connectionInfo* info){
	/**
	 * @return 若操作成功，返回_DATA_TRANSFER_SUCCESS；若fn以"../"开头，返回_NAME_START_WITH_ILLEGAL_DOUBLE_DOT；若文件不存在，返回_DATA_TRANSFER_FAILURE 
	*/

	char fn[MAXLINE]; //filename
	strcpy(fn, buf+5);
	fn[strlen(fn)-2]='\0';
	char* filename = (char*) malloc (strlen(fn)+strlen(info->dir)+20);
	int result = getAbsoluteFilePath(fn, info->dir, filename);
	if(result!=_FILE_EXIST){
		free(filename);
		if(result==_NAME_START_WITH_ILLEGAL_DOUBLE_DOT){
			return _NAME_START_WITH_ILLEGAL_DOUBLE_DOT;
		}else{
			return _DATA_TRANSFER_FAILURE;
		}
	}else{
		int fd = open(filename, O_RDONLY);
		off_t offset = info->start_position;
		struct stat stats;
		if(stat(filename, &stats)!=0){
			free(filename);
			return _DATA_TRANSFER_FAILURE;
		}
		sendfile(dataTransferSockFD, fd, &offset, stats.st_size-offset);
		free(filename);
		close(fd);
		return _DATA_TRANSFER_SUCCESS;
	}
}

int ftp_list(int dataTransferSockFD, char* buf, struct connectionInfo* info){
	/**
	 * @return 若操作成功，返回_DATA_TRANSFER_SUCCESS；若文件或文件夹不存在，返回-2 
	*/
	if(strlen(buf)==strlen("LIST\r\n")){ // Case 1: LIST <CRLF>
		int result = listFiles(dataTransferSockFD, info->dir);
		if(result==_DATA_TRANSFER_SUCCESS){
			return _DATA_TRANSFER_SUCCESS;
		}else{
			return _DATA_TRANSFER_FAILURE;
		}
	}else{ // Case 2: LIST <SP> <pathname> <CRLF>
		char* tmp_ = (char*) malloc (strlen(buf)-4);
		strcpy(tmp_, buf+5);
		tmp_[strlen(tmp_)-2]='\0';
		if(isDirectoryExists(tmp_, info->dir, 0)==0){ // Case 2-1: <pathname> corresponds to a directory
			char dir[MAXLINE];
			strcpy(dir, info->dir);
			if(dir[strlen(dir)-1]!='/'){
				strcat(dir, "/");
			}
			strcat(dir, buf+5);
			dir[strlen(dir)-2]='\0';
			int result = listFiles(dataTransferSockFD, dir);
			if(result==_DATA_TRANSFER_SUCCESS){
				return _DATA_TRANSFER_SUCCESS;
			}else{
				return _DATA_TRANSFER_FAILURE;
			}
		}else{ // Case 2-2: <pathname> corresponds to a file
			int result = getFileProperties(tmp_, dataTransferSockFD, info->dir);
			if(result==_DATA_TRANSFER_SUCCESS){
				return _DATA_TRANSFER_SUCCESS;
			}else{
				return _DATA_TRANSFER_FAILURE;
			}
		}
		free(tmp_);
	}
	return 0;
}

int ftp_stor(int dataTransferSockFD, char* buf, struct connectionInfo* info){
	/**
	 * @return 若操作成功，返回_DATA_TRANSFER_SUCCESS；若操作失败，返回_DATA_TRANSFER_FAILURE
	*/
	char fn[MAXLINE]; //filename
	strcpy(fn, buf+5);
	fn[strlen(fn)-2]='\0';
	char* filename = (char*) malloc (strlen(fn)+strlen(info->dir)+20);
	strcpy(filename, info->dir);
	if(filename[strlen(filename)-1]!='/'){
		strcat(filename, "/");
	}
	strcat(filename, fn);
	FILE* fp = fopen(filename, "w");
	if(fp == NULL){
		return _DATA_TRANSFER_FAILURE;
	}else{
		int fd = fileno(fp);
		int pipefd[2];
		if(pipe(pipefd)==-1){
			return _DATA_TRANSFER_FAILURE;
		}
		int result;
		lseek(fd, info->start_position, SEEK_SET);
		while((result=splice(dataTransferSockFD, 0, pipefd[1], NULL, MAXLINE, SPLICE_F_MORE | SPLICE_F_MOVE)>0)){
			splice(pipefd[0], NULL, fd, 0, MAXLINE, SPLICE_F_MORE | SPLICE_F_MOVE);
		}
		close(fd);
		if(result==-1){
			return _DATA_TRANSFER_FAILURE;
		}else{
			return _DATA_TRANSFER_SUCCESS;
		}
	}
}
