import socket
import os


class responseToCmd:
    def __init__(self, isSuccess, text):
        self.isSuccess = isSuccess
        self.text = text

    def getText(self):
        return self.text


class ftpClient:
    maxline = 4096
    CRLF = '\r\n'
    encoding = 'utf-8'

    def __init__(self, host, port, inPasvMode=True):
        self.host = host
        self.port = port
        self.inPasvMode = inPasvMode
        self.dataSocket = None
        pass

    def set_port(self, portMode: bool):
        self.inPasvMode = not portMode

    def processSpace(self, arg):
        '''
        处理ftp命令的参数中的空格
        '''
        if arg is None:
            return arg
        result = ""
        for character in arg:
            if character == ' ':
                result += '\ '
            else:
                result += character
        return result

    def connect(self, timeout: int = 999):
        if timeout > 0:
            self.timeout = timeout
        try:
            self.socket = socket.create_connection((self.host, self.port), self.timeout)
        except BaseException as e:
            return responseToCmd(False, '')
        self.af = self.socket.family
        self.file = self.socket.makefile(mode='r', encoding=self.encoding)
        resp = self.getResponse()
        return responseToCmd(resp.startswith("220"), resp)

    def readline(self):
        line = self.file.readline(self.maxline + 1)  # make room for null terminator
        return line

    def getResponse(self):
        newline = self.readline()
        line = newline
        while newline[3:4] == '-':
            newline = self.readline()
            line = line + '\n' + newline
        print("get resp:\n***\n" + line + "***\n")
        return line

    def sendRequest(self, cmd):
        if '\r' in cmd or '\n' in cmd:
            raise ValueError("cmd should not contain any newline character")
        # process space in cmd arg:
        if cmd.startswith("RETR ") or cmd.startswith("STOR ") or cmd.startswith("LIST ") or cmd.startswith("MKD ") or cmd.startswith("RMD ") or \
            cmd.startswith("CWD ") or cmd.startswith("RNFR ") or cmd.startswith("RNTO "):
            idx = cmd.find(" ")
            cmd = cmd[:idx+1] + self.processSpace(cmd[idx+1:])
        cmd = cmd + self.CRLF
        print("send:\n***\n" + cmd + "***\n")
        self.socket.sendall(cmd.encode(self.encoding))

    def login(self, username='anonymous', passwd=None):
        resp = self.__ftp_user(username)
        if resp.isSuccess:
            resp = self.__ftp_pass(passwd)
        return resp

    def __ftp_user(self, username):
        self.sendRequest("USER " + username)
        resp = self.getResponse()
        if resp.startswith("230") or resp.startswith("331") or resp.startswith("332"):
            return responseToCmd(True, resp)
        else:
            return responseToCmd(False, resp)

    def __ftp_pass(self, passwd=None):
        if passwd is None:
            passwd = "anonymous@"
        self.sendRequest("PASS " + passwd)
        resp = self.getResponse()
        if resp.startswith("2") or resp.startswith("3"):
            return responseToCmd(True, resp)
        else:
            return responseToCmd(False, resp)

    def ftp_retr(self, cmdArg, callback=None):
        cmdArg = cmdArg.replace(" ","\ ")
        self.ftp_type()
        result = None
        if self.inPasvMode:
            result = self.__ftp_pasv()
        else:
            result = self.__ftp_port()
        try:
            f = open(os.path.join(os.getcwd(), cmdArg))
        except:
            pass
        else:
            self.sendRequest("REST " + str(os.path.getsize(os.path.join(os.getcwd(), cmdArg))))
            f.close()
            resp = self.getResponse()
            if resp[0] not in ['1', '2', '3']:
                return responseToCmd(False, resp)
        if callback is None:
            callback = open(os.path.join(os.getcwd(), cmdArg), "ab").write
        if result.isSuccess:
            self.sendRequest("RETR " + cmdArg)
            resp = self.getResponse()
            if resp.startswith("1") or resp.startswith("2"):
                if not self.inPasvMode:
                    conn = self.dataSocket.accept()[0]
                    self.dataSocket.close()
                    self.dataSocket = conn
                while True:
                    data = self.dataSocket.recv(self.maxline)
                    if not data:
                        break
                    callback(data)
                if self.dataSocket:
                    self.dataSocket.close()
                    self.dataSocket = None
                return responseToCmd(True, self.getResponse())
            else:
                if self.dataSocket:
                    self.dataSocket.close()
                    self.dataSocket = None
                return responseToCmd(False, resp)
        else:
            return result

    def ftp_stor(self, cmdArg, file=None):
        if file is None:
            file = open(os.path.join(os.getcwd(), cmdArg), "rb")
        self.ftp_type()
        result = None
        if self.inPasvMode:
            result = self.__ftp_pasv()
        else:
            result = self.__ftp_port()
        if result.isSuccess:
            self.sendRequest("STOR " + cmdArg)
            resp = self.getResponse()
            if resp.startswith("1") or resp.startswith("2"):
                if not self.inPasvMode:
                    conn = self.dataSocket.accept()[0]
                    self.dataSocket.close()
                    self.dataSocket = conn
                while True:
                    data = file.read(self.maxline)
                    if not data:
                        break
                    self.dataSocket.sendall(data)
                if self.dataSocket:
                    self.dataSocket.close()
                    self.dataSocket = None
                return responseToCmd(True, self.getResponse())
            else:
                if self.dataSocket:
                    self.dataSocket.close()
                    self.dataSocket = None
                return responseToCmd(False, resp)
        else:
            return result

    def ftp_quit(self):  # OK!
        self.sendRequest("QUIT")
        resp = self.getResponse()
        if resp.startswith("2"):
            if self.dataSocket is not None:
                self.dataSocket.close()
                self.dataSocket = None
            self.socket.close()
            self.socket = None
            return responseToCmd(True, resp)
        else:
            return responseToCmd(False, resp)

    def ftp_syst(self):
        self.sendRequest("SYST")
        resp = self.getResponse()
        if resp.startswith("2"):
            return responseToCmd(True, resp)
        else:
            return responseToCmd(False, resp)

    def ftp_type(self):
        self.sendRequest("TYPE I")
        resp = self.getResponse()
        if resp.startswith("2"):
            return responseToCmd(True, resp)
        else:
            return responseToCmd(False, resp)

    def __ftp_port(self):
        # create a socket at first:
        for res in socket.getaddrinfo(None, 0, self.af, socket.SOCK_STREAM, 0, socket.AI_PASSIVE):
            af, socktype, proto, canonname, sa = res
            try:
                sock = socket.socket(af, socktype, proto)
                sock.bind(sa)
            except OSError as _:
                err = _
                if sock:
                    sock.close()
                sock = None
                continue
            break
        sock.listen(1)
        self.dataSocket = sock
        port = sock.getsockname()[1]
        host = self.socket.getsockname()[0]
        request = "PORT " + ",".join(host.split(".") + [str(port // 256), str(port % 256)])
        self.sendRequest(request)
        resp = self.getResponse()
        if resp.startswith("2"):
            return responseToCmd(True, resp)
        else:
            if self.dataSocket is not None:
                self.dataSocket.close()
                self.dataSocket = None
            return responseToCmd(False, resp)

    def __ftp_pasv(self):
        self.sendRequest("PASV")
        resp = self.getResponse()
        return_value = None
        if resp.startswith("2"):
            return_value = responseToCmd(True, resp)
            pos1 = resp.find("(")
            pos2 = resp.find(")")
            resp = resp[pos1 + 1:pos2]
            resp = resp.split(",")
            host = ".".join(resp[:4])
            port = 256 * int(resp[4]) + int(resp[5])
            self.dataSocket = socket.create_connection((host, port), self.timeout)
        else:
            return_value = responseToCmd(False, resp)
        return return_value

    def ftp_mkd(self, cmdArg):
        self.sendRequest("MKD " + cmdArg)
        resp = self.getResponse()
        if resp.startswith("2"):
            return responseToCmd(True, resp)
        else:
            return responseToCmd(False, resp)

    def ftp_cwd(self, cmdArg):
        self.sendRequest("CWD " + cmdArg)
        resp = self.getResponse()
        if resp.startswith("2"):
            return responseToCmd(True, resp)
        else:
            return responseToCmd(False, resp)

    def ftp_pwd(self):
        self.sendRequest("PWD")
        resp = self.getResponse()
        if resp.startswith("2"):
            return responseToCmd(True, resp)
        else:
            return responseToCmd(False, resp)

    def ftp_list(self, cmdArg=None):
        self.ftp_type()
        result = None
        if self.inPasvMode:
            result = self.__ftp_pasv()
        else:
            result = self.__ftp_port()
        if result.isSuccess:
            request = "LIST"
            if cmdArg:
                request = request + " " + cmdArg
            self.sendRequest(request)
            resp = self.getResponse()
            if resp.startswith("1") or resp.startswith("2"):
                if not self.inPasvMode:
                    conn = self.dataSocket.accept()[0]
                    self.dataSocket.close()
                    self.dataSocket = conn
                received = ''.encode('utf-8')
                while True:
                    data = self.dataSocket.recv(self.maxline)
                    if not data:
                        break
                    received = received + data
                if self.dataSocket:
                    self.dataSocket.close()
                    self.dataSocket = None
                resp = self.getResponse()
                if resp[0] in ['1','2','3']:
                    return responseToCmd(True, received.decode('utf-8'))
                else:
                    return responseToCmd(True, resp)
            else:
                if self.dataSocket:
                    self.dataSocket.close()
                    self.dataSocket = None
                return responseToCmd(False, resp)
        else:
            return result

    def ftp_rmd(self, cmdArg):
        self.sendRequest("RMD " + cmdArg)
        resp = self.getResponse()
        if resp.startswith("2"):
            return responseToCmd(True, resp)
        else:
            return responseToCmd(False, resp)

    def ftp_rename(self, oldName, newName):
        result = self.__ftp_rnfr(oldName)
        if result.isSuccess:
            result = self.__ftp_rnto(newName)
            return result
        else:
            return result

    def __ftp_rnfr(self, cmdArg):
        self.sendRequest("RNFR " + cmdArg)
        resp = self.getResponse()
        if resp[0] in ['1', '2', '3']:
            return responseToCmd(True, resp)
        else:
            return responseToCmd(False, resp)

    def __ftp_rnto(self, cmdArg):
        self.sendRequest("RNTO " + cmdArg)
        resp = self.getResponse()
        if resp[0] in ['1', '2', '3']:
            return responseToCmd(True, resp)
        else:
            return responseToCmd(False, resp)