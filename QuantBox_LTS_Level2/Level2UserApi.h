#pragma once

#include "../include/LTS_L2/SecurityFtdcL2MDUserApi.h"
#include "../include/ApiStruct.h"

#ifdef _WIN64
#pragma comment(lib, "../include/LTS_L2/win64/L2mduserapi.lib")
#pragma comment(lib, "../lib/QuantBox_Queue_x64.lib")
#else
#pragma comment(lib, "../include/LTS_L2/win32/L2mduserapi.lib")
#pragma comment(lib, "../lib/QuantBox_Queue_x86.lib")
#endif

#include <set>
#include <string>
#include <atomic>
#include <mutex>
#include <map>

using namespace std;
_USING_LTS_NS_

class CLevel2UserApi :public CSecurityFtdcL2MDUserSpi
{
public:
	CLevel2UserApi(void);
	virtual ~CLevel2UserApi(void);

	void Register(void* pMsgQueue);

	void Connect(const string& szPath,
		ServerInfoField* pServerInfo,
		UserInfoField* pUserInfo);
	void Disconnect();

	void SubscribeL2MarketData(const string& szInstrumentIDs, const string& szExchageID);
	void UnSubscribeL2MarketData(const string& szInstrumentIDs, const string& szExchageID);
	void SubscribeL2Index(const string& szInstrumentIDs, const string& szExchageID);
	void UnSubscribeL2Index(const string& szInstrumentIDs, const string& szExchageID);

	void SubscribeL2OrderAndTrade();
	void UnSubscribeL2OrderAndTrade();

private:
	//��������
	void SubscribeL2MarketData(const set<string>& instrumentIDs, const string& szExchageID);
	void SubscribeL2Index(const set<string>& instrumentIDs, const string& szExchageID);

	//��¼����
	void ReqUserLogin();

	virtual void OnFrontConnected();
	virtual void OnFrontDisconnected(int nReason);
	virtual void OnRspUserLogin(CSecurityFtdcUserLoginField *pUserLogin, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspError(CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	///Level2����
	virtual void OnRspSubL2MarketData(CSecurityFtdcSpecificInstrumentField *pSpecificInstrument, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspUnSubL2MarketData(CSecurityFtdcSpecificInstrumentField *pSpecificInstrument, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRtnL2MarketData(CSecurityFtdcL2MarketDataField *pL2MarketData);
	///Level2ָ������
	virtual void OnRspSubL2Index(CSecurityFtdcSpecificInstrumentField *pSpecificInstrument, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspUnSubL2Index(CSecurityFtdcSpecificInstrumentField *pSpecificInstrument, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRtnL2Index(CSecurityFtdcL2IndexField *pL2Index);

private:
	//����Ƿ����
	bool IsErrorRspInfo(CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);//��������Ϣ�͵���Ϣ����
	bool IsErrorRspInfo(CSecurityFtdcRspInfoField *pRspInfo);//���ͳ�����Ϣ

private:
	mutex						m_csMapSecurityIDs;
	mutex						m_csMapIndexIDs;

	int							m_nRequestID;			//����ID��ÿ������ǰ����

	map<string,set<string> >	m_mapSecurityIDs;		//���ڶ��ĵĺ�Լ
	map<string,set<string> >	m_mapIndexIDs;			//���ڶ��ĵĺ�Լ

	CSecurityFtdcL2MDUserApi*	m_pApi;					//����API
	void*						m_msgQueue;				//��Ϣ����ָ��

	string						m_szPath;				//���������ļ���·��
	ServerInfoField				m_ServerInfo;
	UserInfoField				m_UserInfo;
};

