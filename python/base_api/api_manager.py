# -*- coding: utf-8 -*-
import os

from comm import *
from proxy import Proxy

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

    def start_pulling(self):
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

    def process_first_message(self):
        """
        Process the first message in the queue (if any).
        Invoke corresponding callback function for that message.
        """
        if self.proxy is not None:
            return self.proxy.x_request(Process, self.handle)
        return False

    def register(self, ptr1):
        if self.proxy is not None:
            self.proxy.x_request(Register, self.handle, ptr1=ptr1)

    def clear(self):
        """
        Clear all messages in current message queue.
        """
        if self.proxy is not None:
            self.proxy.x_request(Clear, self.handle)


class ApiManager(object):
    """
    Api manager used to use many api easily
    """
    queue_path = os.path.dirname(__file__) + "/../include/QuantBox_Queue_x86.dll"
    dict = {}

    @staticmethod
    def create(path, local_path, server_address, broker_id, investor_id, password,
               user_product_info=b"", auth_code=b"", is_market=True):
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
        :param is_market: bool, is market api, default is True
        :return: Api, Queue
        """
        queue = Queue(ApiManager.queue_path)
        api = XApi(path, queue, local_path, server_address, broker_id, investor_id, password,
                   user_product_info, auth_code, is_market)
        ApiManager.dict[api] = queue
        return api, queue

    @staticmethod
    def release(api):
        """
        release the api
        :param api:
        :return:
        """
        if api in ApiManager.dict:
            queue = ApiManager.dict[api]
            print "disconect api"
            api.disconnect()
            print "disconect queue"
            queue.disconnect()
            del ApiManager.dict[api]


class XApi(object):
    """
    Api 基类
    """
    proxy = None  # Proxy
    handle = None
    _queue = None  # Queue
    _path1 = None
    _local_path = None
    _is_market = True

    is_connected = False
    _reconnectInterval = 0
    server = None  # ServerInfoField
    user = None  # UserInfoField
    user_login_field = None  # RspUserLoginField

    _x_response = None
    _callbacks = None

    def __init__(self, path, queue, local_path, server_address, broker_id, investor_id, password,
                 user_product_info, auth_code, is_market):
        self._path1 = path
        self._queue = queue
        self._is_market = is_market
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
        if not isinstance(callbacks, list):
            callbacks = [callbacks]
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
        # self._queue.start_pulling()

        self.proxy.x_request(Connect, self.handle, ptr1=byref(self.server), ptr2=byref(self.user),
                             ptr3=self._local_path)

    def disconnect(self):
        self.is_connected = False
        if self.proxy is not None:
            self.proxy.x_request(Disconnect, self.handle)
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

    def subscribe(self, instrument_ids, exchange_id=b''):
        """
        Subscribe market data for the given instruments.
        :param instrument_ids: [string], list of instrument ids.
        :param exchange_id: string, exchange id
        """
        self.proxy.x_request(Subscribe, self.handle,
                             ptr1=c_char_p(
                                 b','.join(instrument_ids) if isinstance(instrument_ids, list) else instrument_ids),
                             ptr2=c_char_p(exchange_id))

    def unsubscribe(self, instrument_ids, exchange_id=b''):
        """
         Un-subscribe market data for the given instruments.
        :param instrument_ids: [string], list of instrument ids.
        :param exchange_id: string, exchange id
        """
        self.proxy.x_request(Unsubscribe, self.handle,
                             ptr1=b','.join(instrument_ids) if isinstance(instrument_ids, list) else instrument_ids,
                             ptr2=exchange_id)

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
        order = OrderField()
        order.InstrumentID = future_id
        order.ExchangeID = exchange_id
        order.Type = order_type
        order.Side = side
        order.OpenClose = open_close
        order.HedgeFlag = hedge_flag
        order.Qty = qty
        order.Price = price
        order.StopPx = stop_price
        order.TimeInForce = time_in_force
        order_id_ref = self.proxy.x_request(ReqOrderInsert, self.handle, double1=order_ref, ptr1=byref(order))
        return cast(order_id_ref, POINTER(OrderIDType)).contents.value

    def cancel_order(self, order):
        """
        cancel an order
        :param order: OrderField
        :return:
        """
        self.proxy.x_request(ReqOrderAction, self.handle, ptr1=order.ID)

    def get_investor_position(self, future_id, exchange_id):
        """
        get investor position
        :param future_id: string
        :param exchange_id: string
        :return:
        """
        self.proxy.x_request(ReqQryInvestorPosition, self.handle, ptr1=future_id, ptr2=exchange_id)

    def get_trading_account(self):
        """
        get trading account info
        :return:
        """
        self.proxy.x_request(ReqQryTradingAccount, self.handle)

    def get_instrument(self, future_id, exchange_id):
        """
        get future info
        :param future_id: string
        :param exchange_id: string, exchange id
        :return:
        """
        self.proxy.x_request(ReqQryInstrument, self.handle,
                             ptr1=b','.join(future_id) if isinstance(future_id, list) else future_id,
                             ptr2=exchange_id)

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