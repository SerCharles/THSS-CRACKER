# 运行环境
基于[GCC 8.3.0] on Linux的python3.6.8。
需要使用的python库有：PyQt5、sys、argparse、os、socket、pathlib等。PyQt5可以使用pip直接安装。

# 运行方式
首先启动相应ftp服务器，然后在命令行中键入:
```
python3.6 login.py --port **** 
```
其中\*\*\*\*表示实际的服务器端监听的端口号。至于host，默认使用'127.0.0.1'，如需使用其他host可以在上述命令中加上参数'--host \*\*\*\*'（****为实际的host）。