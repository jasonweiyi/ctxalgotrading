#ifndef _API_STRUCT_H_
#define _API_STRUCT_H_

#include "ApiDataType.h"
#include "ApiEnum.h"


struct PositionField
{
	///Ψһ����
	SymbolType			Symbol;
	///��Լ����
	InstrumentIDType	InstrumentID;
	///����������
	ExchangeIDType	ExchangeID;

	PositionSide	Side;
	QtyType Position;
	QtyType TdPosition;
	QtyType YdPosition;
	HedgeFlagType HedgeFlag;
	//TThostFtdcPositionDateType ��û����
};

struct QuoteField
{
	InstrumentIDType InstrumentID;
	ExchangeIDType	ExchangeID;

	QtyType AskQty;
	PriceType AskPrice;
	OpenCloseType AskOpenClose;
	HedgeFlagType AskHedgeFlag;

	QtyType BidQty;
	PriceType BidPrice;
	OpenCloseType BidOpenClose;
	HedgeFlagType BidHedgeFlag;

	OrderIDType ID;
	OrderIDType AskID;
	OrderIDType BidID;
	OrderIDType AskOrderID;
	OrderIDType BidOrderID;
	OrderStatus Status;
	ExecType ExecType;
	ErrorIDType ErrorID;
	ErrorMsgType Text;

	/*
	QtyType LeavesQty;
	QtyType CumQty;
	PriceType AvgPx;
	

	
	
	long DateTime;*/
};

struct OrderField
{
	InstrumentIDType InstrumentID;
	ExchangeIDType	ExchangeID;
	OrderType Type;
	OrderSide Side;
	QtyType Qty;
	PriceType Price;
	OpenCloseType OpenClose;
	HedgeFlagType HedgeFlag;

	PriceType StopPx;
	TimeInForce TimeInForce;

	OrderStatus Status;
	ExecType ExecType;
	QtyType LeavesQty;
	QtyType CumQty;
	PriceType AvgPx;
	ErrorIDType ErrorID;
	ErrorMsgType Text;

	OrderIDType ID;
	OrderIDType OrderID;
	long DateTime;
};

struct TradeField
{
	InstrumentIDType InstrumentID;
	ExchangeIDType	ExchangeID;

	OrderSide Side;
	QtyType Qty;
	PriceType Price;
	OpenCloseType OpenClose;
	HedgeFlagType HedgeFlag;
	MoneyType Commission;
	TimeType Time;
	OrderIDType ID;
	TradeIDType TradeID;
};

struct ServerInfoField
{
	bool IsUsingUdp;
	bool IsMulticast;
	int	TopicId;
	int Port;
	ResumeType	MarketDataTopicResumeType;
	ResumeType	PrivateTopicResumeType;
	ResumeType	PublicTopicResumeType;
	ResumeType	UserTopicResumeType;
	BrokerIDType	BrokerID;
	ProductInfoType	UserProductInfo;
	AuthCodeType	AuthCode;
	AddressType	Address;
};

struct ConfigInfoField
{

};

// �û���Ϣ
struct UserInfoField
{
	///�û�����
	UserIDType	UserID;
	///����
	PasswordType	Password;
};


// ������Ϣ
struct ErrorField
{
	// �������
	ErrorIDType	ErrorID;
	// ������Ϣ
	ErrorMsgType	ErrorMsg;
};


// ��¼�ر�
struct RspUserLoginField
{
	///������
	DateType	TradingDay;
	// ʱ��
	TimeType	LoginTime;
	// �ỰID
	SessionIDType	SessionID;
	// �������
	ErrorIDType	ErrorID;
	// ������Ϣ
	ErrorMsgType	ErrorMsg;
};



///�������
struct DepthMarketDataField
{
	///������ʱ��
	DateIntType			TradingDay; // �����գ����ڸ����ݽ��������ֵ�ͬһ�ļ�ʹ�ã�����ûɶ����ô�
	DateIntType			ActionDay;
	TimeIntType			UpdateTime;
	TimeIntType			UpdateMillisec;

	///Ψһ����
	SymbolType			Symbol;
	///��Լ����
	InstrumentIDType	InstrumentID;
	///����������
	ExchangeIDType	ExchangeID;

	///���¼�
	PriceType	LastPrice;
	///����
	LargeVolumeType	Volume;
	///�ɽ����
	MoneyType	Turnover;
	///�ֲ���
	LargeVolumeType	OpenInterest;
	///���վ���
	PriceType	AveragePrice;


	///����
	PriceType	OpenPrice;
	///��߼�
	PriceType	HighestPrice;
	///��ͼ�
	PriceType	LowestPrice;
	///������
	PriceType	ClosePrice;
	///���ν����
	PriceType	SettlementPrice;

	///��ͣ���
	PriceType	UpperLimitPrice;
	///��ͣ���
	PriceType	LowerLimitPrice;
	///������
	PriceType	PreClosePrice;
	///�ϴν����
	PriceType	PreSettlementPrice;
	///��ֲ���
	LargeVolumeType	PreOpenInterest;


	///�����һ
	PriceType	BidPrice1;
	///������һ
	VolumeType	BidVolume1;
	///������һ
	PriceType	AskPrice1;
	///������һ
	VolumeType	AskVolume1;
	///����۶�
	PriceType	BidPrice2;
	///��������
	VolumeType	BidVolume2;
	///�����۶�
	PriceType	AskPrice2;
	///��������
	VolumeType	AskVolume2;
	///�������
	PriceType	BidPrice3;
	///��������
	VolumeType	BidVolume3;
	///��������
	PriceType	AskPrice3;
	///��������
	VolumeType	AskVolume3;
	///�������
	PriceType	BidPrice4;
	///��������
	VolumeType	BidVolume4;
	///��������
	PriceType	AskPrice4;
	///��������
	VolumeType	AskVolume4;
	///�������
	PriceType	BidPrice5;
	///��������
	VolumeType	BidVolume5;
	///��������
	PriceType	AskPrice5;
	///��������
	VolumeType	AskVolume5;
};

///Tick����
struct TickField
{
	///������ʱ��
	DateIntType			Date;
	TimeIntType			Time;
	TimeIntType			Millisecond;

	PriceType	LastPrice;
	///����
	LargeVolumeType	Volume;
	///�ֲ���
	LargeVolumeType	OpenInterest;
	PriceType	BidPrice1;
	PriceType	AskPrice1;
	VolumeType	BidSize1;
	VolumeType	AskSize1;
};


///Bar����
struct BarField
{
	///������ʱ��
	DateIntType			Date;
	TimeIntType			Time;

	///��
	PriceType	Open;
	///��
	PriceType	High;
	///��
	PriceType	Low;
	///��
	PriceType	Close;
	///����
	LargeVolumeType	Volume;
	///�ֲ���
	LargeVolumeType	OpenInterest;
	///�ɽ����
	MoneyType	Turnover;
};

///���������̵�ѯ������
struct QuoteRequestField
{
	///Ψһ����
	SymbolType			Symbol;
	///��Լ����
	InstrumentIDType	InstrumentID;
	///����������
	ExchangeIDType	ExchangeID;
	///������
	DateType	TradingDay;
	///ѯ�۱��
	OrderIDType	QuoteID;
	///ѯ��ʱ��
	TimeType	QuoteTime;
};

///��Լ
struct InstrumentField
{
	///Ψһ����
	SymbolType			Symbol;
	///��Լ����
	InstrumentIDType	InstrumentID;
	///����������
	ExchangeIDType	ExchangeID;
	///��Լ����
	InstrumentNameType InstrumentName;

	///��Լ����
	InstrumentType		Type;
	///��Լ��������
	VolumeMultipleType	VolumeMultiple;
	///��С�䶯��λ
	PriceType	PriceTick;
	///������
	DateType	ExpireDate;
	///������Ʒ����
	InstrumentIDType	UnderlyingInstrID;
	///ִ�м�
	PriceType	StrikePrice;
	///��Ȩ����
	PutCall	OptionsType;
};

///�˺�
struct AccountField
{
	///�ϴν���׼����
	MoneyType	PreBalance;
	///��ǰ��֤���ܶ�
	MoneyType	CurrMargin;
	///ƽ��ӯ��
	MoneyType	CloseProfit;
	///�ֲ�ӯ��
	MoneyType	PositionProfit;
	///�ڻ�����׼����
	MoneyType	Balance;
	///�����ʽ�
	MoneyType	Available;

	///�����
	MoneyType	Deposit;
	///������
	MoneyType	Withdraw;

	///����Ĺ�����
	MoneyType	FrozenTransferFee;
	///�����ӡ��˰
	MoneyType	FrozenStampTax;
	///�����������
	MoneyType	FrozenCommission;
	///������ʽ�
	MoneyType	FrozenCash;

	///������
	MoneyType	TransferFee;
	///ӡ��˰
	MoneyType	StampTax;
	///������
	MoneyType	Commission;
	///�ʽ���
	MoneyType	CashIn;

};

///�˺�
struct SettlementInfoField
{
	///������
	DateType	TradingDay;
	///��Ϣ����
	ContentType	Content;
};

struct HistoricalDataRequestField
{
	///Ψһ����
	SymbolType			Symbol;
	///��Լ����
	InstrumentIDType	InstrumentID;
	///����������
	ExchangeIDType	ExchangeID;

	int Date1;
	int Date2;
	int Time1;
	int Time2;

	DataObjetType DataType;
	BarType BarType;
	long BarSize;

	int RequestId;
	int CurrentDate;
	int lRequest;
};
#endif
