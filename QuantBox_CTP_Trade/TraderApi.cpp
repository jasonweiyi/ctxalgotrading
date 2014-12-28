#include "stdafx.h"
#include "TraderApi.h"

#include "../include/QueueEnum.h"
#include "../include/QueueHeader.h"

#include "../include/ApiHeader.h"
#include "../include/ApiStruct.h"

#include "../include/toolkit.h"

#include "TypeConvert.h"

#include <cstring>
#include <assert.h>

CTraderApi::CTraderApi(void)
{
	m_pApi = nullptr;
	m_msgQueue = nullptr;
	m_lRequestID = 0;

	m_hThread = nullptr;
	m_bRunning = false;
}


CTraderApi::~CTraderApi(void)
{
	Disconnect();
}

void CTraderApi::StartThread()
{
    if(nullptr == m_hThread)
    {
        m_bRunning = true;
        m_hThread = new thread(ProcessThread,this);
    }
}

void CTraderApi::StopThread()
{
    m_bRunning = false;
    if(m_hThread)
    {
        m_hThread->join();
        delete m_hThread;
        m_hThread = nullptr;
    }
}

void CTraderApi::Register(void* pMsgQueue)
{
	m_msgQueue = pMsgQueue;
}

bool CTraderApi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	bool bRet = ((pRspInfo) && (pRspInfo->ErrorID != 0));
	if (bRet)
	{
		ErrorField field = { 0 };
		field.ErrorID = pRspInfo->ErrorID;
		strcpy(field.ErrorMsg, pRspInfo->ErrorMsg);

		XRespone(ResponeType::OnRtnError, m_msgQueue, this, bIsLast, 0, &field, sizeof(ErrorField), nullptr, 0, nullptr, 0);
	}
	return bRet;
}

bool CTraderApi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
	bool bRet = ((pRspInfo) && (pRspInfo->ErrorID != 0));

	return bRet;
}

void CTraderApi::Connect(const string& szPath,
	ServerInfoField* pServerInfo,
	UserInfoField* pUserInfo)
{
	m_szPath = szPath;
	memcpy(&m_ServerInfo, pServerInfo, sizeof(ServerInfoField));
	memcpy(&m_UserInfo, pUserInfo, sizeof(UserInfoField));
	
	char *pszPath = new char[szPath.length() + 1024];
	srand((unsigned int)time(nullptr));
	sprintf(pszPath, "%s/%s/%s/Td/%d/", szPath.c_str(), m_ServerInfo.BrokerID, m_UserInfo.UserID, rand());
	makedirs(pszPath);

	m_pApi = CThostFtdcTraderApi::CreateFtdcTraderApi(pszPath);
	delete[] pszPath;

	XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Initialized, 0, nullptr, 0, nullptr, 0, nullptr, 0);

	if (m_pApi)
	{
		m_pApi->RegisterSpi(this);

		//��ӵ�ַ
		size_t len = strlen(m_ServerInfo.Address) + 1;
		char* buf = new char[len];
		strncpy(buf, m_ServerInfo.Address, len);

		char* token = strtok(buf, _QUANTBOX_SEPS_);
		while(token)
		{
			if (strlen(token)>0)
			{
				m_pApi->RegisterFront(token);
			}
			token = strtok(nullptr, _QUANTBOX_SEPS_);
		}
		delete[] buf;

		m_pApi->SubscribePublicTopic((THOST_TE_RESUME_TYPE)pServerInfo->PublicTopicResumeType);
		m_pApi->SubscribePrivateTopic((THOST_TE_RESUME_TYPE)pServerInfo->PrivateTopicResumeType);

		//��ʼ������
		m_pApi->Init();
		XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Connecting, 0, nullptr, 0, nullptr, 0, nullptr, 0);
	}
}

void CTraderApi::Disconnect()
{
	// �������������������ں����ֽ�����Release,�ֻع�ͷ�����ͣ����ܵ��µ���
	StopThread();

	if(m_pApi)
	{
		m_pApi->RegisterSpi(nullptr);
		m_pApi->Release();
		m_pApi = nullptr;

		XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Disconnected, 0, nullptr, 0, nullptr, 0, nullptr, 0);
	}

	m_lRequestID = 0;//�����߳��Ѿ�ֹͣ��û�б�Ҫ��ԭ�Ӳ�����

	ReleaseRequestListBuf();
	ReleaseRequestMapBuf();
}

CTraderApi::SRequest* CTraderApi::MakeRequestBuf(RequestType type)
{
	SRequest *pRequest = new SRequest;
	if (nullptr == pRequest)
		return nullptr;

	memset(pRequest,0,sizeof(SRequest));
	pRequest->type = type;

	return pRequest;
}

void CTraderApi::ReleaseRequestListBuf()
{
	lock_guard<mutex> cl(m_csList);
	while (!m_reqList.empty())
	{
		SRequest * pRequest = m_reqList.front();
		delete pRequest;
		m_reqList.pop_front();
	}
}

void CTraderApi::ReleaseRequestMapBuf()
{
	lock_guard<mutex> cl(m_csMap);
	for (map<int,SRequest*>::iterator it=m_reqMap.begin();it!=m_reqMap.end();++it)
	{
		delete (*it).second;
	}
	m_reqMap.clear();
}

void CTraderApi::ReleaseRequestMapBuf(int nRequestID)
{
	lock_guard<mutex> cl(m_csMap);
	map<int,SRequest*>::iterator it = m_reqMap.find(nRequestID);
	if (it!=m_reqMap.end())
	{
		delete it->second;
		m_reqMap.erase(nRequestID);
	}
}

void CTraderApi::AddRequestMapBuf(int nRequestID,SRequest* pRequest)
{
	if(nullptr == pRequest)
		return;

	lock_guard<mutex> cl(m_csMap);
	map<int,SRequest*>::iterator it = m_reqMap.find(nRequestID);
	if (it!=m_reqMap.end())
	{
		SRequest* p = it->second;
		if(pRequest != p)//���ʵ����ָ����ͬһ�ڴ棬���ٲ���
		{
			delete p;
			m_reqMap[nRequestID] = pRequest;
		}
	}
}


void CTraderApi::AddToSendQueue(SRequest * pRequest)
{
	if (nullptr == pRequest)
		return;

	lock_guard<mutex> cl(m_csList);
	bool bFind = false;
	//Ŀǰ��ȥ����ͬ���͵����󣬼�û�жԴ���ͬ������������Ż�
	//for (list<SRequest*>::iterator it = m_reqList.begin();it!= m_reqList.end();++it)
	//{
	//	if (pRequest->type == (*it)->type)
	//	{
	//		bFind = true;
	//		break;
	//	}
	//}

	if (!bFind)
		m_reqList.push_back(pRequest);

	if (!m_reqList.empty())
	{
		StartThread();
	}
}


void CTraderApi::RunInThread()
{
	int iRet = 0;

	while (!m_reqList.empty()&&m_bRunning)
	{
		SRequest * pRequest = m_reqList.front();
		int lRequest = ++m_lRequestID;// ����ط��Ƿ�����ԭ�Ӳ����������أ�
		switch(pRequest->type)
		{
		case E_ReqAuthenticateField:
			iRet = m_pApi->ReqAuthenticate(&pRequest->ReqAuthenticateField,lRequest);
			break;
		case E_ReqUserLoginField:
			iRet = m_pApi->ReqUserLogin(&pRequest->ReqUserLoginField,lRequest);
			break;
		case E_SettlementInfoConfirmField:
			iRet = m_pApi->ReqSettlementInfoConfirm(&pRequest->SettlementInfoConfirmField,lRequest);
			break;
		case E_QryInstrumentField:
			iRet = m_pApi->ReqQryInstrument(&pRequest->QryInstrumentField,lRequest);
			break;
		case E_QryTradingAccountField:
			iRet = m_pApi->ReqQryTradingAccount(&pRequest->QryTradingAccountField,lRequest);
			break;
		case E_QryInvestorPositionField:
			iRet = m_pApi->ReqQryInvestorPosition(&pRequest->QryInvestorPositionField,lRequest);
			break;
		case E_QryInvestorPositionDetailField:
			iRet=m_pApi->ReqQryInvestorPositionDetail(&pRequest->QryInvestorPositionDetailField,lRequest);
			break;
		case E_QryInstrumentCommissionRateField:
			iRet = m_pApi->ReqQryInstrumentCommissionRate(&pRequest->QryInstrumentCommissionRateField,lRequest);
			break;
		case E_QryInstrumentMarginRateField:
			iRet = m_pApi->ReqQryInstrumentMarginRate(&pRequest->QryInstrumentMarginRateField,lRequest);
			break;
		case E_QryDepthMarketDataField:
			iRet = m_pApi->ReqQryDepthMarketData(&pRequest->QryDepthMarketDataField,lRequest);
			break;
		case E_QrySettlementInfoField:
			iRet = m_pApi->ReqQrySettlementInfo(&pRequest->QrySettlementInfoField, lRequest);
			break;
		case E_QryOrderField:
			iRet = m_pApi->ReqQryOrder(&pRequest->QryOrderField, lRequest);
			break;
		case E_QryTradeField:
			iRet = m_pApi->ReqQryTrade(&pRequest->QryTradeField, lRequest);
			break;
		case E_QryQuoteField:
			iRet = m_pApi->ReqQryQuote(&pRequest->QryQuoteField, lRequest);
			break;
		default:
			assert(false);
			break;
		}

		if (0 == iRet)
		{
			//���سɹ�����ӵ��ѷ��ͳ�
			m_nSleep = 1;
			AddRequestMapBuf(lRequest,pRequest);

			lock_guard<mutex> cl(m_csList);
			m_reqList.pop_front();
		}
		else
		{
			//ʧ�ܣ���4���ݽ�����ʱ����������1s
			m_nSleep *= 4;
			m_nSleep %= 1023;
		}
		this_thread::sleep_for(chrono::milliseconds(m_nSleep));
	}

	// �����߳�
	m_hThread = nullptr;
	m_bRunning = false;
}

void CTraderApi::OnFrontConnected()
{
	XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Connected, 0, nullptr, 0, nullptr, 0, nullptr, 0);

	//���ӳɹ����Զ�������֤���¼
	if (strlen(m_ServerInfo.AuthCode)>0
		&& strlen(m_ServerInfo.UserProductInfo)>0)
	{
		//������֤�������֤
		ReqAuthenticate();
	}
	else
	{
		ReqUserLogin();
	}
}

void CTraderApi::OnFrontDisconnected(int nReason)
{
	RspUserLoginField field = { 0 };
	//����ʧ�ܷ��ص���Ϣ��ƴ�Ӷ��ɣ���Ҫ��Ϊ��ͳһ���
	field.ErrorID = nReason;
	GetOnFrontDisconnectedMsg(nReason,field.ErrorMsg);

	XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Disconnected, 0, &field, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);

}

void CTraderApi::ReqAuthenticate()
{
	if (nullptr == m_pApi)
		return;

	SRequest* pRequest = MakeRequestBuf(E_ReqAuthenticateField);
	if (pRequest)
	{
		XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Authorizing, 0, nullptr, 0, nullptr, 0, nullptr, 0);

		CThostFtdcReqAuthenticateField& body = pRequest->ReqAuthenticateField;

		strncpy(body.BrokerID, m_ServerInfo.BrokerID, sizeof(TThostFtdcBrokerIDType));
		strncpy(body.UserID, m_UserInfo.UserID, sizeof(TThostFtdcInvestorIDType));
		strncpy(body.UserProductInfo, m_ServerInfo.UserProductInfo, sizeof(TThostFtdcProductInfoType));
		strncpy(body.AuthCode, m_ServerInfo.AuthCode, sizeof(TThostFtdcAuthCodeType));

		AddToSendQueue(pRequest);
	}
}

void CTraderApi::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo)
		&&pRspAuthenticateField)
	{
		XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Authorized, 0, nullptr, 0, nullptr, 0, nullptr, 0);

		ReqUserLogin();
	}
	else
	{
		RspUserLoginField field = { 0 };
		field.ErrorID = pRspInfo->ErrorID;
		strncpy(field.ErrorMsg, pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg));

		XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Disconnected, 0, &field, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);
	}

	if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);
}

void CTraderApi::ReqUserLogin()
{
	if (nullptr == m_pApi)
		return;

	SRequest* pRequest = MakeRequestBuf(E_ReqUserLoginField);
	if (pRequest)
	{
		XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Logining, 0, nullptr, 0, nullptr, 0, nullptr, 0);

		CThostFtdcReqUserLoginField& body = pRequest->ReqUserLoginField;

		strncpy(body.BrokerID, m_ServerInfo.BrokerID, sizeof(TThostFtdcBrokerIDType));
		strncpy(body.UserID, m_UserInfo.UserID, sizeof(TThostFtdcInvestorIDType));
		strncpy(body.Password, m_UserInfo.Password, sizeof(TThostFtdcPasswordType));
		strncpy(body.UserProductInfo, m_ServerInfo.UserProductInfo, sizeof(TThostFtdcProductInfoType));

		AddToSendQueue(pRequest);
	}
}

void CTraderApi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	RspUserLoginField field = { 0 };

	if (!IsErrorRspInfo(pRspInfo)
		&&pRspUserLogin)
	{
		strncpy(field.TradingDay, pRspUserLogin->TradingDay, sizeof(DateType));
		strncpy(field.LoginTime, pRspUserLogin->LoginTime, sizeof(TimeType));
		sprintf(field.SessionID, "%d:%d", pRspUserLogin->FrontID, pRspUserLogin->SessionID);

		XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Logined, 0, &field, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);

		// ���µ�¼��Ϣ�����ܻ��õ�
		memcpy(&m_RspUserLogin,pRspUserLogin,sizeof(CThostFtdcRspUserLoginField));
		m_nMaxOrderRef = atol(pRspUserLogin->MaxOrderRef);
		// �Լ�����ʱID��1��ʼ�����ܴ�0��ʼ
		m_nMaxOrderRef = m_nMaxOrderRef>1 ? m_nMaxOrderRef:1;
		ReqSettlementInfoConfirm();
	}
	else
	{
		field.ErrorID = pRspInfo->ErrorID;
		strncpy(field.ErrorMsg, pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg));

		XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Disconnected, 0, &field, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);
	}

	if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);
}

void CTraderApi::ReqSettlementInfoConfirm()
{
	if (nullptr == m_pApi)
		return;

	SRequest* pRequest = MakeRequestBuf(E_SettlementInfoConfirmField);
	if (pRequest)
	{
		XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Confirming, 0, nullptr, 0, nullptr, 0, nullptr, 0);

		CThostFtdcSettlementInfoConfirmField& body = pRequest->SettlementInfoConfirmField;

		strncpy(body.BrokerID, m_ServerInfo.BrokerID, sizeof(TThostFtdcBrokerIDType));
		strncpy(body.InvestorID, m_UserInfo.UserID, sizeof(TThostFtdcInvestorIDType));

		AddToSendQueue(pRequest);
	}
}

void CTraderApi::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo)
		&&pSettlementInfoConfirm)
	{
		XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Confirmed, 0, nullptr, 0, nullptr, 0, nullptr, 0);
		XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Done, 0, nullptr, 0, nullptr, 0, nullptr, 0);
	}
	else
	{
		RspUserLoginField field = { 0 };
		field.ErrorID = pRspInfo->ErrorID;
		strncpy(field.ErrorMsg, pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg));

		XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Disconnected, 0, &field, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);
	}

	if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);
}

char* CTraderApi::ReqOrderInsert(
	int OrderRef,
	OrderField* pOrder1,
	OrderField* pOrder2)
{
	if (nullptr == m_pApi)
		return nullptr;

	SRequest* pRequest = MakeRequestBuf(E_InputOrderField);
	if (nullptr == pRequest)
		return nullptr;

	CThostFtdcInputOrderField& body = pRequest->InputOrderField;

	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TThostFtdcBrokerIDType));
	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TThostFtdcInvestorIDType));

	body.MinVolume = 1;
	body.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	body.IsAutoSuspend = 0;
	body.UserForceClose = 0;
	body.IsSwapOrder = 0;

	//��Լ
	strncpy(body.InstrumentID, pOrder1->InstrumentID, sizeof(TThostFtdcInstrumentIDType));
	//����
	body.Direction = OrderSide_2_TThostFtdcDirectionType(pOrder1->Side);
	//��ƽ
	body.CombOffsetFlag[0] = OpenCloseType_2_TThostFtdcOffsetFlagType(pOrder1->OpenClose);
	//Ͷ��
	body.CombHedgeFlag[0] = HedgeFlagType_2_TThostFtdcHedgeFlagType(pOrder1->HedgeFlag);
	//����
	body.VolumeTotalOriginal = (int)pOrder1->Qty;

	// ���������������õ�һ�������ļ۸񣬻��������������ļ۸���أ�
	body.LimitPrice = pOrder1->Price;
	body.StopPrice = pOrder1->StopPx;

	// ��Եڶ������д�������еڶ�����������Ϊ�ǽ�����������
	if (pOrder2)
	{
		body.CombOffsetFlag[1] = OpenCloseType_2_TThostFtdcOffsetFlagType(pOrder1->OpenClose);
		body.CombHedgeFlag[1] = HedgeFlagType_2_TThostFtdcHedgeFlagType(pOrder1->HedgeFlag);
		// ���������Ʋֻ��¹��ܣ�û��ʵ���
		body.IsSwapOrder = (body.CombOffsetFlag[0] != body.CombOffsetFlag[1]);
	}
	
	// �м����޼�
	switch (pOrder1->Type)
	{
	case Market:
	case Stop:
	case MarketOnClose:
	case TrailingStop:
		body.OrderPriceType = THOST_FTDC_OPT_AnyPrice;
		body.TimeCondition = THOST_FTDC_TC_IOC;
		body.LimitPrice = 0;
		break;
	case Limit:
	case StopLimit:
	case TrailingStopLimit:
	default:
		body.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
		body.TimeCondition = THOST_FTDC_TC_GFD;
		break;
	}

	// IOC��FOK
	switch (pOrder1->TimeInForce)
	{
	case IOC:
		body.TimeCondition = THOST_FTDC_TC_IOC;
		body.VolumeCondition = THOST_FTDC_VC_AV;
		break;
	case FOK:
		body.TimeCondition = THOST_FTDC_TC_IOC;
		body.VolumeCondition = THOST_FTDC_VC_CV;
		//body.MinVolume = body.VolumeTotalOriginal; // ����ط��������
		break;
	default:
		body.VolumeCondition = THOST_FTDC_VC_AV;
		break;
	}

	// ������
	switch (pOrder1->Type)
	{
	case Stop:
	case TrailingStop:
	case StopLimit:
	case TrailingStopLimit:
		// ������û�в��ԣ�������
		body.ContingentCondition = THOST_FTDC_CC_Immediately;
		break;
	default:
		body.ContingentCondition = THOST_FTDC_CC_Immediately;
		break;
	}

	int nRet = 0;
	{
		//���ܱ���̫�죬m_nMaxOrderRef��û�иı���ύ��
		lock_guard<mutex> cl(m_csOrderRef);

		if (OrderRef < 0)
		{
			nRet = m_nMaxOrderRef;
			++m_nMaxOrderRef;
		}
		else
		{
			nRet = OrderRef;
		}
		sprintf(body.OrderRef, "%d", nRet);

		//�����浽���У�����ֱ�ӷ���
		int n = m_pApi->ReqOrderInsert(&pRequest->InputOrderField, ++m_lRequestID);
		if (n < 0)
		{
			nRet = n;
			delete pRequest;
			return nullptr;
		}
		else
		{
			// ���ڸ���������ҵ�ԭ���������ڽ�����Ӧ��֪ͨ
			sprintf(m_orderInsert_Id, "%d:%d:%d", m_RspUserLogin.FrontID, m_RspUserLogin.SessionID, nRet);

			OrderField* pField = new OrderField();
			memcpy(pField, pOrder1, sizeof(OrderField));
			strcpy(pField->ID, m_orderInsert_Id);
			m_id_platform_order.insert(pair<string, OrderField*>(m_orderInsert_Id, pField));
		}
	}
	delete pRequest;//�����ֱ��ɾ��

	return m_orderInsert_Id;
}

void CTraderApi::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	OrderIDType orderId = { 0 };
	sprintf(orderId, "%d:%d:%s", m_RspUserLogin.FrontID, m_RspUserLogin.SessionID, pInputOrder->OrderRef);

	unordered_map<string, OrderField*>::iterator it = m_id_platform_order.find(orderId);
	if (it == m_id_platform_order.end())
	{
		// û�ҵ�����Ӧ�������ʾ������
		assert(false);
	}
	else
	{
		// �ҵ��ˣ�Ҫ����״̬
		// ��ʹ���ϴε�״̬
		OrderField* pField = it->second;
		pField->ExecType = ExecType::ExecRejected;
		pField->Status = OrderStatus::Rejected;
		pField->ErrorID = pRspInfo->ErrorID;
		strncpy(pField->Text, pRspInfo->ErrorMsg, sizeof(TThostFtdcErrorMsgType));
		XRespone(ResponeType::OnRtnOrder, m_msgQueue, this, 0, 0, pField, sizeof(OrderField), nullptr, 0, nullptr, 0);
	}
}

void CTraderApi::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
{
	OrderIDType orderId = { 0 };
	sprintf(orderId, "%d:%d:%s", m_RspUserLogin.FrontID, m_RspUserLogin.SessionID, pInputOrder->OrderRef);

	unordered_map<string, OrderField*>::iterator it = m_id_platform_order.find(orderId);
	if (it == m_id_platform_order.end())
	{
		// û�ҵ�����Ӧ�������ʾ������
		assert(false);
	}
	else
	{
		// �ҵ��ˣ�Ҫ����״̬
		// ��ʹ���ϴε�״̬
		OrderField* pField = it->second;
		pField->ExecType = ExecType::ExecRejected;
		pField->Status = OrderStatus::Rejected;
		pField->ErrorID = pRspInfo->ErrorID;
		strncpy(pField->Text, pRspInfo->ErrorMsg, sizeof(TThostFtdcErrorMsgType));
		XRespone(ResponeType::OnRtnOrder, m_msgQueue, this, 0, 0, pField, sizeof(OrderField), nullptr, 0, nullptr, 0);
	}
}

char* CTraderApi::ReqParkedOrderInsert(
	int OrderRef,
	OrderField* pOrder1,
	OrderField* pOrder2)
{
	if (nullptr == m_pApi)
		return nullptr;

	SRequest* pRequest = MakeRequestBuf(E_ParkedOrderField);
	if (nullptr == pRequest)
		return nullptr;

	CThostFtdcParkedOrderField& body = pRequest->ParkedOrderField;

	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TThostFtdcBrokerIDType));
	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TThostFtdcInvestorIDType));

	body.MinVolume = 1;
	body.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	body.IsAutoSuspend = 0;
	body.UserForceClose = 0;
	body.IsSwapOrder = 0;

	//��Լ
	strncpy(body.InstrumentID, pOrder1->InstrumentID, sizeof(TThostFtdcInstrumentIDType));
	//����
	body.Direction = OrderSide_2_TThostFtdcDirectionType(pOrder1->Side);
	//��ƽ
	body.CombOffsetFlag[0] = OpenCloseType_2_TThostFtdcOffsetFlagType(pOrder1->OpenClose);
	//Ͷ��
	body.CombHedgeFlag[0] = HedgeFlagType_2_TThostFtdcHedgeFlagType(pOrder1->HedgeFlag);
	//����
	body.VolumeTotalOriginal = (int)pOrder1->Qty;

	// ���������������õ�һ�������ļ۸񣬻��������������ļ۸���أ�
	body.LimitPrice = pOrder1->Price;
	body.StopPrice = pOrder1->StopPx;

	// ��Եڶ������д�������еڶ�����������Ϊ�ǽ�����������
	if (pOrder2)
	{
		body.CombOffsetFlag[1] = OpenCloseType_2_TThostFtdcOffsetFlagType(pOrder1->OpenClose);
		body.CombHedgeFlag[1] = HedgeFlagType_2_TThostFtdcHedgeFlagType(pOrder1->HedgeFlag);
		// ���������Ʋֻ��¹��ܣ�û��ʵ���
		body.IsSwapOrder = (body.CombOffsetFlag[0] != body.CombOffsetFlag[1]);
	}

	//�۸�
	//body.OrderPriceType = OrderType_2_TThostFtdcOrderPriceTypeType(pOrder1->Type);

	// �м����޼�
	switch (pOrder1->Type)
	{
	case Market:
	case Stop:
	case MarketOnClose:
	case TrailingStop:
		body.OrderPriceType = THOST_FTDC_OPT_AnyPrice;
		body.TimeCondition = THOST_FTDC_TC_IOC;
		break;
	case Limit:
	case StopLimit:
	case TrailingStopLimit:
	default:
		body.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
		body.TimeCondition = THOST_FTDC_TC_GFD;
		break;
	}

	// IOC��FOK
	switch (pOrder1->TimeInForce)
	{
	case IOC:
		body.TimeCondition = THOST_FTDC_TC_IOC;
		body.VolumeCondition = THOST_FTDC_VC_AV;
		break;
	case FOK:
		body.TimeCondition = THOST_FTDC_TC_IOC;
		body.VolumeCondition = THOST_FTDC_VC_CV;
		//body.MinVolume = body.VolumeTotalOriginal; // ����ط��������
		break;
	default:
		body.VolumeCondition = THOST_FTDC_VC_AV;
		break;
	}

	// ������
	switch (pOrder1->Type)
	{
	case Stop:
	case TrailingStop:
	case StopLimit:
	case TrailingStopLimit:
		// ������û�в��ԣ�������
		body.ContingentCondition = THOST_FTDC_CC_Immediately;
		break;
	default:
		body.ContingentCondition = THOST_FTDC_CC_Immediately;
		break;
	}

	int nRet = 0;
	{
		//���ܱ���̫�죬m_nMaxOrderRef��û�иı���ύ��
		lock_guard<mutex> cl(m_csOrderRef);

		if (OrderRef < 0)
		{
			nRet = m_nMaxOrderRef;
			++m_nMaxOrderRef;
		}
		else
		{
			nRet = OrderRef;
		}
		sprintf(body.OrderRef, "%d", nRet);

		//�����浽���У�����ֱ�ӷ���
		int n = m_pApi->ReqParkedOrderInsert(&pRequest->ParkedOrderField, ++m_lRequestID);
		if (n < 0)
		{
			nRet = n;
			delete pRequest;
			return nullptr;
		}
		else
		{
			sprintf(m_orderInsert_Id, "%d:%d:%d", m_RspUserLogin.FrontID, m_RspUserLogin.SessionID, nRet);

			OrderField* pField = new OrderField();
			memcpy(pField, pOrder1, sizeof(OrderField));
			m_id_platform_order.insert(pair<string, OrderField*>(m_orderInsert_Id, pField));
		}
	}
	delete pRequest;//�����ֱ��ɾ��

	return m_orderInsert_Id;
}

void CTraderApi::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
	OnTrade(pTrade);
}

int CTraderApi::ReqOrderAction(const string& szId)
{
	unordered_map<string, CThostFtdcOrderField*>::iterator it = m_id_api_order.find(szId);
	if (it == m_id_api_order.end())
	{
		// <error id="ORDER_NOT_FOUND" value="25" prompt="CTP:�����Ҳ�����Ӧ����"/>
		//ErrorField field = { 0 };
		//field.ErrorID = 25;
		//sprintf(field.ErrorMsg, "ORDER_NOT_FOUND");

		////TODO:Ӧ��ͨ�������ر�֪ͨ�����Ҳ���

		//XRespone(ResponeType::OnRtnError, m_msgQueue, this, 0, 0, &field, sizeof(ErrorField), nullptr, 0, nullptr, 0);
		return -100;
	}
	else
	{
		// �ҵ��˶���
		return ReqOrderAction(it->second);
	}
}

int CTraderApi::ReqOrderAction(CThostFtdcOrderField *pOrder)
{
	if (nullptr == m_pApi)
		return 0;

	SRequest* pRequest = MakeRequestBuf(E_InputOrderActionField);
	if (nullptr == pRequest)
		return 0;

	CThostFtdcInputOrderActionField& body = pRequest->InputOrderActionField;

	///���͹�˾����
	strncpy(body.BrokerID, pOrder->BrokerID,sizeof(TThostFtdcBrokerIDType));
	///Ͷ���ߴ���
	strncpy(body.InvestorID, pOrder->InvestorID,sizeof(TThostFtdcInvestorIDType));
	///��������
	strncpy(body.OrderRef, pOrder->OrderRef,sizeof(TThostFtdcOrderRefType));
	///ǰ�ñ��
	body.FrontID = pOrder->FrontID;
	///�Ự���
	body.SessionID = pOrder->SessionID;
	///����������
	strncpy(body.ExchangeID,pOrder->ExchangeID,sizeof(TThostFtdcExchangeIDType));
	///�������
	strncpy(body.OrderSysID,pOrder->OrderSysID,sizeof(TThostFtdcOrderSysIDType));
	///������־
	body.ActionFlag = THOST_FTDC_AF_Delete;
	///��Լ����
	strncpy(body.InstrumentID, pOrder->InstrumentID,sizeof(TThostFtdcInstrumentIDType));

	int nRet = m_pApi->ReqOrderAction(&pRequest->InputOrderActionField, ++m_lRequestID);
	delete pRequest;
	return nRet;
}

void CTraderApi::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	OrderIDType orderId = { 0 };
	sprintf(orderId, "%d:%d:%s", pInputOrderAction->FrontID, pInputOrderAction->SessionID, pInputOrderAction->OrderRef);

	unordered_map<string, OrderField*>::iterator it = m_id_platform_order.find(orderId);
	if (it == m_id_platform_order.end())
	{
		// û�ҵ�����Ӧ�������ʾ������
		assert(false);
	}
	else
	{
		// �ҵ��ˣ�Ҫ����״̬
		// ��ʹ���ϴε�״̬
		OrderField* pField = it->second;
		strcpy(pField->ID, orderId);
		pField->ExecType = ExecType::ExecCancelReject;
		pField->ErrorID = pRspInfo->ErrorID;
		strncpy(pField->Text, pRspInfo->ErrorMsg, sizeof(TThostFtdcErrorMsgType));
		XRespone(ResponeType::OnRtnOrder, m_msgQueue, this, 0, 0, pField, sizeof(OrderField), nullptr, 0, nullptr, 0);
	}
}

void CTraderApi::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo)
{
	OrderIDType orderId = { 0 };
	sprintf(orderId, "%d:%d:%s", pOrderAction->FrontID, pOrderAction->SessionID, pOrderAction->OrderRef);

	unordered_map<string, OrderField*>::iterator it = m_id_platform_order.find(orderId);
	if (it == m_id_platform_order.end())
	{
		// û�ҵ�����Ӧ�������ʾ������
		assert(false);
	}
	else
	{
		// �ҵ��ˣ�Ҫ����״̬
		// ��ʹ���ϴε�״̬
		OrderField* pField = it->second;
		strcpy(pField->ID, orderId);
		pField->ExecType = ExecType::ExecCancelReject;
		pField->ErrorID = pRspInfo->ErrorID;
		strncpy(pField->Text, pRspInfo->ErrorMsg, sizeof(TThostFtdcErrorMsgType));
		XRespone(ResponeType::OnRtnOrder, m_msgQueue, this, 0, 0, pField, sizeof(OrderField), nullptr, 0, nullptr, 0);
	}
}

void CTraderApi::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
	OnOrder(pOrder);
}

char* CTraderApi::ReqQuoteInsert(
	int QuoteRef,
	QuoteField* pQuote)
{
	if (nullptr == m_pApi)
		return nullptr;

	SRequest* pRequest = MakeRequestBuf(E_InputQuoteField);
	if (nullptr == pRequest)
		return nullptr;

	CThostFtdcInputQuoteField& body = pRequest->InputQuoteField;

	strcpy(body.BrokerID, m_RspUserLogin.BrokerID);
	strcpy(body.InvestorID, m_RspUserLogin.UserID);

	//��Լ,Ŀǰֻ�Ӷ���1��ȡ
	strncpy(body.InstrumentID, pQuote->InstrumentID, sizeof(TThostFtdcInstrumentIDType));
	//��ƽ
	body.AskOffsetFlag = OpenCloseType_2_TThostFtdcOffsetFlagType(pQuote->AskOpenClose);
	body.BidOffsetFlag = OpenCloseType_2_TThostFtdcOffsetFlagType(pQuote->BidOpenClose);
	//Ͷ��
	body.AskHedgeFlag = HedgeFlagType_2_TThostFtdcHedgeFlagType(pQuote->AskHedgeFlag);
	body.BidHedgeFlag = HedgeFlagType_2_TThostFtdcHedgeFlagType(pQuote->BidHedgeFlag);

	//�۸�
	body.AskPrice = pQuote->AskPrice;
	body.BidPrice = pQuote->BidPrice;

	//����
	body.AskVolume = (int)pQuote->AskQty;
	body.BidVolume = (int)pQuote->BidQty;

	int nRet = 0;
	{
		//���ܱ���̫�죬m_nMaxOrderRef��û�иı���ύ��
		lock_guard<mutex> cl(m_csOrderRef);

		if (QuoteRef < 0)
		{
			nRet = m_nMaxOrderRef;
			sprintf(body.QuoteRef, "%d", m_nMaxOrderRef);
			sprintf(body.AskOrderRef, "%d", m_nMaxOrderRef);
			sprintf(body.BidOrderRef, "%d", ++m_nMaxOrderRef);
			++m_nMaxOrderRef;
		}
		else
		{
			nRet = QuoteRef;
			sprintf(body.QuoteRef, "%d", QuoteRef);
			sprintf(body.AskOrderRef, "%d", QuoteRef);
			sprintf(body.BidOrderRef, "%d", ++QuoteRef);
			++QuoteRef;
		}

		//�����浽���У�����ֱ�ӷ���
		int n = m_pApi->ReqQuoteInsert(&pRequest->InputQuoteField, ++m_lRequestID);
		if (n < 0)
		{
			nRet = n;
			delete pRequest;
			return nullptr;
		}
		else
		{
			sprintf(m_orderInsert_Id, "%d:%d:%d", m_RspUserLogin.FrontID, m_RspUserLogin.SessionID, nRet);

			QuoteField* pField = new QuoteField();
			memcpy(pField, pQuote, sizeof(QuoteField));
			strcpy(pField->ID, m_orderInsert_Id);
			strcpy(pField->AskID, m_orderInsert_Id);
			//strcpy(pField->BidID, m_orderInsert_Id);
			sprintf(pField->BidID, "%d:%d:%d", m_RspUserLogin.FrontID, m_RspUserLogin.SessionID, nRet + 1);
			
			m_id_platform_quote.insert(pair<string, QuoteField*>(m_orderInsert_Id, pField));
		}
	}
	delete pRequest;//�����ֱ��ɾ��

	return m_orderInsert_Id;
}

void CTraderApi::OnRspQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	OrderIDType quoteId = { 0 };
	sprintf(quoteId, "%d:%d:%s", m_RspUserLogin.FrontID, m_RspUserLogin.SessionID, pInputQuote->QuoteRef);

	unordered_map<string, QuoteField*>::iterator it = m_id_platform_quote.find(quoteId);
	if (it == m_id_platform_quote.end())
	{
		// û�ҵ�����Ӧ�������ʾ������
		assert(false);
	}
	else
	{
		// �ҵ��ˣ�Ҫ����״̬
		// ��ʹ���ϴε�״̬
		QuoteField* pField = it->second;
		pField->ExecType = ExecType::ExecRejected;
		pField->Status = OrderStatus::Rejected;
		pField->ErrorID = pRspInfo->ErrorID;
		strncpy(pField->Text, pRspInfo->ErrorMsg, sizeof(TThostFtdcErrorMsgType));
		XRespone(ResponeType::OnRtnQuote, m_msgQueue, this, 0, 0, pField, sizeof(QuoteField), nullptr, 0, nullptr, 0);
	}
}

void CTraderApi::OnErrRtnQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo)
{
	OrderIDType quoteId = { 0 };
	sprintf(quoteId, "%d:%d:%s", m_RspUserLogin.FrontID, m_RspUserLogin.SessionID, pInputQuote->QuoteRef);

	unordered_map<string, QuoteField*>::iterator it = m_id_platform_quote.find(quoteId);
	if (it == m_id_platform_quote.end())
	{
		// û�ҵ�����Ӧ�������ʾ������
		assert(false);
	}
	else
	{
		// �ҵ��ˣ�Ҫ����״̬
		// ��ʹ���ϴε�״̬
		QuoteField* pField = it->second;
		pField->ExecType = ExecType::ExecRejected;
		pField->Status = OrderStatus::Rejected;
		pField->ErrorID = pRspInfo->ErrorID;
		strncpy(pField->Text, pRspInfo->ErrorMsg, sizeof(TThostFtdcErrorMsgType));
		XRespone(ResponeType::OnRtnQuote, m_msgQueue, this, 0, 0, pField, sizeof(QuoteField), nullptr, 0, nullptr, 0);
	}
}

void CTraderApi::OnRtnQuote(CThostFtdcQuoteField *pQuote)
{
	OnQuote(pQuote);
}

int CTraderApi::ReqQuoteAction(const string& szId)
{
	unordered_map<string, CThostFtdcQuoteField*>::iterator it = m_id_api_quote.find(szId);
	if (it == m_id_api_quote.end())
	{
		//// <error id="QUOTE_NOT_FOUND" value="86" prompt="CTP:���۳����Ҳ�����Ӧ����"/>
		return -100;
	}
	else
	{
		// �ҵ��˶���
		ReqQuoteAction(it->second);
	}
	return 0;
}

int CTraderApi::ReqQuoteAction(CThostFtdcQuoteField *pQuote)
{
	if (nullptr == m_pApi)
		return 0;

	SRequest* pRequest = MakeRequestBuf(E_InputQuoteActionField);
	if (nullptr == pRequest)
		return 0;

	CThostFtdcInputQuoteActionField& body = pRequest->InputQuoteActionField;

	///���͹�˾����
	strcpy(body.BrokerID, pQuote->BrokerID);
	///Ͷ���ߴ���
	strcpy(body.InvestorID, pQuote->InvestorID);
	///��������
	strcpy(body.QuoteRef, pQuote->QuoteRef);
	///ǰ�ñ��
	body.FrontID = pQuote->FrontID;
	///�Ự���
	body.SessionID = pQuote->SessionID;
	///����������
	strcpy(body.ExchangeID, pQuote->ExchangeID);
	///�������
	strcpy(body.QuoteSysID, pQuote->QuoteSysID);
	///������־
	body.ActionFlag = THOST_FTDC_AF_Delete;
	///��Լ����
	strcpy(body.InstrumentID, pQuote->InstrumentID);

	int nRet = m_pApi->ReqQuoteAction(&pRequest->InputQuoteActionField, ++m_lRequestID);
	delete pRequest;
	return nRet;
}

void CTraderApi::OnRspQuoteAction(CThostFtdcInputQuoteActionField *pInputQuoteAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	OrderIDType quoteId = { 0 };
	sprintf(quoteId, "%d:%d:%s", pInputQuoteAction->FrontID, pInputQuoteAction->SessionID, pInputQuoteAction->QuoteRef);

	unordered_map<string, QuoteField*>::iterator it = m_id_platform_quote.find(quoteId);
	if (it == m_id_platform_quote.end())
	{
		// û�ҵ�����Ӧ�������ʾ������
		assert(false);
	}
	else
	{
		// �ҵ��ˣ�Ҫ����״̬
		// ��ʹ���ϴε�״̬
		QuoteField* pField = it->second;
		strcpy(pField->ID, quoteId);
		//sprintf(pField->AskID, "%d:%d:%s", pInputQuoteAction->FrontID, pInputQuoteAction->SessionID, pInputQuoteAction->);
		//sprintf(pField->BidID, "%d:%d:%s", pInputQuoteAction->FrontID, pInputQuoteAction->SessionID, pInputQuoteAction->QuoteRef);
		pField->ExecType = ExecType::ExecCancelReject;
		pField->ErrorID = pRspInfo->ErrorID;
		strncpy(pField->Text, pRspInfo->ErrorMsg, sizeof(TThostFtdcErrorMsgType));
		XRespone(ResponeType::OnRtnQuote, m_msgQueue, this, 0, 0, pField, sizeof(QuoteField), nullptr, 0, nullptr, 0);
	}
}

void CTraderApi::OnErrRtnQuoteAction(CThostFtdcQuoteActionField *pQuoteAction, CThostFtdcRspInfoField *pRspInfo)
{
	OrderIDType quoteId = { 0 };
	sprintf(quoteId, "%d:%d:%s", pQuoteAction->FrontID, pQuoteAction->SessionID, pQuoteAction->QuoteRef);

	unordered_map<string, QuoteField*>::iterator it = m_id_platform_quote.find(quoteId);
	if (it == m_id_platform_quote.end())
	{
		// û�ҵ�����Ӧ�������ʾ������
		assert(false);
	}
	else
	{
		// �ҵ��ˣ�Ҫ����״̬
		// ��ʹ���ϴε�״̬
		QuoteField* pField = it->second;
		strcpy(pField->ID, quoteId);
		pField->ExecType = ExecType::ExecCancelReject;
		pField->ErrorID = pRspInfo->ErrorID;
		strncpy(pField->Text, pRspInfo->ErrorMsg, sizeof(TThostFtdcErrorMsgType));
		XRespone(ResponeType::OnRtnQuote, m_msgQueue, this, 0, 0, pField, sizeof(QuoteField), nullptr, 0, nullptr, 0);
	}
}

void CTraderApi::ReqQryTradingAccount()
{
	if (nullptr == m_pApi)
		return;

	SRequest* pRequest = MakeRequestBuf(E_QryTradingAccountField);
	if (nullptr == pRequest)
		return;

	CThostFtdcQryTradingAccountField& body = pRequest->QryTradingAccountField;

	strncpy(body.BrokerID, m_RspUserLogin.BrokerID,sizeof(TThostFtdcBrokerIDType));
	strncpy(body.InvestorID, m_RspUserLogin.UserID,sizeof(TThostFtdcInvestorIDType));

	AddToSendQueue(pRequest);
}

void CTraderApi::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo, nRequestID, bIsLast))
	{
		if (pTradingAccount)
		{
			AccountField field = { 0 };
			field.PreBalance = pTradingAccount->PreBalance;
			field.CurrMargin = pTradingAccount->CurrMargin;
			field.Commission = pTradingAccount->Commission;
			field.CloseProfit = pTradingAccount->CloseProfit;
			field.PositionProfit = pTradingAccount->PositionProfit;
			field.Balance = pTradingAccount->Balance;
			field.Available = pTradingAccount->Available;

			XRespone(ResponeType::OnRspQryTradingAccount, m_msgQueue, this, bIsLast, 0, &field, sizeof(AccountField), nullptr, 0, nullptr, 0);
		}
		else
		{
			XRespone(ResponeType::OnRspQryTradingAccount, m_msgQueue, this, bIsLast, 0, nullptr, 0, nullptr, 0, nullptr, 0);
		}
	}

	if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);
}

void CTraderApi::ReqQryInvestorPosition(const string& szInstrumentId, const string& szExchange)
{
	if (nullptr == m_pApi)
		return;

	SRequest* pRequest = MakeRequestBuf(E_QryInvestorPositionField);
	if (nullptr == pRequest)
		return;

	CThostFtdcQryInvestorPositionField& body = pRequest->QryInvestorPositionField;

	strncpy(body.BrokerID, m_RspUserLogin.BrokerID,sizeof(TThostFtdcBrokerIDType));
	strncpy(body.InvestorID, m_RspUserLogin.UserID,sizeof(TThostFtdcInvestorIDType));
	strncpy(body.InstrumentID,szInstrumentId.c_str(),sizeof(TThostFtdcInstrumentIDType));

	AddToSendQueue(pRequest);
}

// ����������ѯ���ͽ�����ȫ������
// ����Ǻ��ڵĳɽ��ر�����ֻ���ظ��µļ�¼
// �����н�����ͬʱ�н�������ĳֲ�ʱ��ֻ���ؽ���������������
// ������������Ŀǰû�����⣬������Ҳֻ������
void CTraderApi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo, nRequestID, bIsLast))
	{
		if (pInvestorPosition)
		{
			PositionIDType positionId = { 0 };
			sprintf(positionId, "%s:%d:%c",
				pInvestorPosition->InstrumentID, TThostFtdcPosiDirectionType_2_PositionSide(pInvestorPosition->PosiDirection), pInvestorPosition->HedgeFlag);

			PositionField* pField = nullptr;
			unordered_map<string, PositionField*>::iterator it = m_id_platform_position.find(positionId);
			if (it == m_id_platform_position.end())
			{
				pField = new PositionField();
				memset(pField, 0, sizeof(PositionField));
				
				strcpy(pField->Symbol, pInvestorPosition->InstrumentID);
				strcpy(pField->InstrumentID, pInvestorPosition->InstrumentID);
				//strcpy(pField->ExchangeID, );
				pField->Side = TThostFtdcPosiDirectionType_2_PositionSide(pInvestorPosition->PosiDirection);
				pField->HedgeFlag = TThostFtdcHedgeFlagType_2_HedgeFlagType(pInvestorPosition->HedgeFlag);

				m_id_platform_position.insert(pair<string, PositionField*>(positionId, pField));
			}
			else
			{
				pField = it->second;
			}
						
			pField->Position = pInvestorPosition->Position;
			pField->TdPosition = pInvestorPosition->TodayPosition;
			pField->YdPosition = pInvestorPosition->Position - pInvestorPosition->TodayPosition;

			// �������ռ�ȫ���ٱ���֪ͨһ�Σ�Ϊ��Ҫ����������Ϊ������������¼�����Ҽ���һ������
			if (bIsLast)
			{
				int cnt = 0;
				size_t count = m_id_platform_position.size();
				for (unordered_map<string, PositionField*>::iterator iter = m_id_platform_position.begin(); iter != m_id_platform_position.end(); iter++)
				{
					++cnt;
					XRespone(ResponeType::OnRspQryInvestorPosition, m_msgQueue, this, cnt == count, 0, iter->second, sizeof(PositionField), nullptr, 0, nullptr, 0);
				}
			}
			//XRespone(ResponeType::OnRspQryInvestorPosition, m_msgQueue, this, bIsLast, 0, pField, sizeof(PositionField), nullptr, 0, nullptr, 0);
		}
		else
		{
			XRespone(ResponeType::OnRspQryInvestorPosition, m_msgQueue, this, bIsLast, 0, nullptr, 0, nullptr, 0, nullptr, 0);
		}
	}

	if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);
}

void CTraderApi::ReqQryInvestorPositionDetail(const string& szInstrumentId)
{
	if (nullptr == m_pApi)
		return;

	SRequest* pRequest = MakeRequestBuf(E_QryInvestorPositionDetailField);
	if (nullptr == pRequest)
		return;

	CThostFtdcQryInvestorPositionDetailField& body = pRequest->QryInvestorPositionDetailField;

	strncpy(body.BrokerID, m_RspUserLogin.BrokerID,sizeof(TThostFtdcBrokerIDType));
	strncpy(body.InvestorID, m_RspUserLogin.UserID,sizeof(TThostFtdcInvestorIDType));
	strncpy(body.InstrumentID,szInstrumentId.c_str(),sizeof(TThostFtdcInstrumentIDType));

	AddToSendQueue(pRequest);
}

void CTraderApi::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	
}

void CTraderApi::ReqQryInstrument(const string& szInstrumentId, const string& szExchange)
{
	if (nullptr == m_pApi)
		return;

	SRequest* pRequest = MakeRequestBuf(E_QryInstrumentField);
	if (nullptr == pRequest)
		return;

	CThostFtdcQryInstrumentField& body = pRequest->QryInstrumentField;

	strncpy(body.InstrumentID,szInstrumentId.c_str(),sizeof(TThostFtdcInstrumentIDType));
	strncpy(body.ExchangeID, szExchange.c_str(), sizeof(TThostFtdcExchangeIDType));

	AddToSendQueue(pRequest);
}


void CTraderApi::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo, nRequestID, bIsLast))
	{
		if (pInstrument)
		{
			InstrumentField field = { 0 };

			strncpy(field.InstrumentID, pInstrument->InstrumentID, sizeof(TThostFtdcInstrumentIDType));
			strncpy(field.ExchangeID, pInstrument->ExchangeID, sizeof(TThostFtdcExchangeIDType));

			strncpy(field.Symbol, pInstrument->InstrumentID, sizeof(TThostFtdcInstrumentIDType));

			strncpy(field.InstrumentName, pInstrument->InstrumentName, sizeof(TThostFtdcInstrumentNameType));
			field.Type = CThostFtdcInstrumentField_2_InstrumentType(pInstrument);
			field.VolumeMultiple = pInstrument->VolumeMultiple;
			field.PriceTick = pInstrument->PriceTick;
			strncpy(field.ExpireDate, pInstrument->ExpireDate, sizeof(TThostFtdcDateType));
			field.OptionsType = TThostFtdcOptionsTypeType_2_PutCall(pInstrument->OptionsType);

			XRespone(ResponeType::OnRspQryInstrument, m_msgQueue, this, bIsLast, 0, &field, sizeof(InstrumentField), nullptr, 0, nullptr, 0);
		}
		else
		{
			XRespone(ResponeType::OnRspQryInstrument, m_msgQueue, this, bIsLast, 0, nullptr, 0, nullptr, 0, nullptr, 0);
		}
	}

	if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);
}

void CTraderApi::ReqQryInstrumentCommissionRate(const string& szInstrumentId)
{
	if (nullptr == m_pApi)
		return;

	SRequest* pRequest = MakeRequestBuf(E_QryInstrumentCommissionRateField);
	if (nullptr == pRequest)
		return;

	CThostFtdcQryInstrumentCommissionRateField& body = pRequest->QryInstrumentCommissionRateField;

	strncpy(body.BrokerID, m_RspUserLogin.BrokerID,sizeof(TThostFtdcBrokerIDType));
	strncpy(body.InvestorID, m_RspUserLogin.UserID,sizeof(TThostFtdcInvestorIDType));
	strncpy(body.InstrumentID,szInstrumentId.c_str(),sizeof(TThostFtdcInstrumentIDType));

	AddToSendQueue(pRequest);
}

void CTraderApi::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	//if(m_msgQueue)
	//	m_msgQueue->Input_OnRspQryInstrumentCommissionRate(this,pInstrumentCommissionRate,pRspInfo,nRequestID,bIsLast);

	if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);
}

void CTraderApi::ReqQryInstrumentMarginRate(const string& szInstrumentId,TThostFtdcHedgeFlagType HedgeFlag)
{
	if (nullptr == m_pApi)
		return;

	SRequest* pRequest = MakeRequestBuf(E_QryInstrumentMarginRateField);
	if (nullptr == pRequest)
		return;

	CThostFtdcQryInstrumentMarginRateField& body = pRequest->QryInstrumentMarginRateField;

	strncpy(body.BrokerID, m_RspUserLogin.BrokerID,sizeof(TThostFtdcBrokerIDType));
	strncpy(body.InvestorID, m_RspUserLogin.UserID,sizeof(TThostFtdcInvestorIDType));
	strncpy(body.InstrumentID,szInstrumentId.c_str(),sizeof(TThostFtdcInstrumentIDType));
	body.HedgeFlag = HedgeFlag;

	AddToSendQueue(pRequest);
}

void CTraderApi::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	//if(m_msgQueue)
	//	m_msgQueue->Input_OnRspQryInstrumentMarginRate(this,pInstrumentMarginRate,pRspInfo,nRequestID,bIsLast);

	if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);
}

void CTraderApi::ReqQryDepthMarketData(const string& szInstrumentId)
{
	if (nullptr == m_pApi)
		return;

	SRequest* pRequest = MakeRequestBuf(E_QryDepthMarketDataField);
	if (nullptr == pRequest)
		return;

	CThostFtdcQryDepthMarketDataField& body = pRequest->QryDepthMarketDataField;

	strncpy(body.InstrumentID,szInstrumentId.c_str(),sizeof(TThostFtdcInstrumentIDType));

	AddToSendQueue(pRequest);
}

void CTraderApi::OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	//if(m_msgQueue)
	//	m_msgQueue->Input_OnRspQryDepthMarketData(this,pDepthMarketData,pRspInfo,nRequestID,bIsLast);

	if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);
}

void CTraderApi::ReqQrySettlementInfo(const string& szTradingDay)
{
	if (nullptr == m_pApi)
		return;

	SRequest* pRequest = MakeRequestBuf(E_QrySettlementInfoField);
	if (nullptr == pRequest)
		return;

	CThostFtdcQrySettlementInfoField& body = pRequest->QrySettlementInfoField;

	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TThostFtdcBrokerIDType));
	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TThostFtdcInvestorIDType));
	strncpy(body.TradingDay, szTradingDay.c_str(), sizeof(TThostFtdcDateType));

	AddToSendQueue(pRequest);
}

void CTraderApi::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo, nRequestID, bIsLast))
	{
		if (pSettlementInfo)
		{
			SettlementInfoField field = { 0 };
			strncpy(field.TradingDay, pSettlementInfo->TradingDay, sizeof(TThostFtdcDateType));
			strncpy(field.Content, pSettlementInfo->Content, sizeof(TThostFtdcContentType));

			XRespone(ResponeType::OnRspQrySettlementInfo, m_msgQueue, this, bIsLast, 0, &field, sizeof(SettlementInfoField), nullptr, 0, nullptr, 0);
		}
		else
		{
			XRespone(ResponeType::OnRspQrySettlementInfo, m_msgQueue, this, bIsLast, 0, nullptr, 0, nullptr, 0, nullptr, 0);
		}
	}

	if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);
}

void CTraderApi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	IsErrorRspInfo(pRspInfo, nRequestID, bIsLast);

	if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);
}

void CTraderApi::ReqQryOrder()
{
	if (nullptr == m_pApi)
		return;

	SRequest* pRequest = MakeRequestBuf(E_QryOrderField);
	if (nullptr == pRequest)
		return;

	CThostFtdcQryOrderField& body = pRequest->QryOrderField;

	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TThostFtdcBrokerIDType));
	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TThostFtdcInvestorIDType));

	AddToSendQueue(pRequest);
}

void CTraderApi::OnOrder(CThostFtdcOrderField *pOrder)
{
	if (nullptr == pOrder)
		return;

	OrderIDType orderId = { 0 };
	sprintf(orderId, "%d:%d:%s", pOrder->FrontID, pOrder->SessionID, pOrder->OrderRef);
	OrderIDType orderSydId = { 0 };

	{
		// ����ԭʼ������Ϣ�����ڳ���

		unordered_map<string, CThostFtdcOrderField*>::iterator it = m_id_api_order.find(orderId);
		if (it == m_id_api_order.end())
		{
			// �Ҳ����˶�������ʾ���µ�
			CThostFtdcOrderField* pField = new CThostFtdcOrderField();
			memcpy(pField, pOrder, sizeof(CThostFtdcOrderField));
			m_id_api_order.insert(pair<string, CThostFtdcOrderField*>(orderId, pField));
		}
		else
		{
			// �ҵ��˶���
			// ��Ҫ�ٸ��Ʊ������һ�ε�״̬������ֻҪ��һ�ε����ڳ������ɣ����£��������ñȽ�
			CThostFtdcOrderField* pField = it->second;
			memcpy(pField, pOrder, sizeof(CThostFtdcOrderField));
		}

		// ����SysID���ڶ���ɽ��ر��붩��
		sprintf(orderSydId, "%s:%s", pOrder->ExchangeID, pOrder->OrderSysID);
		m_sysId_orderId.insert(pair<string, string>(orderSydId, orderId));
	}

	{
		// ��API�Ķ���ת�����Լ��Ľṹ��

		OrderField* pField = nullptr;
		unordered_map<string, OrderField*>::iterator it = m_id_platform_order.find(orderId);
		if (it == m_id_platform_order.end())
		{
			// ����ʱ������Ϣ��û�У������Ҳ�����Ӧ�ĵ��ӣ���Ҫ����Order�Ļָ�
			pField = new OrderField();
			memset(pField, 0, sizeof(OrderField));
			strcpy(pField->ID, orderId);
			strcpy(pField->InstrumentID, pOrder->InstrumentID);
			strcpy(pField->ExchangeID, pOrder->ExchangeID);
			pField->HedgeFlag = TThostFtdcHedgeFlagType_2_HedgeFlagType(pOrder->CombHedgeFlag[0]);
			pField->Side = TThostFtdcDirectionType_2_OrderSide(pOrder->Direction);
			pField->Price = pOrder->LimitPrice;
			pField->StopPx = pOrder->StopPrice;
			strncpy(pField->Text, pOrder->StatusMsg, sizeof(TThostFtdcErrorMsgType));
			pField->OpenClose = TThostFtdcOffsetFlagType_2_OpenCloseType(pOrder->CombOffsetFlag[0]);
			pField->Status = CThostFtdcOrderField_2_OrderStatus(pOrder);
			pField->Qty = pOrder->VolumeTotalOriginal;
			pField->Type = CThostFtdcOrderField_2_OrderType(pOrder);
			pField->TimeInForce = CThostFtdcOrderField_2_TimeInForce(pOrder);
			pField->ExecType = ExecType::ExecNew;
			strcpy(pField->OrderID, pOrder->OrderSysID);


			// ��ӵ�map�У������������ߵĶ�ȡ������ʧ��ʱ����֪ͨ��
			m_id_platform_order.insert(pair<string, OrderField*>(orderId, pField));
		}
		else
		{
			pField = it->second;
			strcpy(pField->ID, orderId);
			pField->LeavesQty = pOrder->VolumeTotal;
			pField->Price = pOrder->LimitPrice;
			pField->Status = CThostFtdcOrderField_2_OrderStatus(pOrder);
			pField->ExecType = CThostFtdcOrderField_2_ExecType(pOrder);
			strcpy(pField->OrderID, pOrder->OrderSysID);
			strncpy(pField->Text, pOrder->StatusMsg, sizeof(TThostFtdcErrorMsgType));
		}

		XRespone(ResponeType::OnRtnOrder, m_msgQueue, this, 0, 0, pField, sizeof(OrderField), nullptr, 0, nullptr, 0);
	}
}

void CTraderApi::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo, nRequestID, bIsLast))
	{
		OnOrder(pOrder);
	}

	if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);
}

void CTraderApi::ReqQryTrade()
{
	if (nullptr == m_pApi)
		return;

	SRequest* pRequest = MakeRequestBuf(E_QryTradeField);
	if (nullptr == pRequest)
		return;

	CThostFtdcQryTradeField& body = pRequest->QryTradeField;

	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TThostFtdcBrokerIDType));
	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TThostFtdcInvestorIDType));

	AddToSendQueue(pRequest);
}

void CTraderApi::OnTrade(CThostFtdcTradeField *pTrade)
{
	if (nullptr == pTrade)
		return;

	TradeField* pField = new TradeField();
	strcpy(pField->InstrumentID, pTrade->InstrumentID);
	strcpy(pField->ExchangeID, pTrade->ExchangeID);
	pField->Side = TThostFtdcDirectionType_2_OrderSide(pTrade->Direction);
	pField->Qty = pTrade->Volume;
	pField->Price = pTrade->Price;
	pField->OpenClose = TThostFtdcOffsetFlagType_2_OpenCloseType(pTrade->OffsetFlag);
	pField->HedgeFlag = TThostFtdcHedgeFlagType_2_HedgeFlagType(pTrade->HedgeFlag);
	pField->Commission = 0;//TODO�������Ժ�Ҫ�������
	strcpy(pField->Time, pTrade->TradeTime);
	strcpy(pField->TradeID, pTrade->TradeID);

	OrderIDType orderSysId = { 0 };
	sprintf(orderSysId, "%s:%s", pTrade->ExchangeID, pTrade->OrderSysID);
	unordered_map<string, string>::iterator it = m_sysId_orderId.find(orderSysId);
	if (it == m_sysId_orderId.end())
	{
		// �˳ɽ��Ҳ�����Ӧ�ı���
		assert(false);
	}
	else
	{
		// �ҵ���Ӧ�ı���
		strcpy(pField->ID, it->second.c_str());

		XRespone(ResponeType::OnRtnTrade, m_msgQueue, this, 0, 0, pField, sizeof(TradeField), nullptr, 0, nullptr, 0);

		unordered_map<string, OrderField*>::iterator it2 = m_id_platform_order.find(it->second);
		if (it2 == m_id_platform_order.end())
		{
			// �˳ɽ��Ҳ�����Ӧ�ı���
			assert(false);
		}
		else
		{
			// ���¶�����״̬
			// �Ƿ�Ҫ֪ͨ�ӿ�
		}

		OnTrade(pField);
	}
}

void CTraderApi::OnTrade(TradeField *pTrade)
{
	PositionIDType positionId = { 0 };
	sprintf(positionId, "%s:%d:%c",
		pTrade->InstrumentID, TradeField_2_PositionSide(pTrade), pTrade->HedgeFlag);

	PositionField* pField = nullptr;
	unordered_map<string, PositionField*>::iterator it = m_id_platform_position.find(positionId);
	if (it == m_id_platform_position.end())
	{
		pField = new PositionField();
		memset(pField, 0, sizeof(PositionField));

		strcpy(pField->Symbol, pTrade->InstrumentID);
		strcpy(pField->InstrumentID, pTrade->InstrumentID);
		pField->Side = TradeField_2_PositionSide(pTrade);
		pField->HedgeFlag = TThostFtdcHedgeFlagType_2_HedgeFlagType(pTrade->HedgeFlag);

		m_id_platform_position.insert(pair<string, PositionField*>(positionId, pField));
	}
	else
	{
		pField = it->second;
	}

	if (pTrade->OpenClose == OpenCloseType::Open)
	{
		pField->Position += pTrade->Qty;
		pField->TdPosition += pTrade->Qty;
	}
	else
	{
		pField->Position -= pTrade->Qty;
		if (pTrade->OpenClose == OpenCloseType::CloseToday)
		{
			pField->TdPosition -= pTrade->Qty;
		}
		else
		{
			pField->YdPosition -= pTrade->Qty;
			// �������ı����ɸ������ӽ��쿪ʼ������
			if (pField->YdPosition<0)
			{
				pField->TdPosition += pField->YdPosition;
				pField->YdPosition = 0;
			}
		}
		
		// �������ֱ�����²�ѯ
		if (pField->Position < 0 || pField->TdPosition < 0 || pField->YdPosition < 0)
		{
			ReqQryInvestorPosition("", "");
			return;
		}
	}

	XRespone(ResponeType::OnRspQryInvestorPosition, m_msgQueue, this, false, 0, pField, sizeof(PositionField), nullptr, 0, nullptr, 0);
}

void CTraderApi::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo, nRequestID, bIsLast))
	{
		OnTrade(pTrade);
	}

	if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);
}

void CTraderApi::ReqQryQuote()
{
	if (nullptr == m_pApi)
		return;

	SRequest* pRequest = MakeRequestBuf(E_QryQuoteField);
	if (nullptr == pRequest)
		return;

	CThostFtdcQryQuoteField& body = pRequest->QryQuoteField;

	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TThostFtdcBrokerIDType));
	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TThostFtdcInvestorIDType));

	AddToSendQueue(pRequest);
}

void CTraderApi::OnQuote(CThostFtdcQuoteField *pQuote)
{
	if (nullptr == pQuote)
		return;

	OrderIDType quoteId = { 0 };
	sprintf(quoteId, "%d:%d:%s", pQuote->FrontID, pQuote->SessionID, pQuote->QuoteRef);
	OrderIDType orderSydId = { 0 };

	{
		// ����ԭʼ������Ϣ�����ڳ���

		unordered_map<string, CThostFtdcQuoteField*>::iterator it = m_id_api_quote.find(quoteId);
		if (it == m_id_api_quote.end())
		{
			// �Ҳ����˶�������ʾ���µ�
			CThostFtdcQuoteField* pField = new CThostFtdcQuoteField();
			memcpy(pField, pQuote, sizeof(CThostFtdcQuoteField));
			m_id_api_quote.insert(pair<string, CThostFtdcQuoteField*>(quoteId, pField));
		}
		else
		{
			// �ҵ��˶���
			// ��Ҫ�ٸ��Ʊ������һ�ε�״̬������ֻҪ��һ�ε����ڳ������ɣ����£��������ñȽ�
			CThostFtdcQuoteField* pField = it->second;
			memcpy(pField, pQuote, sizeof(CThostFtdcQuoteField));
		}

		// ����ط��Ƿ�Ҫ������������

		// ����SysID���ڶ���ɽ��ر��붩��
		//sprintf(orderSydId, "%s:%s", pQuote->ExchangeID, pQuote->QuoteSysID);
		//m_sysId_quoteId.insert(pair<string, string>(orderSydId, quoteId));
	}

	{
		// ��API�Ķ���ת�����Լ��Ľṹ��

		QuoteField* pField = nullptr;
		unordered_map<string, QuoteField*>::iterator it = m_id_platform_quote.find(quoteId);
		if (it == m_id_platform_quote.end())
		{
			// ����ʱ������Ϣ��û�У������Ҳ�����Ӧ�ĵ��ӣ���Ҫ����Order�Ļָ�
			pField = new QuoteField();
			memset(pField, 0, sizeof(QuoteField));
			strcpy(pField->InstrumentID, pQuote->InstrumentID);
			strcpy(pField->ExchangeID, pQuote->ExchangeID);

			pField->AskQty = pQuote->AskVolume;
			pField->AskPrice = pQuote->AskPrice;
			pField->AskOpenClose = TThostFtdcOffsetFlagType_2_OpenCloseType(pQuote->AskOffsetFlag);
			pField->AskHedgeFlag = TThostFtdcHedgeFlagType_2_HedgeFlagType(pQuote->AskHedgeFlag);

			pField->BidQty = pQuote->BidVolume;
			pField->BidPrice = pQuote->BidPrice;
			pField->BidOpenClose = TThostFtdcOffsetFlagType_2_OpenCloseType(pQuote->BidOffsetFlag);
			pField->BidHedgeFlag = TThostFtdcHedgeFlagType_2_HedgeFlagType(pQuote->BidHedgeFlag);

			strcpy(pField->ID, quoteId);
			strcpy(pField->AskOrderID, pQuote->AskOrderSysID);
			strcpy(pField->BidOrderID, pQuote->BidOrderSysID);

			strncpy(pField->Text, pQuote->StatusMsg, sizeof(TThostFtdcErrorMsgType));
			
			//pField->ExecType = ExecType::ExecNew;
			pField->Status = CThostFtdcQuoteField_2_OrderStatus(pQuote);
			pField->ExecType = ExecType::ExecNew;


			// ��ӵ�map�У������������ߵĶ�ȡ������ʧ��ʱ����֪ͨ��
			m_id_platform_quote.insert(pair<string, QuoteField*>(quoteId, pField));
		}
		else
		{
			pField = it->second;

			strcpy(pField->ID, quoteId);
			strcpy(pField->AskOrderID, pQuote->AskOrderSysID);
			strcpy(pField->BidOrderID, pQuote->BidOrderSysID);

			pField->Status = CThostFtdcQuoteField_2_OrderStatus(pQuote);
			pField->ExecType = CThostFtdcQuoteField_2_ExecType(pQuote);
			
			strncpy(pField->Text, pQuote->StatusMsg, sizeof(TThostFtdcErrorMsgType));
		}

		XRespone(ResponeType::OnRtnQuote, m_msgQueue, this, 0, 0, pField, sizeof(QuoteField), nullptr, 0, nullptr, 0);
	}
}

void CTraderApi::OnRspQryQuote(CThostFtdcQuoteField *pQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo, nRequestID, bIsLast))
	{
		OnQuote(pQuote);
	}

	if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);
}

void CTraderApi::OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus)
{
	//if(m_msgQueue)
	//	m_msgQueue->Input_OnRtnInstrumentStatus(this,pInstrumentStatus);
}
