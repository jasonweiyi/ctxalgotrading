# -*- coding: utf-8 -*-
from _ctypes import Structure
from datetime import datetime
from base_api.callbacks import CallBacks
import sys

__author__ = 'Chunyou'


class Logger(CallBacks):
    is_flushing = False
    market_data = []
    trading_data = []
    gbk_decode_key = ['Text', 'ErrorMsg', 'InstrumentName']
    ord_decode_key = ['Type', 'OptionsType', 'Side', 'OpenClose', 'HedgeFlag', 'TimeInForce', 'Status', 'ExecType']

    def __init__(self):
        """
        Initialize current callback.
        """
        CallBacks.__init__(self)
        # The kinds of items to be logged.
        self.data_kinds = [
            # The following are callbacks.
            "market_connected",
            "market_rsp_error",
            "market_rtn_depth_market_data",
            "trading_connected",
            "trading_rsp_error",
            "trading_rsp_qry_depth_market_data",
            "trading_rsp_qry_instrument",
            "trading_rsp_qry_investor_position",
            "trading_rsp_qry_investor_position_detail",
            "trading_rsp_qry_trading_account",
            "trading_rtn_order",
            "trading_rtn_trade",

            # The following are method invocations.
            "invoke_market_connect",
            "invoke_market_disconnect",
            "invoke_market_subscribe",
            "invoke_market_unsubscribe",
            "invoke_trading_connect",
            "invoke_trading_disconnect",
            "invoke_trading_send_order",
            "invoke_trading_cancel_order",
            "invoke_trading_get_investor_position",
            "invoke_trading_get_investor_position_details",
            "invoke_trading_get_order",
            "invoke_trading_get_trading_account",
            "invoke_trading_get_instrument",
            "invoke_trading_get_commission_rate",
            "invoke_trading_get_margin_rate",
            "invoke_trading_get_deep_market_data"
        ]

        # The maximum number of records to keep in memory before
        # writing them into permanent storage.
        self.in_memory_records = 100

        # The time formatter function pointer.
        # If not None, it should be a function which takes a datetime object as input and returns an adapted
        # datetime object. Mainly used for adapting time zone time information.
        self.datetime_adapter = None

    def _do_log(self, content, data_kind=None):
        data_kind = data_kind if data_kind else sys._getframe(1).f_code.co_name[3:]
        if data_kind in self.data_kinds:
            for k, v in content.iteritems():
                if isinstance(v, Structure):
                    content[k] = {}
                    for k1, _ in v._fields_:
                        content[k][k1] = getattr(v, k1).decode('gbk') if k1 in self.gbk_decode_key else (ord(getattr(v, k1)) if k1 in self.ord_decode_key else getattr(v, k1))
                elif 'ctypes' in str(type(v)) or 'c_char_Array_' in str(type(v)):
                    content[k] = v.value
                elif k == 'status':
                    content[k] = ord(v)

            now = datetime.now()
            if self.datetime_adapter is not None:
                now = self.datetime_adapter(now)
            if data_kind in ['market_rtn_depth_market_data']:
                self.market_data.append([now, data_kind, content])
            else:
                self.trading_data.append([now, data_kind, content])
            if len(self.market_data) + len(self.trading_data) >= self.in_memory_records:
                self.flush()

    #handel on_invoke_*
    def __getattr__(self, item):
        if item[:10] == 'on_invoke_':
            def on_invoke(kwargs):
                self._do_log(kwargs, data_kind=item[3:])
            return on_invoke

    def on_market_connected(self, p_api, rsp_user_login, status):
        """
        callback on market server connect status change
        :param p_api: c_void_p
        :param rsp_user_login: RspUserLoginField
        :param status: ConnectionStatus int
        :return:
        """
        self._do_log({
            "p_api": p_api,
            "rsp_user_login": rsp_user_login,
            "status": status
        })

    def on_market_rsp_error(self, p_api, rsp_info, b_is_last):
        """
        callback on market server when error occurred
        :param p_api: c_void_p
        :param rsp_info: ErrorField
        :param b_is_last: boolean, True if it is the last response for this request
        :return:
        """
        self._do_log({
            "p_api": p_api,
            "rsp_info": rsp_info,
            "b_is_last": b_is_last
        })

    def on_market_rtn_depth_market_data(self, p_api, depth_market_data):
        """
        callback on market server when receive market data
        :param p_api:  c_void_p
        :param depth_market_data: DepthMarketDataField
        :return:
        """
        self._do_log({
            "p_api": p_api,
            "depth_market_data": depth_market_data
        })

    def on_trading_connected(self, p_api, rsp_user_login, status):
        """
        callback on trading server connect status change
        :param p_api: c_void_p
        :param rsp_user_login: CThostFtdcRspUserLoginField
        :param status: ConnectionStatus int
        :return:
        """
        self._do_log({
            "p_api": p_api,
            "rsp_user_login": rsp_user_login,
            "status": status
        })

    def on_trading_rsp_error(self, p_api, rsp_info, b_is_last):
        """
        callback on trading server when error occurred
        :param p_api: c_void_p
        :param rsp_info: CThostFtdcRspInfoField
        :param b_is_last: boolean, True if it is the last response for this request
        :return:
        """
        self._do_log({
            "p_api": p_api,
            "rsp_info": rsp_info,
            "b_is_last": b_is_last
        })

    def on_trading_rsp_qry_depth_market_data(self, p_api, depth_market_data, rsp_info, b_is_last):
        """
        请求查询行情响应。当客户端发出请求查询行情指令后，交易托管系统返回响应时，该方法会被调用。
        :param p_api: c_void_p
        :param depth_market_data: CThostFtdcDepthMarketDataField
        :param rsp_info: CThostFtdcRspInfoField
        :param b_is_last: boolean, True if it is the last response for this request
        :return:
        """
        self._do_log({
            "p_api": p_api,
            "depth_market_data": depth_market_data,
            "rsp_info": rsp_info,
            "b_is_last": b_is_last
        })

    def on_trading_rsp_qry_instrument(self, p_api, instrument, b_is_last):
        """
        请求查询合约响应。当客户端发出请求查询合约指令后，交易托管系统返回响应时，该方法会被调用。
        :param p_api: c_void_p
        :param instrument: InstrumentField
        :param b_is_last: boolean, True if it is the last response for this request
        :return:
        """
        self._do_log({
            "p_api": p_api,
            "instrument": instrument,
            "b_is_last": b_is_last
        })

    def on_trading_rsp_qry_investor_position(self, p_api, investor_position, b_is_last):
        """
        投资者持仓查询应答。当客户端发出投资者持仓查询指令后，后交易托管系统返回响应时，该方法会被调用。
        :param p_api: c_void_p
        :param investor_position: PositionField
        :param b_is_last: boolean, True if it is the last response for this request
        :return:
        """
        self._do_log({
            "p_api": p_api,
            "investor_position": investor_position,
            "b_is_last": b_is_last
        })

    def on_trading_rsp_qry_trading_account(self, p_api, trading_account, b_is_last):
        """
        请求查询资金账户响应。当客户端发出请求查询资金账户指令后，交易托管系统返回响应时，该方法会被调用。
        :param p_api: c_void_p
        :param trading_account: AccountField
        :param b_is_last: boolean, True if it is the last response for this request
        :return:
        """
        self._do_log({
            "p_api": p_api,
            "trading_account": trading_account,
            "b_is_last": b_is_last
        })

    def on_trading_rtn_order(self, p_api, order):
        """
        报单回报。当客户端进行报单录入、报单操作及其它原因（如部分成交）导致报单状态发生变化时，交易托管系统会主动通知客户端，该方法会被调用。
        :param p_api: c_void_p
        :param order: OrderField
        """
        self._do_log({
            "p_api": p_api,
            "order": order
        })

    def on_trading_rtn_trade(self, p_api, trade):
        """
        成交回报。当发生成交时交易托管系统会通知客户端，该方法会被调用。
        :param p_api: c_void_p
        :param trade:TradeField
        :return:
        """
        self._do_log({
            "p_api": p_api,
            "trade": trade
        })

    def flush(self):
        """
        Flush unwritten data into permanent storage. Can be used when the trading program ends.
        """
        if not self.is_flushing:
            self.is_flushing = True
            self.flush_impl()
            self.is_flushing = False

    def flush_impl(self):
        """
        Sub-class should redefine this to implement permanent storage..
        :return:
        """
        pass