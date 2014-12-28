#pragma once

#include "../include/LTS/SecurityFtdcTraderApi.h"
#include "../include/ApiStruct.h"

#ifdef _WIN64
#pragma comment(lib, "../include/LTS/win64/securitytraderapi.lib")
#pragma comment(lib, "../lib/QuantBox_Queue_x64.lib")
#else
#pragma comment(lib, "../include/LTS/win32/securitytraderapi.lib")
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
	public CSecurityFtdcTraderSpi
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
		E_QryTradingAccountField,
		E_QryInvestorPositionField,
		E_QryInvestorPositionDetailField,
		E_QryInstrumentCommissionRateField,
		E_QryInstrumentMarginRateField,
		E_QryDepthMarketDataField,
		E_QrySettlementInfoField,
		E_QryOrderField,
		E_QryTradeField,
	};

	//�������ݰ��ṹ��
	struct SRequest
	{
		RequestType type;
		union{
			//CSecurityFtdcReqAuthenticateField				ReqAuthenticateField;
			CSecurityFtdcReqUserLoginField					ReqUserLoginField;
			//CSecurityFtdcSettlementInfoConfirmField		SettlementInfoConfirmField;
			CSecurityFtdcQryDepthMarketDataField			QryDepthMarketDataField;
			CSecurityFtdcQryInstrumentField				QryInstrumentField;
			CSecurityFtdcQryInstrumentCommissionRateField	QryInstrumentCommissionRateField;
			//CSecurityFtdcQryInstrumentMarginRateField		QryInstrumentMarginRateField;
			CSecurityFtdcQryInvestorPositionField			QryInvestorPositionField;
			CSecurityFtdcQryInvestorPositionDetailField    QryInvestorPositionDetailField;
			CSecurityFtdcQryTradingAccountField			QryTradingAccountField;
			CSecurityFtdcInputOrderField					InputOrderField;
			CSecurityFtdcInputOrderActionField				InputOrderActionField;
			//CSecurityFtdcInputQuoteField					InputQuoteField;
			//CSecurityFtdcInputQuoteActionField				InputQuoteActionField;
			//CSecurityFtdcQrySettlementInfoField			QrySettlementInfoField;
			CSecurityFtdcQryOrderField						QryOrderField;
			CSecurityFtdcQryTradeField						QryTradeField;
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

	int ReqOrderAction(const string& szId);
	int ReqOrderAction(CSecurityFtdcOrderField *pOrder);

	int ReqQuoteInsert(
		int QuoteRef,
		OrderField* pOrderAsk,
		OrderField* pOrderBid);

	//int ReqQuoteAction(CSecurityFtdcQuoteField *pQuote);
	//int ReqQuoteAction(const string& szId);

	void ReqQryTradingAccount();
	void ReqQryInvestorPosition(const string& szInstrumentId, const string& szExchange);
	void ReqQryInvestorPositionDetail(const string& szInstrumentId);
	void ReqQryInstrument(const string& szInstrumentId, const string& szExchange);
	void ReqQryInstrumentCommissionRate(const string& szInstrumentId);
	void ReqQryInstrumentMarginRate(const string& szInstrumentId, TSecurityFtdcHedgeFlagType HedgeFlag = SECURITY_FTDC_HF_Speculation);
	void ReqQryDepthMarketData(const string& szInstrumentId);
	void ReqQrySettlementInfo(const string& szTradingDay);

	void ReqQryOrder();
	void ReqQryTrade();

private:
	void OnOrder(CSecurityFtdcOrderField *pOrder, bool bFromQry);
	void OnTrade(CSecurityFtdcTradeField *pTrade, bool bFromQry);

	void OnTrade(TradeField *pTrade, bool bFromQry);

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

	//void ReqAuthenticate();
	void ReqUserLogin();
	//void ReqSettlementInfoConfirm();

	//����Ƿ����
	bool IsErrorRspInfo(CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);//����Ϣ���������Ϣ
	bool IsErrorRspInfo(CSecurityFtdcRspInfoField *pRspInfo);//�������Ϣ

	//����
	virtual void OnFrontConnected();
	virtual void OnFrontDisconnected(int nReason);

	//��֤
	//virtual void OnRspAuthenticate(CSecurityFtdcRspAuthenticateField *pRspAuthenticateField, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspUserLogin(CSecurityFtdcRspUserLoginField *pRspUserLogin, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnRspSettlementInfoConfirm(CSecurityFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//�µ�
	virtual void OnRspOrderInsert(CSecurityFtdcInputOrderField *pInputOrder, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnErrRtnOrderInsert(CSecurityFtdcInputOrderField *pInputOrder, CSecurityFtdcRspInfoField *pRspInfo);

	//����
	virtual void OnRspOrderAction(CSecurityFtdcInputOrderActionField *pInputOrderAction, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnErrRtnOrderAction(CSecurityFtdcOrderActionField *pOrderAction, CSecurityFtdcRspInfoField *pRspInfo);

	//�����ر�
	virtual void OnRspQryOrder(CSecurityFtdcOrderField *pOrder, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRtnOrder(CSecurityFtdcOrderField *pOrder);

	//�ɽ��ر�
	virtual void OnRspQryTrade(CSecurityFtdcTradeField *pTrade, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRtnTrade(CSecurityFtdcTradeField *pTrade);

	//����¼��
	//virtual void OnRspQuoteInsert(CSecurityFtdcInputQuoteField *pInputQuote, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnErrRtnQuoteInsert(CSecurityFtdcInputQuoteField *pInputQuote, CSecurityFtdcRspInfoField *pRspInfo);
	//virtual void OnRtnQuote(CSecurityFtdcQuoteField *pQuote);

	//���۳���
	//virtual void OnRspQuoteAction(CSecurityFtdcInputQuoteActionField *pInputQuoteAction, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnErrRtnQuoteAction(CSecurityFtdcQuoteActionField *pQuoteAction, CSecurityFtdcRspInfoField *pRspInfo);

	//��λ
	virtual void OnRspQryInvestorPosition(CSecurityFtdcInvestorPositionField *pInvestorPosition, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnRspQryInvestorPositionDetail(CSecurityFtdcInvestorPositionDetailField *pInvestorPositionDetail, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnRspQryInvestorPositionCombineDetail(CSecurityFtdcInvestorPositionCombineDetailField *pInvestorPositionCombineDetail, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	//�ʽ�
	virtual void OnRspQryTradingAccount(CSecurityFtdcTradingAccountField *pTradingAccount, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//��Լ��������
	virtual void OnRspQryInstrument(CSecurityFtdcInstrumentField *pInstrument, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnRspQryInstrumentMarginRate(CSecurityFtdcInstrumentMarginRateField *pInstrumentMarginRate, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryInstrumentCommissionRate(CSecurityFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//��ѯ������Ӧ
	virtual void OnRspQryDepthMarketData(CSecurityFtdcDepthMarketDataField *pDepthMarketData, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//�����ѯͶ���߽�������Ӧ
	//virtual void OnRspQrySettlementInfo(CSecurityFtdcSettlementInfoField *pSettlementInfo, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//����
	virtual void OnRspError(CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnRtnInstrumentStatus(CSecurityFtdcInstrumentStatusField *pInstrumentStatus);

private:
	atomic<int>					m_lRequestID;			//����ID,�ñ�������

	CSecurityFtdcRspUserLoginField m_RspUserLogin;			//���صĵ�¼�ɹ���Ӧ��Ŀǰ���ô��ڳ�Ա���б�����������
	OrderIDType					m_orderInsert_Id;

	mutex						m_csOrderRef;
	int							m_nMaxOrderRef;			//�������ã��������ֱ�������������

	CSecurityFtdcTraderApi*		m_pApi;					//����API
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
	unordered_map<string, CSecurityFtdcOrderField*>		m_id_api_order;
	unordered_map<string, string>					m_sysId_orderId;

	//hash_map<string, QuoteField*>				m_id_platform_quote;
	//hash_map<string, CSecurityFtdcQuoteField*>		m_id_api_quote;
	//hash_map<string, string>					m_sysId_quoteId;

	unordered_map<string, PositionField*>			m_id_platform_position;
};

