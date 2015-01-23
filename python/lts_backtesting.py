# -*- coding: utf-8 -*-
__author__ = 'Chunyou'

from base_api.api_manager import *
from base_api.api_base import AbstractApi


class LtsMarketBacktesting(AbstractApi):
    def __init__(self, local_path, server_address, broker_id, investor_id, password, callbacks):
        """
        Initialize market API with callbacks
        :param local_path: string, the local folder to store temp files.
        :param server_address: string, the server address to connect to.
        :param broker_id: string, Broker id.
        :param investor_id: string, investor id.
        :param password: string, investor password.
        """
        self.queue = ApiManager.create_queue()
        super(LtsMarketBacktesting, self).__init__(None, self.queue, local_path, server_address, broker_id, investor_id,
                                                   password, None, None, True)
        self.set_callbacks(callbacks)

    def connect(self):
        """
        Connect to the server.
        """
        self.invoke_log('on_invoke_market_connect')

    def disconnect(self):
        self.invoke_log('on_invoke_market_disconnect')

    def get_api_type(self):
        self.invoke_log('on_invoke_market_get_api_type')
        return MarketData

    def get_api_version(self):
        self.invoke_log('on_invoke_market_get_api_version')
        return '0.1'

    def get_api_name(self):
        self.invoke_log('on_invoke_market_get_api_name')
        return 'LtsMarket Backtesting'

    def subscribe(self, instrument_ids, exchange_id=b''):
        """
        Subscribe market data for the given instruments.
        :param instrument_ids: [string], list of instrument ids.
        :param exchange_id: string, exchange id
        """
        self.invoke_log('on_invoke_market_subscribe', instrument_ids=instrument_ids, exchange_id=exchange_id)

    def unsubscribe(self, instrument_ids, exchange_id=b''):
        """
         Un-subscribe market data for the given instruments.
        :param instrument_ids: [string], list of instrument ids.
        :param exchange_id: string, exchange id
        """
        self.invoke_log('on_invoke_market_unsubscribe', instrument_ids=instrument_ids, exchange_id=exchange_id)

    def release(self):
        self.disconnect()
        self.queue.disconnect()


class LtsTraderBacktesting(AbstractApi):
    def __init__(self, local_path, server_address, broker_id, investor_id, password, callbacks):
        """
        Initialize trader API with callbacks
        :param local_path: string, the local folder to store temp files.
        :param server_address: string, the server address to connect to.
        :param broker_id: string, Broker id.
        :param investor_id: string, investor id.
        :param password: string, investor password.
        """
        self.queue = ApiManager.create_queue()
        super(LtsTraderBacktesting, self).__init__(None, self.queue, local_path, server_address, broker_id, investor_id,
                                                   password, None, None, False)
        self.set_callbacks(callbacks)

    def connect(self):
        """
        Connect to the server.
        """
        self.invoke_log('on_invoke_trading_connect')
        user_login_field = RspUserLoginField()
        user_login_field.TradingDay = '20150120'
        user_login_field.SessionID = '565656565'
        user_login_field.LoginTime = '07:22:07'
        self.make_response(OnConnectionStatus, p_api2=self, double1=ord(Logined.value), ptr1=user_login_field)
        self.make_response(OnConnectionStatus, p_api2=self, double1=ord(Done.value))

    def disconnect(self):
        self.invoke_log('on_invoke_trading_disconnect')

    def get_api_type(self):
        self.invoke_log('on_invoke_trading_get_api_type')
        return Trade

    def get_api_version(self):
        self.invoke_log('on_invoke_trading_get_api_version')
        return '0.1'

    def get_api_name(self):
        self.invoke_log('on_invoke_trading_get_api_name')
        return 'LtsTrader Backtesting'

    def send_order(self, order_ref, future_id, exchange_id, order_type, time_in_force, side, open_close,
                   hedge_flag, qty, price, stop_price):
        """
        create a new order
        :param order_ref: int, if order ref less than 0, it will auto generate an order ref
        :param future_id: string, future id
        :param exchange_id: string, exchange id
        :param order_type: OrderType
        :param time_in_force: TimeInForce
        :param side: OrderSide, Buy or Sell
        :param open_close: OpenCloseType
        :param hedge_flag: HedgeFlagType
        :param qty: double
        :param price: double, Limit price
        :param stop_price: double
        :return: string, order id
        """
        self.invoke_log('on_invoke_trading_send_order', order_ref=order_ref, future_id=future_id,
                        exchange_id=exchange_id, order_type=order_type, time_in_force=time_in_force,
                        side=side, open_close=open_close, hedge_flag=hedge_flag, qty=qty,
                        price=price, stop_price=stop_price)

    def cancel_order(self, order):
        """
        cancel an order
        :param order: OrderField
        :return:
        """
        self.invoke_log('on_invoke_trading_cancel_order', order=order)

    def get_investor_position(self, future_id, exchange_id):
        """
        get investor position
        :param future_id: string
        :param exchange_id: string
        :return:
        """
        self.invoke_log('on_invoke_trading_get_investor_position', future_id=future_id, exchange_id=exchange_id)

    def get_trading_account(self):
        """
        get trading account info
        :return:
        """
        self.invoke_log('on_invoke_trading_get_trading_account')

    def get_instrument(self, future_id, exchange_id):
        """
        get future info
        :param future_id: string
        :param exchange_id: string, exchange id
        :return:
        """
        self.invoke_log('on_invoke_trading_get_instrument')

    def release(self):
        self.disconnect()
        self.queue.disconnect()
