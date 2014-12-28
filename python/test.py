# -*- coding: utf-8 -*-
__author__ = 'Chunyou'

from ctypes import *
import os

os.environ['PATH'] = ';'.join(["D:\\EverBox\\work\\weiyi\\projects\\ctxalgotrading\\Release",
                               "D:\\EverBox\\work\\weiyi\\projects\\ctxalgotrading\\python\\include\\LTS\\win32",
                               "D:\\EverBox\\work\\weiyi\\projects\\ctxalgotrading\\include\\CTP\\win32",
                               os.environ['PATH']])

if __name__ == "__main__":
    WinDLL("QuantBox_Queue")
    WinDLL("securitymduserapi")
    WinDLL("securitytraderapi")
    WinDLL("thostmduserapi")
    print os.path.exists(
        "D:\\EverBox\\work\\weiyi\\projects\\ctxalgotrading\\Release\\QuantBox_LTS_Quote.dll")
    a = WinDLL("D:\\EverBox\\work\\weiyi\\projects\\ctxalgotrading\\Release\\QuantBox_LTS_Quote.dll")