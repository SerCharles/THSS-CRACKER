# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'login.ui'
#
# Created by: PyQt5 UI code generator 5.13.1
#
# WARNING! All changes made in this file will be lost!


from PyQt5 import QtCore, QtWidgets
from client import ftpClient as FTP
from mainWindow import Ui_mainWindow
import sys
from PyQt5.QtWidgets import QApplication
import argparse

class Ui_loginWindow(QtWidgets.QWidget):

    def __init__(self, ftp, parent=None, host='127.0.0.1', port=21):
        super(Ui_loginWindow, self).__init__(parent)
        self.ftp = ftp
        self.setupUi(self)
        self.pushButton.clicked.connect(self.openMainWindow)
        self.pushButton
        self.host = host
        self.port = port

    def setupUi(self, loginWindow):
        loginWindow.setObjectName("loginWindow")
        loginWindow.resize(800, 800)
        loginWindow.setMinimumSize(QtCore.QSize(800, 800))
        self.pushButton = QtWidgets.QPushButton(loginWindow)
        self.pushButton.setGeometry(QtCore.QRect(110, 270, 201, 51))
        self.pushButton.setObjectName("pushButton")
        self.label = QtWidgets.QLabel(loginWindow)
        self.label.setGeometry(QtCore.QRect(50, 110, 67, 17))
        self.label.setObjectName("label")
        self.label_2 = QtWidgets.QLabel(loginWindow)
        self.label_2.setGeometry(QtCore.QRect(50, 170, 67, 17))
        self.label_2.setObjectName("label_2")
        self.accountInput = QtWidgets.QLineEdit(loginWindow)
        self.accountInput.setGeometry(QtCore.QRect(130, 110, 211, 25))
        self.accountInput.setObjectName("accountInput")
        self.passwdInput = QtWidgets.QLineEdit(loginWindow)
        self.passwdInput.setGeometry(QtCore.QRect(130, 170, 211, 25))
        self.passwdInput.setObjectName("passwdInput")
        self.passwdInput.setEchoMode(QtWidgets.QLineEdit.Password)
        self.retranslateUi(loginWindow)
        QtCore.QMetaObject.connectSlotsByName(loginWindow)

    def retranslateUi(self, loginWindow):
        _translate = QtCore.QCoreApplication.translate
        loginWindow.setWindowTitle(_translate("loginWindow", "Form"))
        self.pushButton.setText(_translate("loginWindow", "Login"))
        self.label.setText(_translate("loginWindow", "Account"))
        self.label_2.setText(_translate("loginWindow", "Passwd"))
        self.accountInput.setText(_translate("loginWindow", "anonymous"))
        self.passwdInput.setText(_translate("loginWindow", "anonymous@"))

    def openMainWindow(self):
        # Test connection
        result = self.ftp.connect()
        if result.isSuccess:
            username = self.accountInput.text()
            passwd = self.passwdInput.text()
            result = self.ftp.login(username=username, passwd=passwd)
            if result.isSuccess:
                mainWindow = Ui_mainWindow(parent=self, ftp=self.ftp)
                print("mainWindow!")
                mainWindow.resize(1600,2200)
                mainWindow.show()
            else:
                QtWidgets.QMessageBox.warning(self, 'Warning', "Wrong username or password", QtWidgets.QMessageBox.Yes)
        else:
            QtWidgets.QMessageBox.warning(self, 'Warning', "Connection refused", QtWidgets.QMessageBox.Yes)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--port', type=int, default=21)
    parser.add_argument('--host', default='127.0.0.1')
    args = parser.parse_args()

    ftp=FTP(host = args.host, port = args.port)

    app = QApplication(sys.argv)
    loginWindow = Ui_loginWindow(parent=None, ftp=ftp)
    loginWindow.show()
    sys.exit(app.exec_())
