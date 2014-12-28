# -*- coding: utf-8 -*-
__author__ = 'Chunyou'

from base_api.api_manager import *

securitymduserapi = WinDLL(os.path.dirname(__file__) + "/include/LTS/win32/securitymduserapi.dll")
securitytraderapi = WinDLL(os.path.dirname(__file__) + "/include/LTS/win32/securitytraderapi.dll")


class LtsMarket(object):
    def __init__(self, local_path, server_address, broker_id, investor_id, password, callbacks):
        """
        Initialize market API with callbacks
        :param local_path: string, the local folder to store temp files.
        :param server_address: string, the server address to connect to.
        :param broker_id: string, Broker id.
        :param investor_id: string, investor id.
        :param password: string, investor password.
        """
        self.api = ApiManager.create(os.path.dirname(__file__) + "/include/LTS/QuantBox_LTS_Quote.dll", local_path,
                                     server_address, broker_id, investor_id, password)
        self.api.set_callbacks(callbacks)

    def __getattr__(self, item):
        if self.api:
            return getattr(self.api, item)

    def release(self):
        self.api.disconnect()
        ApiManager.release(self.api)


class LtsTrader(object):
    def __init__(self, local_path, server_address, broker_id, investor_id, password, callbacks):
        """
        Initialize trader API with callbacks
        :param local_path: string, the local folder to store temp files.
        :param server_address: string, the server address to connect to.
        :param broker_id: string, Broker id.
        :param investor_id: string, investor id.
        :param password: string, investor password.
        """
        self.api = ApiManager.create(os.path.dirname(__file__) + "/include/LTS/QuantBox_LTS_Trade.dll", local_path,
                                     server_address, broker_id, investor_id, password)
        self.api.set_callbacks(callbacks)

    def __getattr__(self, item):
        if self.api:
            return getattr(self.api, item)

    def release(self):
        self.api.disconnect()
        ApiManager.release(self.api)
