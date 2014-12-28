# -*- coding: utf-8 -*-
__author__ = 'Chunyou'

from ctypes import *

"""
ErrorIDType是一个错误代码类型
"""
ErrorIDType = c_int

"""
ErrorMsgType是一个错误信息类型
"""
ErrorMsgType = c_char * 256

"""
PriceType是一个价格类型
"""
PriceType = c_double

"""
VolumeType是一个交易量类型
"""
VolumeType = c_int

"""
QtyType是一个交易量类型
"""
QtyType = c_double

"""
LargeVolumeType是一个大额数量类型
"""
LargeVolumeType = c_double

"""
InstrumentIDType是一个合约代码类型
"""
InstrumentIDType = c_char * 31

"""
SymbolType是一个合约唯一符号类型
"""
SymbolType = c_char * 64

"""
DateTimeType是一个日期时间类型
"""
DateTimeType = c_char * 32

"""
ExchangeIDType是一个交易所代码类型
"""
ExchangeIDType = c_char * 9

"""
MoneyType是一个资金类型
"""
MoneyType = c_double

"""
DateType是一个日期类型
"""
DateType = c_char * 9
DateIntType = c_int

"""
TimeType是一个时间类型
"""
TimeType = c_char * 9
TimeIntType = c_int

"""
UserIDType是一个用户代码类型
"""
UserIDType = c_char * 32

"""
TFtdcPasswordType是一个密码类型
"""
PasswordType = c_char * 41

"""
ProductInfoType是一个产品信息类型
"""
ProductInfoType = c_char * 11

"""
BrokerIDType是一个经纪公司代码类型
"""
BrokerIDType = c_char * 11

"""
AuthCodeType是一个客户端认证码类型
"""
AuthCodeType = c_char * 17

"""
AddressType是一个地址类型
"""
AddressType = c_char * 512

"""
VolumeMultipleType是一个合约数量乘数类型
"""
VolumeMultipleType = c_int

"""
InstrumentNameType是一个合约名称类型
"""
InstrumentNameType = c_char * 21

"""
ContentType是一个消息正文类型
"""
ContentType = c_char * 501

"""
SessionIDType是一个会话编号类型
"""
SessionIDType = c_char * 32

"""
OrderIDType是一个订单唯一编号类型
"""
OrderIDType = c_char * 64

"""
TradeIDType是一个成交编号类型
"""
TradeIDType = c_char * 64

"""
连接状态枚举
"""
ConnectionStatus = c_char
# 未初始化
Uninitialized = ConnectionStatus(b'\x00')
# 已经初始化
Initialized = ConnectionStatus(b'\x01')
# 连接已经断开
Disconnected = ConnectionStatus(b'\x02')
# 连接中
Connecting = ConnectionStatus(b'\x03')
# 连接成功
Connected = ConnectionStatus(b'\x04')
# 授权中
Authorizing = ConnectionStatus(b'\x05')
# 授权成功
Authorized = ConnectionStatus(b'\x06')
# 登录中
Logining = ConnectionStatus(b'\x07')
# 登录成功
Logined = ConnectionStatus(b'\x08')
# 结算单确认中
Confirming = ConnectionStatus(b'\t')
# 已经确认
Confirmed = ConnectionStatus(b'\n')
# Doing
Doing = ConnectionStatus(b'\x0b')
# 完成
Done = ConnectionStatus(b'\x0c')
# 未知
Unknown = ConnectionStatus(b'\r')

ApiType = c_char

Nono = ApiType(b'\x00')

Trade = ApiType(b'\x01')

MarketData = ApiType(b'\x02')

Level2 = ApiType(b'\x04')

QuoteRequest = ApiType(b'\x08')

DataLevelType = c_char

L0 = DataLevelType(b'\x00')

L1 = DataLevelType(b'\x01')

L5 = DataLevelType(b'\x02')

L10 = DataLevelType(b'\x03')

FULL = DataLevelType(b'\x04')

ResumeType = c_char

Restart = ResumeType(b'\x00')

Resume = ResumeType(b'\x01')

Quick = ResumeType(b'\x02')

PutCall = c_char

Put = PutCall(b'\x00')

Call = PutCall(b'\x01')

OrderStatus = c_char

NotSent = OrderStatus(b'\x00')

PendingNew = OrderStatus(b'\x01')

New = OrderStatus(b'\x02')

Rejected = OrderStatus(b'\x03')

PartiallyFilled = OrderStatus(b'\x04')

Filled = OrderStatus(b'\x05')

PendingCancel = OrderStatus(b'\x06')

Cancelled = OrderStatus(b'\x07')

Expired = OrderStatus(b'\x08')

PendingReplace = OrderStatus(b'\t')

Replaced = OrderStatus(b'\n')

OrderSide = c_char

Buy = OrderSide(b'\x00')

Sell = OrderSide(b'\x01')

OrderType = c_char

Market = OrderType(b'\x00')

Stop = OrderType(b'\x01')

Limit = OrderType(b'\x02')

StopLimit = OrderType(b'\x03')

MarketOnClose = OrderType(b'\x04')

TrailingStop = OrderType(b'\x05')

TrailingStopLimit = OrderType(b'\x06')

TimeInForce = c_char

ATC = TimeInForce(b'\x00')

Day = TimeInForce(b'\x01')

GTC = TimeInForce(b'\x02')

IOC = TimeInForce(b'\x03')

OPG = TimeInForce(b'\x04')

OC = TimeInForce(b'\x05')

FOK = TimeInForce(b'\x06')

GTX = TimeInForce(b'\x07')

GTD = TimeInForce(b'\x08')

GFS = TimeInForce(b'\t')

PositionSide = c_char

Long = PositionSide(b'\x00')

Short = PositionSide(b'\x01')

ExecType = c_char

ExecNew = ExecType(b'\x00')

ExecRejected = ExecType(b'\x01')

ExecTrade = ExecType(b'\x02')

ExecPendingCancel = ExecType(b'\x03')

ExecCancelled = ExecType(b'\x04')

ExecCancelReject = ExecType(b'\x05')

ExecPendingReplace = ExecType(b'\x06')

ExecReplace = ExecType(b'\x07')

ExecReplaceReject = ExecType(b'\x08')

OpenCloseType = c_char

Open = OpenCloseType(b'\x00')

Close = OpenCloseType(b'\x01')

CloseToday = OpenCloseType(b'\x02')

HedgeFlagType = c_char

Speculation = HedgeFlagType(b'\x00')

Arbitrage = HedgeFlagType(b'\x01')

Hedge = HedgeFlagType(b'\x02')

MarketMaker = HedgeFlagType(b'\x03')

InstrumentType = c_char

Stock = InstrumentType(b'\x00')

Future = InstrumentType(b'\x01')

Option = InstrumentType(b'\x02')

FutureOption = InstrumentType(b'\x03')

Bond = InstrumentType(b'\x04')

FX = InstrumentType(b'\x05')

Index = InstrumentType(b'\x06')

ETF = InstrumentType(b'\x07')

MultiLeg = InstrumentType(b'\x08')

Synthetic = InstrumentType(b'\t')


class PositionField(Structure):
    _fields_ = [
        ("InstrumentID", InstrumentIDType),
        ("ExchangeID", ExchangeIDType),
        ("Side", PositionSide),
        ("Position", QtyType),
        ("TdPosition", QtyType),
        ("YdPosition", QtyType),
        ("HedgeFlag", HedgeFlagType)]


class OrderField(Structure):
    _fields_ = [
        ("InstrumentID", InstrumentIDType),
        ("ExchangeID", ExchangeIDType),
        ("Type", OrderType),
        ("Side", OrderSide),
        ("Qty", QtyType),
        ("Price", PriceType),
        ("OpenClose", OpenCloseType),
        ("HedgeFlag", HedgeFlagType),

        ("StopPx", PriceType),
        ("TimeInForce", TimeInForce),

        ("Status", OrderStatus),
        ("ExecType", ExecType),
        ("LeavesQty", QtyType),
        ("CumQty", QtyType),
        ("AvgPx", PriceType),
        ("ErrorID", ErrorIDType),
        ("Text", ErrorMsgType),

        ("ID", OrderIDType),
        ("OrderID", OrderIDType),
        ("DateTime", c_long)
        # # 预留字段，支持bool,int,long,double
        # ("double1", double),
        # ("double2", double),
        # # 是否要进行扩展属性的支持？
        # void* ptr1;
        ## 是否需要从底层传入更多信息，比如说OrderSysID
    ]


class TradeField(Structure):
    _fields_ = [
        ("InstrumentID", InstrumentIDType),
        ("ExchangeID", ExchangeIDType),

        ("Side", OrderSide),
        ("Qty", QtyType),
        ("Price", PriceType),
        ("OpenClose", OpenCloseType),
        ("HedgeFlag", HedgeFlagType),
        ("Commission", MoneyType),
        ("Time", TimeType),
        ("ID", OrderIDType),
        ("TradeID", TradeIDType)]


class ServerInfoField(Structure):
    _fields_ = [
        ("IsUsingUdp", c_bool),
        ("IsMulticast", c_bool),
        ("TopicId", c_int),
        ("Resume", ResumeType),
        ("BrokerID", BrokerIDType),
        ("UserProductInfo", ProductInfoType),
        ("AuthCode", AuthCodeType),
        ("Address", AddressType)]


class ConfigInfoField(Structure):
    _fields_ = []


# 用户信息
class UserInfoField(Structure):
    _fields_ = [
        # /用户代码
        ("UserID", UserIDType),
        # /密码
        ("Password", PasswordType)]


# 错误信息
class ErrorField(Structure):
    _fields_ = [
        # 错误代码
        ("ErrorID", ErrorIDType),
        # 错误信息
        ("ErrorMsg", ErrorMsgType)]


# 登录回报
class RspUserLoginField(Structure):
    _fields_ = [
        # /交易日
        ("TradingDay", DateType),
        # 时间
        ("LoginTime", TimeType),
        # 会话ID
        ("SessionID", SessionIDType),
        # 错误代码
        ("ErrorID", ErrorIDType),
        # 错误信息
        ("ErrorMsg", ErrorMsgType)]


# /深度行情
class DepthMarketDataField(Structure):
    _fields_ = [
        # /交易所时间
        ("TradingDay", DateIntType),  # 交易日，用于给数据接收器划分到同一文件使用，基本没啥别的用处
        ("ActionDay", DateIntType),
        ("UpdateTime", TimeIntType),
        ("UpdateMillisec", TimeIntType),

        # /唯一符号
        ("Symbol", SymbolType),
        # /合约代码
        ("InstrumentID", InstrumentIDType),
        # /交易所代码
        ("ExchangeID", ExchangeIDType),

        #/最新价
        ("LastPrice", PriceType),
        #/数量
        ("Volume", LargeVolumeType),
        #/成交金额
        ("Turnover", MoneyType),
        #/持仓量
        ("OpenInterest", LargeVolumeType),
        #/当日均价
        ("AveragePrice", PriceType),


        #/今开盘
        ("OpenPrice", PriceType),
        #/最高价
        ("HighestPrice", PriceType),
        #/最低价
        ("LowestPrice", PriceType),
        #/今收盘
        ("ClosePrice", PriceType),
        #/本次结算价
        ("SettlementPrice", PriceType),

        #/涨停板价
        ("UpperLimitPrice", PriceType),
        #/跌停板价
        ("LowerLimitPrice", PriceType),
        #/昨收盘
        ("PreClosePrice", PriceType),
        #/上次结算价
        ("PreSettlementPrice", PriceType),
        #/昨持仓量
        ("PreOpenInterest", LargeVolumeType),


        #/申买价一
        ("BidPrice1", PriceType),
        #/申买量一
        ("BidVolume1", VolumeType),
        #/申卖价一
        ("AskPrice1", PriceType),
        #/申卖量一
        ("AskVolume1", VolumeType),
        #/申买价二
        ("BidPrice2", PriceType),
        #/申买量二
        ("BidVolume2", VolumeType),
        #/申卖价二
        ("AskPrice2", PriceType),
        #/申卖量二
        ("AskVolume2", VolumeType),
        #/申买价三
        ("BidPrice3", PriceType),
        #/申买量三
        ("BidVolume3", VolumeType),
        #/申卖价三
        ("AskPrice3", PriceType),
        #/申卖量三
        ("AskVolume3", VolumeType),
        #/申买价四
        ("BidPrice4", PriceType),
        #/申买量四
        ("BidVolume4", VolumeType),
        #/申卖价四
        ("AskPrice4", PriceType),
        #/申卖量四
        ("AskVolume4", VolumeType),
        #/申买价五
        ("BidPrice5", PriceType),
        #/申买量五
        ("BidVolume5", VolumeType),
        #/申卖价五
        ("AskPrice5", PriceType),
        #/申卖量五
        ("AskVolume5", VolumeType)]


# /发给做市商的询价请求
class QuoteRequestField(Structure):
    _fields_ = [
        # /唯一符号
        ("Symbol", SymbolType),
        # /合约代码
        ("InstrumentID", InstrumentIDType),
        # /交易所代码
        ("ExchangeID", ExchangeIDType),
        #/交易日
        ("TradingDay", DateType),
        #/询价编号
        ("QuoteID", OrderIDType),
        #/询价时间
        ("QuoteTime", TimeType)]


# /合约
class InstrumentField(Structure):
    _fields_ = [
        # /唯一符号
        ("Symbol", SymbolType),
        # /合约代码
        ("InstrumentID", InstrumentIDType),
        #/交易所代码
        ("ExchangeID", ExchangeIDType),
        #/合约名称
        ("InstrumentName", InstrumentNameType),

        #/合约名称
        ("Type", InstrumentType),
        #/合约数量乘数
        ("VolumeMultiple", VolumeMultipleType),
        #/最小变动价位
        ("PriceTick", PriceType),
        #/到期日
        ("ExpireDate", DateType),
        #/基础商品代码
        ("UnderlyingInstrID", InstrumentIDType),
        #/执行价
        ("StrikePrice", PriceType),
        #/期权类型
        ("OptionsType", PutCall)]


# /账号
class AccountField(Structure):
    _fields_ = [
        # /上次结算准备金
        ("PreBalance", MoneyType),
        #/当前保证金总额
        ("CurrMargin", MoneyType),
        #/手续费
        ("Commission", MoneyType),
        #/平仓盈亏
        ("CloseProfit", MoneyType),
        #/持仓盈亏
        ("PositionProfit", MoneyType),
        #/期货结算准备金
        ("Balance", MoneyType),
        #/可用资金
        ("Available", MoneyType)]


# /账号
class SettlementInfoField(Structure):
    _fields_ = [
        #/交易日
        ("TradingDay", DateType),
        #/消息正文
        ("Content", ContentType)]


"""
Queue
"""
RequestType = c_char

GetApiType = RequestType(chr(0))

GetApiVersion = RequestType(chr(1))

GetApiName = RequestType(chr(2))

Create = RequestType(chr(3))

Release = RequestType(chr(4))

Register = RequestType(chr(5))

Config = RequestType(chr(6))

Connect = RequestType(chr(7))

Disconnect = RequestType(chr(8))

Clear = RequestType(chr(9))

Process = RequestType(chr(10))

Subscribe = RequestType(chr(11))

Unsubscribe = RequestType(chr(12))

SubscribeQuote = RequestType(chr(13))

UnsubscribeQuote = RequestType(chr(14))

ReqOrderInsert = RequestType(chr(15))

ReqQuoteInsert = RequestType(chr(16))

ReqOrderAction = RequestType(chr(17))

ReqQuoteAction = RequestType(chr(18))

ReqQryOrder = RequestType(chr(19))

ReqQryTrade = RequestType(chr(20))

ReqQryInstrument = RequestType(chr(21))

ReqQryTradingAccount = RequestType(chr(22))

ReqQryInvestorPosition = RequestType(chr(23))

ReqQryInvestorPositionDetail = RequestType(chr(24))

ReqQryInstrumentCommissionRate = RequestType(chr(25))

ReqQryInstrumentMarginRate = RequestType(chr(26))

ReqQrySettlementInfo = RequestType(chr(27))

ResponeType = c_char

OnConnectionStatus = ResponeType(chr(64))

OnRtnDepthMarketData = ResponeType(chr(65))

OnRtnError = ResponeType(chr(66))

OnRspQryInstrument = ResponeType(chr(67))

OnRspQryTradingAccount = ResponeType(chr(68))

OnRspQryInvestorPosition = ResponeType(chr(69))

OnRspQrySettlementInfo = ResponeType(chr(70))

OnRtnOrder = ResponeType(chr(71))

OnRtnTrade = ResponeType(chr(72))

OnRtnQuoteRequest = ResponeType(chr(73))

# function
fnOnRespone = WINFUNCTYPE(None, c_char, c_void_p, c_void_p, c_double, c_double, c_void_p, c_int, c_void_p, c_int,
                          c_void_p, c_int)