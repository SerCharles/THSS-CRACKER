# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'mainWindow.ui'
#
# Created by: PyQt5 UI code generator 5.13.1
#
# WARNING! All changes made in this file will be lost!


from PyQt5 import QtCore, QtGui, QtWidgets
import os
from pathlib import Path

class openButton(QtWidgets.QPushButton):
    '''
    客户端文件夹显示区域的打开文件夹按钮。
    '''
    def __init__(self, folder_name, *kargs, **kwargs):
        super(openButton, self).__init__(*kargs, **kwargs)
        self.folder_name = folder_name
        self.father = kwargs['parent']
        self.clicked.connect(self.myClicked)

    def myClicked(self):
        if self.father.myDirChanged!=1:
            os.chdir(os.path.join(os.getcwd(), self.folder_name))
            self.father.myDirChanged = 1

class uploadButton(QtWidgets.QPushButton):
    '''
    客户端文件夹显示区域的上传按钮。
    '''
    def __init__(self, filename, *kargs, **kwargs):
        super(uploadButton, self).__init__(*kargs, **kwargs)
        self.filename = filename
        self.father = kwargs['parent']
        self.clicked.connect(self.myClicked)

    def myClicked(self):
        result = self.father.ftp.ftp_stor(self.filename)
        if result.isSuccess:
            QtWidgets.QMessageBox.information(self, "", result.text, QtWidgets.QMessageBox.Yes)
            self.father.serverDirChanged = 1
        else:
            QtWidgets.QMessageBox.warning(self, "Warning", result.text, QtWidgets.QMessageBox.Yes)

class cwdButton(QtWidgets.QPushButton):
    '''
    服务器端文件夹显示区域的打开文件夹按钮。
    '''
    def __init__(self, folder_name, *kargs, **kwargs):
        super(cwdButton, self).__init__(*kargs, **kwargs)
        self.folder_name = folder_name
        self.father = kwargs['parent']
        self.clicked.connect(self.myClicked)

    def myClicked(self):
        result = self.father.ftp.ftp_cwd(self.folder_name)
        if result.isSuccess:
            self.father.serverDirChanged = 1
        else:
            QtWidgets.QMessageBox.warning(self, "Warning", result.text, QtWidgets.QMessageBox.Yes)

class renameButton(QtWidgets.QPushButton):
    '''
        服务器端文件夹显示区域的打开文件夹按钮。
        '''

    def __init__(self, oldname, *kargs, **kwargs):
        super(renameButton, self).__init__(*kargs, **kwargs)
        self.oldname = oldname
        self.father = kwargs['parent']
        self.clicked.connect(self.myClicked)

    def myClicked(self):
        newname, okPressed = QtWidgets.QInputDialog.getText(self, "RENAME folder "+self.oldname, "new name:", QtWidgets.QLineEdit.Normal, "")
        if okPressed and newname != '':
            result = self.father.ftp.ftp_rename(oldName=self.oldname, newName=newname)
            if result.isSuccess:
                QtWidgets.QMessageBox.information(self, "", result.text, QtWidgets.QMessageBox.Yes)
                self.father.serverDirChanged = 1
            else:
                QtWidgets.QMessageBox.warning(self, "Warning", result.text, QtWidgets.QMessageBox.Yes)

class downloadButton(QtWidgets.QPushButton):
    '''
    服务器端文件夹显示区域的下载文件按钮
    '''
    def __init__(self, filename, *kargs, **kwargs):
        super(downloadButton, self).__init__(*kargs, **kwargs)
        self.filename = filename
        self.father = kwargs['parent']
        self.clicked.connect(self.myClicked)

    def myClicked(self):
        result = self.father.ftp.ftp_retr(self.filename)
        if result.isSuccess:
            QtWidgets.QMessageBox.information(self, "", result.text, QtWidgets.QMessageBox.Yes)
            self.father.myDirChanged = 1
        else:
            QtWidgets.QMessageBox.warning(self, "Warning", result.text, QtWidgets.QMessageBox.Yes)

class delButton(QtWidgets.QPushButton):
    '''
    服务器端文件夹显示区域的删除文件夹按钮
    '''
    def __init__(self, folder_name, *kargs, **kwargs):
        super(delButton, self).__init__(*kargs, **kwargs)
        self.folder_name = folder_name
        self.father = kwargs['parent']
        self.clicked.connect(self.myClicked)

    def myClicked(self):
        result = self.father.ftp.ftp_rmd(self.folder_name)
        if result.isSuccess:
            QtWidgets.QMessageBox.information(self, "", result.text, QtWidgets.QMessageBox.Yes)
            self.father.serverDirChanged = 1
        else:
            QtWidgets.QMessageBox.warning(self, "Warning", result.text, QtWidgets.QMessageBox.Yes)

class Ui_mainWindow(QtWidgets.QWidget):

    folder_icon_path = os.path.join(os.getcwd(),"img/folder.gif")
    file_icon_path = os.path.join(os.getcwd(),'img/file.jpg')

    def __init__(self, ftp, parent=None):
        super(Ui_mainWindow, self).__init__(parent)
        self.ftp = ftp
        self.myDirChanged = 1
        self.serverDirChanged = 1
        self.setupUi(self)
        self.comboBox.activated[int].connect(self.changeDataTransferMode)
        self.QUIT_BTN.clicked.connect(self.quit)
        self.MKD_BTN.clicked.connect(self.mkd)
        self.clientGotoParentFolder.clicked.connect(self.clientWDtoParent)
        self.serverGotoParentFolder.clicked.connect(self.serverWDtoParent)
        self.timer = QtCore.QTimer(self)
        self.timer.timeout.connect(self.update)
        self.timer.start(150)

    def update(self):
        self.loadMyDir()
        self.loadServerDir()

    def clientWDtoParent(self):
        new_wd = Path(os.getcwd()).parent
        os.chdir(new_wd)
        self.myDirChanged = 1

    def serverWDtoParent(self):
        result = self.ftp.ftp_pwd()
        if result.isSuccess:
            cwd = result.text.split("\"")[1]
            result = self.ftp.ftp_cwd(str(Path(cwd).parent))
            if result.isSuccess:
                self.serverDirChanged = 1
            else:
                QtWidgets.QMessageBox.warning(self, "Warning", result.text, QtWidgets.QMessageBox.Yes)
        else:
            QtWidgets.QMessageBox.warning(self, "Warning", result.text, QtWidgets.QMessageBox.Yes)

    def loadServerDir(self):
        if self.serverDirChanged:
            self.serverDirChanged = 0
            serverWD = self.ftp.ftp_pwd()
            if serverWD.isSuccess:
                self.serverWD.setText(serverWD.text.split("\"")[1])
            else:
                self.serverWD.setText("Failed to get current working directory")
            self.serverDir.setRowCount(0)
            self.serverDir.setColumnCount(5)
            result = self.ftp.ftp_list()
            if result.isSuccess:
                print(result.text)
                lines = result.text.split("\n")
                row = self.serverDir.rowCount()
                for line in lines:
                    idx = line.find(":")
                    if idx < 0:
                        continue
                    self.serverDir.insertRow(row)
                    name = line[idx+4:-1]
                    self.serverDir.setItem(row, 1, QtWidgets.QTableWidgetItem(name))
                    if line[0]=='d':
                        folder_label = QtWidgets.QLabel()
                        folder_label.setPixmap(QtGui.QPixmap(self.folder_icon_path).scaled(25, 20))
                        self.serverDir.setCellWidget(row, 0, folder_label)
                        btn = cwdButton(text="打开", parent=self, folder_name=name)
                        self.serverDir.setCellWidget(row, 2, btn)
                        btn2 = renameButton(text="命名", parent=self, oldname=name)
                        self.serverDir.setCellWidget(row, 3, btn2)
                        btn3 = delButton(text="删除", parent=self, folder_name=name)
                        self.serverDir.setCellWidget(row, 4, btn3)
                    else:
                        file_label = QtWidgets.QLabel()
                        file_label.setPixmap(QtGui.QPixmap(self.file_icon_path).scaled(25, 20))
                        self.serverDir.setCellWidget(row, 0, file_label)
                        btn = downloadButton(text="下载", parent=self, filename=name)
                        self.serverDir.setCellWidget(row, 2, btn)
                self.serverDir.horizontalHeader().setSectionResizeMode(1, QtWidgets.QHeaderView.Interactive)
            else:
                self.timer.stop()
                QtWidgets.QMessageBox.warning(self, "Warning", result.text, QtWidgets.QMessageBox.Yes)
                print("restart timer..")
                self.timer.start(150)

    def setupUi(self, mainWindow):
        mainWindow.setObjectName("mainWindow")
        mainWindow.resize(800, 800)
        mainWindow.setMinimumSize(QtCore.QSize(800, 800))
        mainWindow.setMaximumSize(QtCore.QSize(800, 800))
        self.comboBox = QtWidgets.QComboBox(mainWindow)
        self.comboBox.setGeometry(QtCore.QRect(180, 10, 131, 21))
        self.comboBox.setObjectName("comboBox")
        self.comboBox.addItem("")
        self.comboBox.addItem("")
        self.label = QtWidgets.QLabel(mainWindow)
        self.label.setGeometry(QtCore.QRect(30, 10, 141, 21))
        self.label.setObjectName("label")
        self.MKD_BTN = QtWidgets.QPushButton(mainWindow)
        self.MKD_BTN.setGeometry(QtCore.QRect(220, 50, 171, 25))
        self.MKD_BTN.setObjectName("MKD_BTN")
        self.serverDir = QtWidgets.QTableWidget(mainWindow)
        self.serverDir.setGeometry(QtCore.QRect(20, 90, 371, 511))
        self.serverDir.setObjectName("serverDir")
        self.serverDir.setColumnCount(0)
        self.serverDir.setRowCount(0)
        self.myDir = QtWidgets.QTableWidget(mainWindow)
        self.myDir.setGeometry(QtCore.QRect(410, 90, 371, 511))
        self.myDir.setObjectName("myDir")
        self.myDir.setColumnCount(0)
        self.myDir.setRowCount(0)
        self.QUIT_BTN = QtWidgets.QPushButton(mainWindow)
        self.QUIT_BTN.setGeometry(QtCore.QRect(410, 10, 161, 25))
        self.QUIT_BTN.setObjectName("QUIT_BTN")
        self.label_2 = QtWidgets.QLabel(mainWindow)
        self.label_2.setGeometry(QtCore.QRect(20, 620, 67, 17))
        self.label_2.setObjectName("label_2")
        self.serverWD = QtWidgets.QLabel(mainWindow)
        self.serverWD.setGeometry(QtCore.QRect(70, 620, 321, 17))
        self.serverWD.setObjectName("serverWD")
        self.label_4 = QtWidgets.QLabel(mainWindow)
        self.label_4.setGeometry(QtCore.QRect(410, 620, 67, 17))
        self.label_4.setObjectName("label_4")
        self.myWD = QtWidgets.QLabel(mainWindow)
        self.myWD.setGeometry(QtCore.QRect(460, 620, 321, 17))
        self.myWD.setObjectName("myWD")
        self.serverGotoParentFolder = QtWidgets.QPushButton(mainWindow)
        self.serverGotoParentFolder.setGeometry(QtCore.QRect(20, 50, 178, 25))
        self.serverGotoParentFolder.setObjectName("serverGotoParentFolder")
        self.clientGotoParentFolder = QtWidgets.QPushButton(mainWindow)
        self.clientGotoParentFolder.setGeometry(QtCore.QRect(410, 50, 178, 25))
        self.clientGotoParentFolder.setObjectName("clientGotoParentFolder")
        self.line = QtWidgets.QFrame(mainWindow)
        self.line.setGeometry(QtCore.QRect(0, 35, 791, 16))
        self.line.setFrameShape(QtWidgets.QFrame.HLine)
        self.line.setFrameShadow(QtWidgets.QFrame.Sunken)
        self.line.setObjectName("line")

        self.retranslateUi(mainWindow)
        QtCore.QMetaObject.connectSlotsByName(mainWindow)

    def retranslateUi(self, mainWindow):
        _translate = QtCore.QCoreApplication.translate
        mainWindow.setWindowTitle(_translate("mainWindow", "Form"))
        self.comboBox.setItemText(0, _translate("mainWindow", "PASV mode"))
        self.comboBox.setItemText(1, _translate("mainWindow", "PORT mode"))
        self.label.setText(_translate("mainWindow", "Data transfer Mode"))
        self.MKD_BTN.setText(_translate("mainWindow", "MKD"))
        self.QUIT_BTN.setText(_translate("mainWindow", "QUIT"))
        self.label_2.setText(_translate("mainWindow", "Path:"))
        self.serverWD.setText(_translate("mainWindow", "TextLabel"))
        self.label_4.setText(_translate("mainWindow", "Path:"))
        self.myWD.setText(_translate("mainWindow", "TextLabel"))
        self.serverGotoParentFolder.setText(_translate("mainWindow", "Server goto parent folder"))
        self.clientGotoParentFolder.setText(_translate("mainWindow", "Client goto parent folder"))

    def changeDataTransferMode(self, input):
        if input==0:
            self.ftp.set_port(False)
        else:
            self.ftp.set_port(True)

    def quit(self):
        self.ftp.ftp_quit()
        QtCore.QCoreApplication.instance().quit()

    def mkd(self):
        text, okPressed = QtWidgets.QInputDialog.getText(self, "MKD", "folder name:", QtWidgets.QLineEdit.Normal,"")
        if okPressed and text!='':
            result = self.ftp.ftp_mkd(text)
            if result.isSuccess:
                QtWidgets.QMessageBox.information(self, "", result.text, QtWidgets.QMessageBox.Yes)
                self.serverDirChanged = 1
                self.loadServerDir()

    def loadMyDir(self):
        if self.myDirChanged:
            self.myDirChanged = 0
            self.myWD.setText(os.getcwd())
            self.myDir.setRowCount(0)
            self.myDir.setColumnCount(3)
            for f in os.listdir(os.getcwd()):
                isDir = os.path.isdir(os.path.join(os.getcwd(),f))
                row = self.myDir.rowCount()
                self.myDir.insertRow(row)
                self.myDir.setItem(row, 1, QtWidgets.QTableWidgetItem(f))
                if isDir:
                    folder_label = QtWidgets.QLabel()
                    folder_label.setPixmap(QtGui.QPixmap(self.folder_icon_path).scaled(25, 20))
                    self.myDir.setCellWidget(row, 0, folder_label)
                    btn = openButton(text="打开", folder_name=f, parent=self)
                    self.myDir.setCellWidget(row, 2, btn)
                else:
                    file_label = QtWidgets.QLabel()
                    file_label.setPixmap(QtGui.QPixmap(self.file_icon_path).scaled(25, 20))
                    self.myDir.setCellWidget(row, 0, file_label)
                    btn = uploadButton(text="上传", filename=f, parent=self)
                    self.myDir.setCellWidget(row, 2, btn)
            self.myDir.horizontalHeader().setSectionResizeMode(QtWidgets.QHeaderView.Stretch)
            self.myDir.horizontalHeader().setSectionResizeMode(1, QtWidgets.QHeaderView.Interactive)

    def goToParentDir(self):
        os.chdir(Path(os.getcwd()).parent)
        self.myDirChanged = 1

