#pragma once

#include "../include/Esunny_HistoricalData/EsunnyQuot.h"
#include "../include/ApiStruct.h"

#ifdef _WIN64
#pragma comment(lib, "../lib/QuantBox_Queue_x64.lib")
#else
#pragma comment(lib, "../include/Esunny_HistoricalData/win32/EsunnyQuot.lib")
#pragma comment(lib, "../lib/QuantBox_Queue_x86.lib")
#endif

#include <set>
#include <list>
#include <map>
#include <string>
#include <mutex>
#include <atomic>
#include <thread>
#include <hash_map>
#include <time.h>

using namespace std;

class CHistoricalDataApi :
	public IEsunnyQuotNotify
{
	//�������ݰ�����
	enum RequestType
	{
		E_Init,
		E_ReqQryHistoricalTicks,
		E_ReqQryHistoricalBars,
		E_ReqQryHistoricalTicks_Check,
	};

	//�������ݰ��ṹ��
	struct SRequest
	{
		RequestType type;
		void* pBuf;
	};

public:
	CHistoricalDataApi(void);
	virtual ~CHistoricalDataApi(void);

	void Register(void* pMsgQueue);

	void Connect(const string& szPath,
		ServerInfoField* pServerInfo,
		UserInfoField* pUserInfo);
	void Disconnect();

	int ReqQryHistoricalTicks(HistoricalDataRequestField* request);
	int ReqQryHistoricalBars(HistoricalDataRequestField* request);

private:
	int ReqQryHistoricalTicks_(HistoricalDataRequestField* request, int lRequest);
	int ReqQryHistoricalBars_(HistoricalDataRequestField* request, int lRequest);

	int ReqQryHistoricalTicks_Check();
	int RtnEmptyRspQryHistoricalTicks();

	//���ݰ������߳�
	static void ProcessThread(CHistoricalDataApi* lpParam)
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

	int ReqInit();
	
	virtual int __cdecl OnRspLogin(int err, const char *errtext);
	virtual int __cdecl OnChannelLost(int err, const char *errtext);
	virtual int __cdecl OnStkQuot(struct STKDATA *pData);
	virtual int __cdecl OnRspHistoryQuot(struct STKHISDATA *pHisData);
	virtual int __cdecl OnRspTraceData(struct STKTRACEDATA *pTraceData);
	virtual int __cdecl OnRspMarketInfo(struct MarketInfo *pMarketInfo, int bLast);

private:
	atomic<long>				m_lRequestID;			//����ID,�ñ�������

	IEsunnyQuotClient*			m_pApi;					//����API
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

	HistoricalDataRequestField	m_RequestTick;
	HistoricalDataRequestField	m_RequestBar;

	int							m_nHdRequestId;	//
	time_t						m_timer_1;
	time_t						m_timer_2;
};

