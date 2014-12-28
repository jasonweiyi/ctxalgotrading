#pragma once

#include "../include/XSpeed/DFITCMdApi.h"
#include "../include/ApiStruct.h"

#ifdef _WIN64
#pragma comment(lib, "../include/XSpeed/win64/DFITCMdApi.lib")
#pragma comment(lib, "../lib/QuantBox_Queue_x64.lib")
#else
#pragma comment(lib, "../include/XSpeed/win32/DFITCMdApi.lib")
#pragma comment(lib, "../lib/QuantBox_Queue_x86.lib")
#endif

#include <set>
#include <list>
#include <map>
#include <hash_map>
#include <string>
#include <mutex>
#include <thread>
#include <atomic>

using namespace std;
using namespace DFITCXSPEEDMDAPI;

class CMdUserApi :
	public DFITCMdSpi
{
	//�������ݰ�����
	enum RequestType
	{
		E_Init,
		E_UserLoginField,
	};

	//�������ݰ��ṹ��
	struct SRequest
	{
		RequestType type;
		void* pBuf;
	};

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
	//���ݰ������߳�
	//���ݰ������߳�
	static void ProcessThread(CMdUserApi* lpParam)
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
	void AddRequestMapBuf(int nRequestID, SRequest* pRequest);
	//��ӵ������Ͱ�����
	void AddToSendQueue(SRequest * pRequest);

	int ReqInit();


	//��������
	void Subscribe(const set<string>& instrumentIDs, const string& szExchageID);
	void SubscribeQuote(const set<string>& instrumentIDs, const string& szExchageID);
	//��¼����
	void ReqUserLogin();

	virtual void OnFrontConnected();
	virtual void OnFrontDisconnected(int nReason);
	virtual void OnRspUserLogin(struct DFITCUserLoginInfoRtnField * pRspUserLogin, struct DFITCErrorRtnField * pRspInfo);
	virtual void OnRspError(struct DFITCErrorRtnField *pRspInfo);

	virtual void OnRspSubMarketData(struct DFITCSpecificInstrumentField * pSpecificInstrument, struct DFITCErrorRtnField * pRspInfo);
	virtual void OnRspUnSubMarketData(struct DFITCSpecificInstrumentField * pSpecificInstrument, struct DFITCErrorRtnField * pRspInfo);
	virtual void OnMarketData(struct DFITCDepthMarketDataField *pMarketDataField);

	//virtual void OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp);

	//����Ƿ����
	bool IsErrorRspInfo_Output(struct DFITCErrorRtnField *pRspInfo);//��������Ϣ�͵���Ϣ����
	bool IsErrorRspInfo(struct DFITCErrorRtnField *pRspInfo);//���ͳ�����Ϣ

private:
	mutex						m_csMapInstrumentIDs;
	mutex						m_csMapQuoteInstrumentIDs;

	atomic<long>				m_lRequestID;			//����ID��ÿ������ǰ����

	set<string>					m_setInstrumentIDs;		//���ڶ��ĵĺ�Լ
	set<string>					m_setQuoteInstrumentIDs;		//���ڶ��ĵĺ�Լ
	DFITCMdApi*					m_pApi;					//����API
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
	map<int, SRequest*>			m_reqMap;
};

