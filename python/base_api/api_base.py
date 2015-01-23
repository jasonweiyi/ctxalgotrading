# -*- coding: utf-8 -*-
__author__ = 'Chunyou'

from comm import *


class AbstractApi(object):
    is_connected = False
    _queue = None
    _is_market = True
    _callbacks = None
    user_login_field = None

    def __init__(self, path, queue, local_path, server_address, broker_id, investor_id, password,
                 user_product_info, auth_code, is_market):
        self._queue = queue
        self._is_market = is_market

    def set_callbacks(self, callbacks):
        if not isinstance(callbacks, list):
            callbacks = [callbacks]
        self._callbacks = callbacks

    def connect(self):
        """
        Connect to the server.
        """
        pass

    def disconnect(self):
        pass

    def get_api_type(self):
        return Nono

    def get_api_version(self):
        return '0.1'

    def get_api_name(self):
        return 'AbstractApi'

    def subscribe(self, instrument_ids, exchange_id=b''):
        """
        Subscribe market data for the given instruments.
        :param instrument_ids: [string], list of instrument ids.
        :param exchange_id: string, exchange id
        """
        pass

    def unsubscribe(self, instrument_ids, exchange_id=b''):
        """
         Un-subscribe market data for the given instruments.
        :param instrument_ids: [string], list of instrument ids.
        :param exchange_id: string, exchange id
        """
        pass

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
        pass

    def cancel_order(self, order):
        """
        cancel an order
        :param order: OrderField
        :return:
        """
        pass

    def get_investor_position(self, future_id, exchange_id):
        """
        get investor position
        :param future_id: string
        :param exchange_id: string
        :return:
        """
        pass

    def get_trading_account(self):
        """
        get trading account info
        :return:
        """
        pass

    def get_instrument(self, future_id, exchange_id):
        """
        get future info
        :param future_id: string
        :param exchange_id: string, exchange id
        :return:
        """
        pass

    def _on_response(self, response_type, p_api1, p_api2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3):
        """
        callback from the queue
        :param response_type:
        :param p_api1:
        :param p_api2:
        :param double1:
        :param double2:
        :param ptr1:
        :param size1:
        :param ptr2:
        :param size2:
        :param ptr3:
        :param size3:
        :return:
        """
        print "Response: ", ord(response_type)
        if response_type == OnConnectionStatus.value:
            self._on_connect_status(p_api2, chr(int(double1)), ptr1, size1)
        elif self._callbacks:
            for callback in self._callbacks:
                if response_type == OnRtnDepthMarketData.value:
                    obj = cast(ptr1, POINTER(DepthMarketDataField)).contents
                    if self._is_market:
                        callback.on_market_rtn_depth_market_data(p_api2, obj)
                elif response_type == OnRspQryInstrument.value:
                    obj = cast(ptr1, POINTER(InstrumentField)).contents
                    callback.on_trading_rsp_qry_instrument(p_api2, obj, bool(double1))
                elif response_type == OnRspQryTradingAccount.value:
                    obj = cast(ptr1, POINTER(AccountField)).contents
                    callback.on_trading_rsp_qry_trading_account(p_api2, obj, bool(double1))
                elif response_type == OnRspQryInvestorPosition.value:
                    obj = cast(ptr1, POINTER(PositionField)).contents
                    callback.on_trading_rsp_qry_investor_position(p_api2, obj, bool(double1))
                elif response_type == OnRtnOrder.value:
                    obj = cast(ptr1, POINTER(OrderField)).contents
                    callback.on_trading_rtn_order(p_api2, obj)
                elif response_type == OnRtnTrade.value:
                    obj = cast(ptr1, POINTER(TradeField)).contents
                    callback.on_trading_rtn_trade(p_api2, obj)
                elif response_type == OnRtnError.value:
                    obj = cast(ptr1, POINTER(ErrorField)).contents
                    if self._is_market:
                        callback.on_market_rsp_error(p_api2, obj, bool(double1))
                    else:
                        callback.on_trading_rsp_error(p_api2, obj, bool(double1))

    def _on_connect_status(self, p_api, status, p_user_login_field, size):
        """
        callback on connect status
        :param p_api: int
        :param status:
        :param p_user_login_field: POINT of RspUserLoginField
        :return:
        """
        # 连接状态更新
        self.is_connected = Done.value == status

        obj = RspUserLoginField()

        if status in [Logined.value, Disconnected.value, Doing.value] and size > 0:
            obj = cast(p_user_login_field, POINTER(RspUserLoginField)).contents
            self.user_login_field = obj

        if self._callbacks:
            for callback in self._callbacks:
                if self._is_market:
                    callback.on_market_connected(p_api, obj, status)
                else:
                    callback.on_trading_connected(p_api, obj, status)

    def invoke_log(self, func_name, **kwargs):
        for callback in self._callbacks:
            if getattr(callback, func_name, None):
                getattr(callback, func_name)(kwargs)

    def make_response(self, response_type, p_api1=None, p_api2=None, double1=0, double2=0, ptr1=None, size1=0, ptr2=None, size2=0, ptr3=None, size3=0):
        """
        Make a response manually, useful in backtesting class
        :param response_type:
        :param p_api1:
        :param p_api2:
        :param double1:
        :param double2:
        :param ptr1:
        :param size1:
        :param ptr2:
        :param size2:
        :param ptr3:
        :param size3:
        :return:
        """
        if p_api1:
            p_api1 = id(p_api1)
        if p_api2:
            p_api2 = id(p_api2)
        if ptr1:
            ptr1 = byref(ptr1)
            if size1 == 0:
                size1 = 1
        if ptr2:
            ptr2 = byref(ptr2)
            if size2 == 0:
                size2 = 1
        if ptr3:
            ptr3 = byref(ptr3)
            if size3 == 0:
                size3 = 1
        return self._on_response(response_type.value, p_api1, p_api2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3)