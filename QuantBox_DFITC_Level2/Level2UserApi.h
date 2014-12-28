#pragma once

#include "../include/DFITC_L2/DFITCL2Api.h"

#include "../include/ApiStruct.h"

#ifdef _WIN64
#pragma comment(lib, "../lib/QuantBox_Queue_x64.lib")
#else
#pragma comment(lib, "../include/DFITC_L2/win32/level2Api.lib")
#pragma comment(lib, "../lib/QuantBox_Queue_x86.lib")
#endif

#include <set>
#include <string>
#include <atomic>
#include <mutex>
#include <map>
#include <list>
#include <thread>

using namespace std;
using namespace DFITC_L2;

class CLevel2UserApi :public DFITCL2Spi
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
	CLevel2UserApi(void);
	virtual ~CLevel2UserApi(void);

	void Register(void* pMsgQueue);

	void Connect(const string& szPath,
		ServerInfoField* pServerInfo,
		UserInfoField* pUserInfo);
	void Disconnect();

	void Subscribe(const string& szInstrumentIDs, const string& szExchageID);
	void Unsubscribe(const string& szInstrumentIDs, const string& szExchageID);

	void SubscribeAll();
	void UnsubscribeAll();

private:
	//���ݰ������߳�
	//���ݰ������߳�
	static void ProcessThread(CLevel2UserApi* lpParam)
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

	//��������
	void Subscribe(const set<string>& instrumentIDs, const string& szExchageID);
	void Unsubscribe(const set<string>& instrumentIDs, const string& szExchageID);

	//��¼����
	void ReqUserLogin();
	int ReqInit();

	virtual void OnConnected();
	virtual void OnDisconnected(int nReason);
	virtual void OnRspUserLogin(struct ErrorRtnField * pErrorField);

	virtual void OnRspSubscribeMarketData(struct ErrorRtnField * pErrorField);
	virtual void OnRspUnSubscribeMarketData(struct ErrorRtnField * pErrorField);
	virtual void OnRspSubscribeAll(struct ErrorRtnField * pErrorField);
	virtual void OnRspUnSubscribeAll(struct ErrorRtnField * pErrorField);

	virtual void OnBestAndDeep(MDBestAndDeep * const pQuote);
	virtual void OnArbi(MDBestAndDeep * const pQuote);
	virtual void OnTenEntrust(MDTenEntrust * const pQuote);
	virtual void OnRealtime(MDRealTimePrice * const pQuote);
	virtual void OnOrderStatistic(MDOrderStatistic * const pQuote);
	virtual void OnMarchPrice(MDMarchPriceQty * const pQuote);

	virtual void OnHeartBeatLost() { }
	//����Ƿ����
	bool IsErrorRspInfo_Output(struct ErrorRtnField * pErrorField);//��������Ϣ�͵���Ϣ����
	bool IsErrorRspInfo(struct ErrorRtnField * pErrorField); //�������Ϣ

private:
	mutex						m_csMapInstrumentIDs;

	atomic<long>				m_lRequestID;			//����ID��ÿ������ǰ����

	set<string>					m_setInstrumentIDs;

	DFITCL2Api*					m_pApi;					//����API
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
	map<int, SRequest*>			m_reqMap;				//�ѷ��������
};

