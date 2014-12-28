# -*- coding: utf-8 -*-
__author__ = 'Chunyou'


class BaseCallBacks(object):
    """
    The base class for all callbacks for both market and trading APIs.
    Sub-class should redefine needed callbacks to implement customized behavior.Base
    """
    def __init__(self):
        pass

    def on_connect_status(self, status, rsp_user_login):
        """
        callback on market server connect status change
        :param status: ConnectionStatus int
        :param rsp_user_login: RspUserLoginField
        :return:
        """
        pass

    def on_rtn_error(self, rsp_info, b_is_last):
        """
        callback on market server when error occurred
        :param rsp_info: ErrorField
        :param b_is_last: boolean, True if it is the last response for this request
        :return:
        """
        pass


class MarketCallbacks(BaseCallBacks):
    def on_rtn_depth_market_data(self, depth_market_data):
        """
        callback on market server when receive market data
        :param depth_market_data: DepthMarketDataField
        :return:
        """
        pass


class TraderCallbacks(BaseCallBacks):
    def on_rsp_qry_instrument(self, instrument, b_is_last):
        """
        请求查询合约响应。当客户端发出请求查询合约指令后，交易托管系统返回响应时，该方法会被调用。
        :param instrument: InstrumentField
        :param b_is_last: boolean, True if it is the last response for this request
        :return:
        """
        pass

    def on_rsp_qry_trading_account(self, trading_account, b_is_last):
        """
        请求查询资金账户响应。当客户端发出请求查询资金账户指令后，交易托管系统返回响应时，该方法会被调用。
        :param trading_account: AccountField
        :param b_is_last: boolean, True if it is the last response for this request
        :return:
        """
        pass

    def on_rsp_qry_investor_position(self, investor_position, b_is_last):
        """
        投资者持仓查询应答。当客户端发出投资者持仓查询指令后，后交易托管系统返回响应时，该方法会被调用。
        :param investor_position: PositionField
        :param b_is_last: boolean, True if it is the last response for this request
        :return:
        """
        pass

    def on_rsp_qry_settlement_info(self, settlement_info, b_is_last):
        """
        ReqQrySettlementInfo 回调
        :param investor_position: SettlementInfoField
        :param b_is_last: boolean, True if it is the last response for this request
        :return:
        """
        pass

    def on_rtn_order(self, order):
        """
        报单回报。当客户端进行报单录入、报单操作及其它原因（如部分成交）导致报单状态发生变化时，交易托管系统会主动通知客户端，该方法会被调用。
        :param order: OrderField
        """
        pass

    def on_rtn_trade(self, trade):
        """
        成交回报。当发生成交时交易托管系统会通知客户端，该方法会被调用。
        :param trade:TradeField
        :return:
        """
        pass

    def on_rtn_quote_request(self, quote_request):
        """
        QuoteRequest
        :param quote_request: QuoteRequestField
        :return:
        """
        pass