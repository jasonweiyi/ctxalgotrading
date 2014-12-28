# -*- coding: utf-8 -*-
import os

from comm import *
from proxy import Proxy
from callbacks import MarketCallbacks, TraderCallbacks

__author__ = 'Chunyou'


class Queue(object):
    """
    queue service, used by api
    """
    proxy = None
    handle = None

    def __init__(self, path):
        self.proxy = Proxy(path)
        self.handle = self.proxy.x_request(Create)

    def connect(self):
        if self.proxy is not None:
            # 启动消息队列循环
            self.proxy.x_request(Connect, self.handle)

    def disconnect(self):
        if self.proxy is not None:
            self.register(None)
            # 停止底层线程
            self.proxy.x_request(Disconnect, self.handle)
            self.proxy.x_request(Release, self.handle)
        self.proxy = None
        self.handle = None

    def register(self, ptr1):
        if self.proxy is not None:
            self.proxy.x_request(Register, self.handle, ptr1=ptr1)


class ApiManager(object):
    """
    Api manager used to use many api easily
    """
    queue_path = os.path.dirname(__file__) + "/../include/QuantBox_Queue.dll"
    dict = {}

    @staticmethod
    def create(path, local_path, server_address, broker_id, investor_id, password,
               user_product_info=b"", auth_code=b""):
        """
        create an api handle base on the path
        :param path: path to the dll
        :param local_path: string, the local folder to store temp files.
        :param server_address: string, the server address to connect to.
        :param broker_id: string, Broker id.
        :param investor_id: string, investor id.
        :param password: string, investor password.
        :param user_product_info: the user's product info
        :param auth_code: auth code
        :return:
        """
        queue = Queue(ApiManager.queue_path)
        api = TraderApi(path, queue, local_path, server_address, broker_id, investor_id, password,
                        user_product_info, auth_code)
        ApiManager.dict[api] = queue
        return api

    @staticmethod
    def release(api):
        """
        release the api
        :param api:
        :return:
        """
        if api in ApiManager.dict:
            queue = ApiManager.dict[api]
            api.disconnect()
            queue.disconnect()
            del ApiManager.dict[api]


class BaseApi(object):
    """
    Api 基类
    """
    proxy = None  # Proxy
    handle = None
    _queue = None  # Queue
    _path1 = None
    _local_path = None

    is_connected = False
    _reconnectInterval = 0
    server = None  # ServerInfoField
    user = None  # UserInfoField
    user_login_field = None  # RspUserLoginField

    _x_response = None
    _callbacks = None

    def __init__(self, path, queue, local_path, server_address, broker_id, investor_id, password,
                 user_product_info, auth_code):
        self._path1 = path
        self._queue = queue
        self._local_path = local_path
        self._x_response = fnOnRespone(self._on_response)
        self._reconnectInterval = 0

        self.server = ServerInfoField()
        self.server.BrokerID = broker_id
        self.server.Address = server_address
        self.server.UserProductInfo = user_product_info
        self.server.AuthCode = auth_code

        self.user = UserInfoField()
        self.user.UserID = investor_id
        self.user.Password = password

    def set_callbacks(self, callbacks):
        self._callbacks = callbacks

    def connect(self):
        """
        Connect to the server.
        """
        if self.proxy is None:
            self.proxy = Proxy(self._path1)

        self.handle = self.proxy.x_request(Create)
        # 将API与队列进行绑定
        self.proxy.x_request(Register, self.handle, ptr1=self._queue.handle)
        self._queue.register(self._x_response)
        # 启动队列循环
        self._queue.connect()

        self.proxy.x_request(Connect, self.handle, ptr1=byref(self.server), ptr2=byref(self.user),
                             ptr3=self._local_path)

    def disconnect(self):
        self.is_connected = False
        if self.proxy is not None:
            self.proxy.x_request(Release, self.handle)
        self.proxy = None
        self.handle = None

    def get_api_type(self):
        if self.proxy is None:
            self.proxy = Proxy(self._path1)
        return self.proxy.x_request(GetApiType)

    def get_api_version(self):
        if self.proxy is None:
            self.proxy = Proxy(self._path1)
        ptr = self.proxy.x_request(GetApiVersion)
        return c_char_p(ptr).value

    def get_api_name(self):
        if self.proxy is None:
            self.proxy = Proxy(self._path1)
        ptr = self.proxy.x_request(GetApiName)
        return c_char_p(ptr).value

    def _on_response(self, request_type, p_api1, p_api2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3):
        """
        callback from the queue
        :param request_type:
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
        if request_type == OnConnectionStatus.value:
            self._on_connect_status(chr(int(double1)), ptr1)
        elif request_type == OnRtnError.value:
            if self._callbacks:
                obj = cast(ptr1, POINTER(ErrorField)).contents
                self._callbacks.on_rtn_error(obj, bool(double1))

    def _on_connect_status(self, status, p_user_login_field):
        """
        callback on connect status
        :param status:
        :param p_user_login_field: POINT of RspUserLoginField
        :return:
        """
        # 连接状态更新
        self.is_connected = Done.value == status

        obj = RspUserLoginField()

        if status in [Logined.value, Disconnected.value, Doing.value]:
            obj = cast(p_user_login_field, POINTER(RspUserLoginField)).contents
            self.user_login_field = obj

        if self._callbacks:
            self._callbacks.on_connect_status(status, obj)


class MarketApi(BaseApi):
    """
    Market api，handle market functions
    """
    def subscribe(self, instrument_ids, exchange_id=b''):
        """
        Subscribe market data for the given instruments.
        :param instrument_ids: [string], list of instrument ids.
        :param exchange_id: string, exchange id
        """
        self.proxy.x_request(Subscribe, self.handle,
                             ptr1=c_char_p(b','.join(instrument_ids) if isinstance(instrument_ids, list) else instrument_ids),
                             ptr2=c_char_p(exchange_id))

    def unsubscribe(self, instrument_ids, exchange_id=b''):
        """
         Un-subscribe market data for the given instruments.
        :param instrument_ids: [string], list of instrument ids.
        :param exchange_id: string, exchange id
        """
        self.proxy.x_request(Subscribe, self.handle,
                             ptr1=b','.join(instrument_ids) if isinstance(instrument_ids, list) else instrument_ids,
                             ptr2=exchange_id)

    def _on_response(self, request_type, p_api1, p_api2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3):
        """
        Handle the response
        :param request_type:
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
        if request_type == OnRtnDepthMarketData.value:
            if self._callbacks and isinstance(self._callbacks, MarketCallbacks):
                obj = cast(ptr1, POINTER(DepthMarketDataField)).contents
                self._callbacks.on_rtn_depth_market_data(obj)
        else:
            # handle by the super function
            super(MarketApi, self)._on_response(request_type, p_api1, p_api2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3)


class TraderApi(MarketApi):
    """
    Trader Api, handle trader functions
    """
    def req_qry_instrument(self, instrument_ids, exchange_id=b''):
        """
        get instrument info.
        :param instrument_ids: [string], list of instrument ids.
        :param exchange_id: string, exchange id
        """
        self.proxy.x_request(ReqQryInstrument, self.handle,
                             ptr1=b','.join(instrument_ids) if isinstance(instrument_ids, list) else instrument_ids,
                             ptr2=exchange_id)

    def req_qry_trading_account(self):
        """
        get trading account info
        :return:
        """
        self.proxy.x_request(ReqQryTradingAccount)

    def req_qry_settlement_info(self, trading_day):
        """
        get settlement info
        :param trading_day
        :return:
        """
        self.proxy.x_request(ReqQrySettlementInfo, ptr1=trading_day)

    def _on_response(self, request_type, p_api1, p_api2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3):
        """
        Handle the response
        :param request_type:
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
        if request_type == OnRspQryInstrument.value:
            if self._callbacks and isinstance(self._callbacks, TraderCallbacks):
                obj = cast(ptr1, POINTER(InstrumentField)).contents
                self._callbacks.on_rsp_qry_instrument(obj, bool(double1))
        elif request_type == OnRspQryTradingAccount.value:
            if self._callbacks and isinstance(self._callbacks, TraderCallbacks):
                obj = cast(ptr1, POINTER(AccountField)).contents
                self._callbacks.on_rsp_qry_trading_account(obj, bool(double1))
        elif request_type == OnRspQryInvestorPosition.value:
            if self._callbacks and isinstance(self._callbacks, TraderCallbacks):
                obj = cast(ptr1, POINTER(PositionField)).contents
                self._callbacks.on_rsp_qry_investor_position(obj, bool(double1))
        elif request_type == OnRspQrySettlementInfo.value:
            if self._callbacks and isinstance(self._callbacks, TraderCallbacks):
                obj = cast(ptr1, POINTER(SettlementInfoField)).contents
                self._callbacks.on_rsp_qry_settlement_info(obj, bool(double1))
        elif request_type == OnRtnOrder.value:
            if self._callbacks and isinstance(self._callbacks, TraderCallbacks):
                obj = cast(ptr1, POINTER(OrderField)).contents
                self._callbacks.on_rtn_order(obj)
        elif request_type == OnRtnTrade.value:
            if self._callbacks and isinstance(self._callbacks, TraderCallbacks):
                obj = cast(ptr1, POINTER(TradeField)).contents
                self._callbacks.on_rtn_trade(obj)
        elif request_type == OnRtnQuoteRequest.value:
            if self._callbacks and isinstance(self._callbacks, TraderCallbacks):
                obj = cast(ptr1, POINTER(QuoteRequestField)).contents
                self._callbacks.on_rtn_quote_request(obj)
        else:
            # handle by the super function
            super(TraderApi, self)._on_response(request_type, p_api1, p_api2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3)