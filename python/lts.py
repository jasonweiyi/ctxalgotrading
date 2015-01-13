# -*- coding: utf-8 -*-
__author__ = 'Chunyou'

from base_api.api_manager import *

os.environ['PATH'] = ';'.join([os.path.dirname(__file__) + "\\include",
                               os.path.dirname(__file__) + "\\include\\LTS_Stock\\win32",
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
            attr = getattr(self.api, item)

            def wrapper(**kwargs):
                self.api.invoke_log('on_invoke_market_%s' % item, **kwargs)
                attr(**kwargs)
            return wrapper if 'instancemethod' in str(type(attr)) else attr

    def release(self):
        self.api.invoke_log('invoke_market_disconnect')
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
            attr = getattr(self.api, item)

            def wrapper(**kwargs):
                self.api.invoke_log('on_invoke_trading_%s' % item, **kwargs)
                attr(**kwargs)
            return wrapper if 'instancemethod' in str(type(attr)) else attr

    def release(self):
        self.api.invoke_log('on_invoke_trading_disconnect')
        ApiManager.release(self.api)
