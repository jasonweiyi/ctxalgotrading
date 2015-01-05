# -*- coding: utf-8 -*-
from base_api.comm import *

__author__ = 'Chunyou'

"""
An proxy for dll function call
"""


class Proxy(object):
    _invoke = None
    _xRequest = None

    def __init__(self, path):
        self._invoke = WinDLL(path)

    def x_request(self, request_type, p_api1=None, p_api2=None, double1=0, double2=0, ptr1=None, size1=0, ptr2=None,
                  size2=0, ptr3=None, size3=0):
        if self._xRequest is None:
            if self._invoke is None:
                return None
            self._xRequest = self._invoke.XRequest
            self._xRequest.restype = c_void_p
            self._xRequest.argtypes = [RequestType, c_void_p, c_void_p, c_double, c_double, c_void_p, c_int, c_void_p,
                                       c_int, c_void_p, c_int]
            if self._xRequest is None:
                return None
        # print "Request: ", ord(request_type.value)
        return self._xRequest(request_type, p_api1, p_api2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3)