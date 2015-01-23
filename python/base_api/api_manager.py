# -*- coding: utf-8 -*-
import os

from comm import *
from proxy import Proxy
from api_base import AbstractApi

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
    def create_queue():
        return Queue(ApiManager.queue_path)

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


class XApi(AbstractApi):
    """
    Api 基类
    """
    proxy = None  # Proxy
    handle = None
    _path1 = None
    _local_path = None

    _reconnectInterval = 0
    server = None  # ServerInfoField
    user = None  # UserInfoField

    _x_response = None

    def __init__(self, path, queue, local_path, server_address, broker_id, investor_id, password,
                 user_product_info, auth_code, is_market):
        super(XApi, self).__init__(path, queue, local_path, server_address, broker_id, investor_id, password,
                             user_product_info, auth_code, is_market)
        self._path1 = path
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
