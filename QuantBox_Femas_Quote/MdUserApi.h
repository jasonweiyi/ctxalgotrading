#pragma once

#include "../include/Femas/USTPFtdcMduserApi.h"
#include "../include/ApiStruct.h"

#ifdef _WIN64
#pragma comment(lib, "../include/Femas/win64/USTPmduserapi.lib")
#pragma comment(lib, "../lib/QuantBox_Queue_x64.lib")
#else
#pragma comment(lib, "../include/Femas/win32/USTPmduserapi.lib")
#pragma comment(lib, "../lib/QuantBox_Queue_x86.lib")
#endif

#include <set>
#include <string>
#include <atomic>
#include <mutex>

using namespace std;

class CMdUserApi :
	public CUstpFtdcMduserSpi
{
public:
	CMdUserApi(void);
	virtual ~CMdUserApi(void);

	void Register(void* pMsgQueue);
	ConfigInfoField* Config(ConfigInfoField* pConfigInfo);

	void Connect(const string& szPath,
		ServerInfoField* pServerInfo,
		UserInfoField* pUserInfo);
	void Disconnect();

	void Subscribe(const string& szInstrumentIDs, const string& szExchageID);
	void Unsubscribe(const string& szInstrumentIDs, const string& szExchageID);

	void SubscribeQuote(const string& szInstrumentIDs, const string& szExchageID);
	void UnsubscribeQuote(const string& szInstrumentIDs, const string& szExchageID);

private:
	//��������
	void Subscribe(const set<string>& instrumentIDs, const string& szExchageID);
	void SubscribeQuote(const set<string>& instrumentIDs, const string& szExchageID);
	//��¼����
	void ReqUserLogin();

	virtual void OnFrontConnected();
	virtual void OnFrontDisconnected(int nReason);
	virtual void OnRspUserLogin(CUstpFtdcRspUserLoginField *pRspUserLogin, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspError(CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	virtual void OnRspSubMarketData(CUstpFtdcSpecificInstrumentField *pSpecificInstrument, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspUnSubMarketData(CUstpFtdcSpecificInstrumentField *pSpecificInstrument, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRtnDepthMarketData(CUstpFtdcDepthMarketDataField *pDepthMarketData);

	//����Ƿ����
	bool IsErrorRspInfo(CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);//��������Ϣ�͵���Ϣ����
	bool IsErrorRspInfo(CUstpFtdcRspInfoField *pRspInfo);//���ͳ�����Ϣ

private:
	mutex						m_csMapInstrumentIDs;
	mutex						m_csMapQuoteInstrumentIDs;

	atomic<int>					m_nRequestID;			//����ID��ÿ������ǰ����

	set<string>					m_setInstrumentIDs;		//���ڶ��ĵĺ�Լ
	set<string>					m_setQuoteInstrumentIDs;		//���ڶ��ĵĺ�Լ
	CUstpFtdcMduserApi*			m_pApi;					//����API
	void*						m_msgQueue;				//��Ϣ����ָ��

	string						m_szPath;				//���������ļ���·��
	ServerInfoField				m_ServerInfo;
	UserInfoField				m_UserInfo;
};

