# -*- coding: utf-8 -*-
__author__ = 'Chunyou'

from base_api.api_manager import *

os.environ['PATH'] = ';'.join([os.path.dirname(__file__) + "\\..\\Debug",
                               os.path.dirname(__file__) + "\\include\\LTS\\win32",
                               os.environ['PATH']])


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
        self.api, self.queue = ApiManager.create("QuantBox_LTS_Quote", local_path,
                                                 server_address, broker_id, investor_id, password)
        self.api.set_callbacks(callbacks)

    def __getattr__(self, item):
        if self.api:
            return getattr(self.api, item)

    def release(self):
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
        self.api, self.queue = ApiManager.create("QuantBox_LTS_Trade", local_path,
                                                 server_address, broker_id, investor_id, password, is_market=False)
        self.api.set_callbacks(callbacks)

    def __getattr__(self, item):
        if self.api:
            return getattr(self.api, item)

    def release(self):
        ApiManager.release(self.api)
