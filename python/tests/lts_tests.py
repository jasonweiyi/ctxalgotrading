# -*- coding: utf-8 -*-
import unittest
import time
from base_api.callbacks import MarketCallbacks, TraderCallbacks
from lts import LtsMarket, LtsTrader

__author__ = 'Chunyou'


class LtsMarketCallbacks(MarketCallbacks):
    market_data = []

    def on_connect_status(self, status, rsp_user_login):
        print "Status: %s" % ord(status)

    def on_rtn_depth_market_data(self, depth_market_data):
        self.market_data.append(depth_market_data)
        print(str(depth_market_data.InstrumentID) + "  " + str(depth_market_data.AskPrice1) + " " + str(
            depth_market_data.UpdateTime))


class LtsMarketTests(unittest.TestCase):
    instrument_ids = b"sh600001"

    def setUp(self):
        self.callbacks = LtsMarketCallbacks()
        self.api = LtsMarket(b"C:/tmp/lts", b"tcp://211.144.195.163:44513", b"2011", b"020090032917", b"123321",
                             self.callbacks)
        self.api.connect()

    def tearDown(self):
        self.api.release()

    # 处理消息队列
    def process(self, max_wait=60, stop_condition=lambda: False):
        itr = 0
        while itr < max_wait and not stop_condition():
            itr += 1
            time.sleep(1)

    def test_connect(self):
        self.process(5, lambda: self.api.is_connected)
        self.assertTrue(self.api.is_connected, msg='Login Failed!')

    def test_depth_market_data(self):
        # self.test_connect()
        self.api.subscribe(self.instrument_ids)
        self.process(stop_condition=lambda: len(self.callbacks.market_data) > 0)
        self.api.unsubscribe(self.instrument_ids)
        self.assertNotEqual(len(self.callbacks.market_data), 0, msg='Get Depth Market Data Failed!')


class LtsTraderCallbacks(TraderCallbacks):
    def on_connect_status(self, status, rsp_user_login):
        print "Status: %s" % ord(status)


class LtsTraderTests(unittest.TestCase):
    def setUp(self):
        self.callbacks = LtsTraderCallbacks()
        self.api = LtsTrader(b"C:/tmp/lts", b"tcp://211.144.195.163:44513", b"2011", b"020090032917", b"123321",
                             self.callbacks)
        self.api.connect()

    def tearDown(self):
        self.api.release()

    # 处理消息队列
    def process(self, max_wait=60, stop_condition=lambda: False):
        itr = 0
        while itr < max_wait and not stop_condition():
            itr += 1
            time.sleep(1)

    def test_connect(self):
        self.process(5, lambda: self.api.is_connected)
        self.assertTrue(self.api.is_connected, msg='Login Failed!')


if __name__ == '__main__':
    unittest.main()