#ifndef _QUEUE_ENUM_H_
#define _QUEUE_ENUM_H_

enum RequestType :char
{
	GetApiType = 0,
	GetApiVersion,
	GetApiName,

	Create, // ����
	Release, // ����
	Register, // ע��ص�
	Config,		// ���ò���

	Connect, // ��ʼ/����
	Disconnect, // ֹͣ/�Ͽ�

	Clear, // ����
	Process, // ����

	Subscribe,	// ����
	Unsubscribe, // ȡ������

	SubscribeQuote, // ����ѯ��
	UnsubscribeQuote, // ȡ������ѯ��

	ReqOrderInsert,
	ReqQuoteInsert,
	ReqOrderAction,
	ReqQuoteAction,

	ReqQryOrder,
	ReqQryTrade,

	ReqQryInstrument,
	ReqQryTradingAccount,
	ReqQryInvestorPosition,
	ReqQryInvestorPositionDetail,
	ReqQryInstrumentCommissionRate,
	ReqQryInstrumentMarginRate,
	ReqQrySettlementInfo,

	ReqQryHistoricalTicks,
	ReqQryHistoricalBars,
};

enum ResponeType :char
{
	OnConnectionStatus = 64,
	OnRtnDepthMarketData,
	OnRtnError,
	OnRspQryInstrument,
	OnRspQryTradingAccount,
	OnRspQryInvestorPosition,
	OnRspQrySettlementInfo,
	OnRtnOrder,
	OnRtnTrade,

	OnRtnQuote,
	OnRtnQuoteRequest,

	OnRspQryHistoricalTicks,
	OnRspQryHistoricalBars,
};

#endif
