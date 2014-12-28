#pragma once

#include "../include/CTP/ThostFtdcTraderApi.h"
#include "../include/ApiStruct.h"

#ifdef _WIN64
#pragma comment(lib, "../include/CTP/win64/thosttraderapi.lib")
#pragma comment(lib, "../lib/QuantBox_Queue_x64.lib")
#else
#pragma comment(lib, "../include/CTP/win32/thosttraderapi.lib")
#pragma comment(lib, "../lib/QuantBox_Queue_x86.lib")
#endif


#include <set>
#include <list>
#include <map>
#include <string>
#include <mutex>
#include <atomic>
#include <thread>
#include <unordered_map>

using namespace std;


class CTraderApi :
	public CThostFtdcTraderSpi
{
	//�������ݰ�����
	enum RequestType
	{
		E_ReqAuthenticateField,
		E_ReqUserLoginField,
		E_SettlementInfoConfirmField,
		E_QryInstrumentField,
		E_InputOrderField,
		E_InputOrderActionField,
		E_InputQuoteField,
		E_InputQuoteActionField,
		E_ParkedOrderField,
		E_QryTradingAccountField,
		E_QryInvestorPositionField,
		E_QryInvestorPositionDetailField,
		E_QryInstrumentCommissionRateField,
		E_QryInstrumentMarginRateField,
		E_QryDepthMarketDataField,
		E_QrySettlementInfoField,
		E_QryOrderField,
		E_QryTradeField,
		E_QryQuoteField,
	};

	//�������ݰ��ṹ��
	struct SRequest
	{
		RequestType type;
		union{
			CThostFtdcReqAuthenticateField				ReqAuthenticateField;
			CThostFtdcReqUserLoginField					ReqUserLoginField;
			CThostFtdcSettlementInfoConfirmField		SettlementInfoConfirmField;
			CThostFtdcQryDepthMarketDataField			QryDepthMarketDataField;
			CThostFtdcQryInstrumentField				QryInstrumentField;
			CThostFtdcQryInstrumentCommissionRateField	QryInstrumentCommissionRateField;
			CThostFtdcQryInstrumentMarginRateField		QryInstrumentMarginRateField;
			CThostFtdcQryInvestorPositionField			QryInvestorPositionField;
			CThostFtdcQryInvestorPositionDetailField    QryInvestorPositionDetailField;
			CThostFtdcQryTradingAccountField			QryTradingAccountField;
			CThostFtdcInputOrderField					InputOrderField;
			CThostFtdcInputOrderActionField				InputOrderActionField;
			CThostFtdcInputQuoteField					InputQuoteField;
			CThostFtdcInputQuoteActionField				InputQuoteActionField;
			CThostFtdcParkedOrderField					ParkedOrderField;
			CThostFtdcQrySettlementInfoField			QrySettlementInfoField;
			CThostFtdcQryOrderField						QryOrderField;
			CThostFtdcQryTradeField						QryTradeField;
			CThostFtdcQryQuoteField						QryQuoteField;
		};
	};

public:
	CTraderApi(void);
	virtual ~CTraderApi(void);

	void Register(void* pMsgQueue);

	void Connect(const string& szPath,
		ServerInfoField* pServerInfo,
		UserInfoField* pUserInfo);
	void Disconnect();

	char* ReqOrderInsert(
		int OrderRef,
		OrderField* pOrder1,
		OrderField* pOrder2);

	char* ReqParkedOrderInsert(int OrderRef,
		OrderField* pOrder1,
		OrderField* pOrder2);

	int ReqOrderAction(const string& szId);
	int ReqOrderAction(CThostFtdcOrderField *pOrder);

	char* ReqQuoteInsert(
		int QuoteRef,
		QuoteField* pQuote);

	int ReqQuoteAction(CThostFtdcQuoteField *pQuote);
	int ReqQuoteAction(const string& szId);

	void ReqQryTradingAccount();
	void ReqQryInvestorPosition(const string& szInstrumentId, const string& szExchange);
	void ReqQryInvestorPositionDetail(const string& szInstrumentId);
	void ReqQryInstrument(const string& szInstrumentId, const string& szExchange);
	void ReqQryInstrumentCommissionRate(const string& szInstrumentId);
	void ReqQryInstrumentMarginRate(const string& szInstrumentId,TThostFtdcHedgeFlagType HedgeFlag = THOST_FTDC_HF_Speculation);
	void ReqQryDepthMarketData(const string& szInstrumentId);
	void ReqQrySettlementInfo(const string& szTradingDay);

	void ReqQryOrder();
	void ReqQryTrade();
	void ReqQryQuote();

private:
	void OnOrder(CThostFtdcOrderField *pOrder);
	void OnTrade(CThostFtdcTradeField *pTrade);
	void OnQuote(CThostFtdcQuoteField *pQuote);

	void OnTrade(TradeField *pTrade);

	//���ݰ������߳�
	static void ProcessThread(CTraderApi* lpParam)
	{
		if (lpParam)
			lpParam->RunInThread();
	}
	void RunInThread();
	void StartThread();
	void StopThread();

	//ָ�����ݰ����ͣ����ɶ�Ӧ���ݰ�
	SRequest * MakeRequestBuf(RequestType type);
	//������������������
	void ReleaseRequestListBuf();
	//����ѷ����������
	void ReleaseRequestMapBuf();
	//���ָ�����������ָ����
	void ReleaseRequestMapBuf(int nRequestID);
	//��ӵ��Ѿ��������
	void AddRequestMapBuf(int nRequestID,SRequest* pRequest);
	//��ӵ������Ͱ�����
	void AddToSendQueue(SRequest * pRequest);

	void ReqAuthenticate();
	void ReqUserLogin();
	void ReqSettlementInfoConfirm();

	//����Ƿ����
	bool IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);//����Ϣ���������Ϣ
	bool IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo);//�������Ϣ

	//����
	virtual void OnFrontConnected();
	virtual void OnFrontDisconnected(int nReason);

	//��֤
	virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//�µ�
	virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo);

	//����
	virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo);

	//�����ر�
	virtual void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);

	//�ɽ��ر�
	virtual void OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);

	//����¼��
	virtual void OnRspQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnErrRtnQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo);
	virtual void OnRspQryQuote(CThostFtdcQuoteField *pQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRtnQuote(CThostFtdcQuoteField *pQuote);

	//���۳���
	virtual void OnRspQuoteAction(CThostFtdcInputQuoteActionField *pInputQuoteAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnErrRtnQuoteAction(CThostFtdcQuoteActionField *pQuoteAction, CThostFtdcRspInfoField *pRspInfo);

	//��λ
	virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryInvestorPositionCombineDetail(CThostFtdcInvestorPositionCombineDetailField *pInvestorPositionCombineDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	//�ʽ�
	virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//��Լ��������
	virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//��ѯ������Ӧ
	virtual void OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//�����ѯͶ���߽�������Ӧ
	virtual void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//����
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus);

private:
	atomic<int>					m_lRequestID;			//����ID,�ñ�������

	CThostFtdcRspUserLoginField m_RspUserLogin;			//���صĵ�¼�ɹ���Ӧ��Ŀǰ���ô��ڳ�Ա���б�����������
	
	OrderIDType					m_orderInsert_Id;

	mutex						m_csOrderRef;
	int							m_nMaxOrderRef;			//�������ã��������ֱ�������������

	CThostFtdcTraderApi*		m_pApi;					//����API
	void*						m_msgQueue;				//��Ϣ����ָ��

	string						m_szPath;				//���������ļ���·��
	ServerInfoField				m_ServerInfo;
	UserInfoField				m_UserInfo;

	int							m_nSleep;
	volatile bool				m_bRunning;
	thread*						m_hThread;

	mutex						m_csList;
	list<SRequest*>				m_reqList;				//�������������

	mutex						m_csMap;
	map<int,SRequest*>			m_reqMap;				//�ѷ��������

	unordered_map<string, OrderField*>				m_id_platform_order;
	unordered_map<string, CThostFtdcOrderField*>		m_id_api_order;
	unordered_map<string, string>					m_sysId_orderId;

	unordered_map<string, QuoteField*>				m_id_platform_quote;
	unordered_map<string, CThostFtdcQuoteField*>		m_id_api_quote;
	unordered_map<string, string>					m_sysId_quoteId;

	unordered_map<string, PositionField*>			m_id_platform_position;
};

