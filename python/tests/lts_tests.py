# -*- coding: utf-8 -*-
import unittest
import time
from base_api.callbacks import CallBacks
from base_api.comm import *
from logger.file_logger import FileLogger
# from lts_backtesting import LtsMarketBacktesting as LtsMarket, LtsTraderBacktesting as LtsTrader
from lts import LtsMarket, LtsTrader
__author__ = 'Chunyou'


class LtsMarketCallbacks(CallBacks):
    market_data = []

    def on_market_connected(self, p_api, rsp_user_login, status):
        print "Status: %s" % ord(status)
        if status == Disconnected.value:
            print rsp_user_login.ErrorMsg.decode('gbk')

    def on_market_rtn_depth_market_data(self, p_api, depth_market_data):
        self.market_data.append(depth_market_data)
        print(str(depth_market_data.InstrumentID) + "  " + str(depth_market_data.AskPrice1) + " " + str(
            depth_market_data.UpdateTime))

    def on_market_rsp_error(self, p_api, rsp_info, b_is_last):
        print rsp_info.ErrorMsg


class LtsMarketTests(unittest.TestCase):
    instrument_ids = [b"600109"]
    exchange_id = b"SSE"

    def setUp(self):
        self.callbacks = LtsMarketCallbacks()
        self.market = LtsMarket(b"C:/tmp/lts", b"tcp://211.144.195.163:44513", b"2011", b"020000000350",
                                b"123", self.callbacks)
        self.market.connect()

    def tearDown(self):
        self.market.release()

    # 处理消息队列
    def process(self, max_wait=60, stop_condition=lambda: False):
        itr = 0
        while itr < max_wait and not stop_condition():
            if self.market.queue.process_first_message():
                continue
            itr += 1
            time.sleep(1)

    def test_connect(self):
        self.process(5, lambda: self.market.is_connected)
        self.assertTrue(self.market.is_connected, msg='Login Failed!')

    def test_depth_market_data(self):
        self.test_connect()
        self.market.subscribe(instrument_ids=self.instrument_ids, exchange_id=self.exchange_id)
        self.process(stop_condition=lambda: len(self.callbacks.market_data) > 10)
        self.market.unsubscribe(self.instrument_ids)
        self.assertNotEqual(len(self.callbacks.market_data), 1, msg='Get Depth Market Data Failed!')


class LtsTraderCallbacks(CallBacks):
    trading_account = None
    future_detail = None
    investor_position = None
    order = None
    cancel_order = False

    def on_trading_connected(self, p_api, rsp_user_login, status):
        print "Status: %s" % ord(status)

    def on_trading_rsp_qry_trading_account(self, p_api, trading_account, b_is_last):
        print '\nAccount Info: '
        for k, v in trading_account._fields_:
            print k, getattr(trading_account, k)
        if b_is_last:
            self.trading_account = trading_account

    def on_trading_rsp_error(self, p_api, rsp_info, b_is_last):
        print("Error:" + str(rsp_info.ErrorID) + ", Msg:" + rsp_info.ErrorMsg.decode('gbk'))

    def on_trading_rsp_qry_instrument(self, p_api, instrument, b_is_last):
        """
        请求查询合约响应。当客户端发出请求查询合约指令后，交易托管系统返回响应时，该方法会被调用。
        :param p_api: c_void_p
        :param instrument: InstrumentField
        :param b_is_last: boolean, True if it is the last response for this request
        :return:
        """
        print '\nInstrument: '
        for k, v in instrument._fields_:
            print k, getattr(instrument, k).decode('gbk') if k == 'InstrumentName' else (ord(getattr(instrument, k)) if k in ['Type', 'OptionsType'] else getattr(instrument, k))
        if b_is_last:
            self.future_detail = instrument

    def on_trading_rsp_qry_investor_position(self, p_api, investor_position, b_is_last):
        """
        投资者持仓查询应答。当客户端发出投资者持仓查询指令后，后交易托管系统返回响应时，该方法会被调用。
        :param p_api: c_void_p
        :param investor_position: PositionField
        :param b_is_last: boolean, True if it is the last response for this request
        :return:
        """
        print '\nInvestor Position: '
        for k, v in investor_position._fields_:
            print k, getattr(investor_position, k)
        if b_is_last:
            self.investor_position = investor_position

    def on_trading_rtn_order(self, p_api, order):
        """
        报单回报。当客户端进行报单录入、报单操作及其它原因（如部分成交）导致报单状态发生变化时，交易托管系统会主动通知客户端，该方法会被调用。
        :param p_api: c_void_p
        :param order: OrderField
        """
        print '\nOrder: '
        for k, v in order._fields_:
            print k, ":", getattr(order, k).decode('gbk') if k == 'Text' else (ord(getattr(order, k)) if k in ['Type', 'Side', 'OpenClose', 'HedgeFlag', 'TimeInForce', 'Status', 'ExecType'] else getattr(order, k))
        if order.Status in [New, PartiallyFilled, Filled]:
            self.order = order
        elif order.Status in [Cancelled]:
            self.cancel_order = True

    def on_trading_rtn_trade(self, p_api, trade):
        """
        成交回报。当发生成交时交易托管系统会通知客户端，该方法会被调用。
        :param p_api: c_void_p
        :param trade:TradeField
        :return:
        """
        print '\nTrade: '
        for k, v in trade._fields_:
            print k, getattr(trade, k)


class LtsTraderTests(unittest.TestCase):
    future_ids = b"600109"
    exchange_id = b"SSE"

    def setUp(self):
        self.callbacks = [LtsTraderCallbacks(), FileLogger(b"C:/tmp/log")]
        self.trading = LtsTrader(b"C:/tmp/lts", b"tcp://211.144.195.163:44505", b"2011", b"020000000350",
                                 b"123321", self.callbacks)
        self.trading.connect()
        # self.api.queue.start_pulling()

    def tearDown(self):
        self.trading.release()
        self.callbacks[1].flush()

    # 处理消息队列
    def process(self, max_wait=10, stop_condition=lambda: False):
        itr = 0
        while itr < max_wait and not stop_condition():
            if self.trading.queue.process_first_message():
                continue
            itr += 1
            time.sleep(1)

    def test_connect(self):
        self.process(5, lambda: self.trading.is_connected)
        self.assertTrue(self.trading.is_connected, msg='Login Failed!')

    def test_get_trading_account(self):
        self.test_connect()
        self.trading.get_trading_account()
        self.process(10, stop_condition=lambda: self.callbacks[0].trading_account is not None)
        self.assertNotEqual(self.callbacks[0].trading_account, None, msg='Get Trading Account Failed!')

    def test_get_future(self):
        self.test_connect()
        self.trading.get_instrument(future_id=self.future_ids, exchange_id=self.exchange_id)
        self.process(stop_condition=lambda: self.callbacks[0].future_detail is not None)
        self.assertNotEqual(self.callbacks[0].future_detail, None, msg='Get Future Failed!')

    def test_get_investor_position(self):
        """
        Works only afer querying trading account or send orders.
        """
        self.test_connect()
        self.trading.get_trading_account()
        self.trading.get_investor_position(future_id=self.future_ids, exchange_id=self.exchange_id)
        self.process(stop_condition=lambda: self.callbacks[0].investor_position is not None)
        self.assertNotEqual(self.callbacks[0].investor_position, None, msg='Get Investor Position info Failed!')

    def test_order(self):
        self.test_connect()
        order_id = self.trading.send_order(order_ref=-1, future_id=self.future_ids, exchange_id=self.exchange_id,
                                           order_type=Limit, time_in_force=Day, side=Buy, open_close=Open,
                                           hedge_flag=Hedge, qty=1000, price=19.5, stop_price=20)
        print '\nOrder Id: %s' % order_id
        self.process(
            stop_condition=lambda: self.callbacks[0].order is not None and self.callbacks[0].order.ID == order_id)
        self.assertNotEqual(self.callbacks[0].order, None, msg='Send Order Failed!')


if __name__ == '__main__':
    unittest.main()