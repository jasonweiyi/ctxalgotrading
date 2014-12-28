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
	if (nullptr == m_hThread)
	{
		m_bRunning = true;
		m_hThread = new thread(ProcessThread, this);
	}
}

void CTraderApi::StopThread()
{
	m_bRunning = false;
	if (m_hThread)
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

bool CTraderApi::IsErrorRspInfo_Output(struct DFITCErrorRtnField *pRspInfo)
{
	bool bRet = ((pRspInfo) && (pRspInfo->nErrorID != 0));
	if (bRet)
	{
		ErrorField field = { 0 };
		field.ErrorID = pRspInfo->nErrorID;
		strncpy(field.ErrorMsg, pRspInfo->errorMsg, sizeof(pRspInfo->errorMsg));

		XRespone(ResponeType::OnRtnError, m_msgQueue, this, true, 0, &field, sizeof(ErrorField), nullptr, 0, nullptr, 0);
	}
	return bRet;
}

bool CTraderApi::IsErrorRspInfo(struct DFITCErrorRtnField *pRspInfo)
{
	bool bRet = ((pRspInfo) && (pRspInfo->nErrorID != 0));

	return bRet;
}

void CTraderApi::Connect(const string& szPath,
	ServerInfoField* pServerInfo,
	UserInfoField* pUserInfo)
{
	m_szPath = szPath;
	memcpy(&m_ServerInfo, pServerInfo, sizeof(ServerInfoField));
	memcpy(&m_UserInfo, pUserInfo, sizeof(UserInfoField));

	m_pApi = DFITCTraderApi::CreateDFITCTraderApi();
	XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Initialized, 0, nullptr, 0, nullptr, 0, nullptr, 0);

	if (m_pApi)
	{
		// ֹͣ�����̣߳�������
		StopThread();
		ReleaseRequestListBuf();
		ReleaseRequestMapBuf();

		SRequest* pRequest = MakeRequestBuf(E_Init);
		if (pRequest)
		{
			AddToSendQueue(pRequest);
		}
	}
}

void CTraderApi::Disconnect()
{
	// �������������������ں����ֽ�����Release,�ֻع�ͷ�����ͣ����ܵ��µ���
	StopThread();

	if (m_pApi)
	{
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

	memset(pRequest, 0, sizeof(SRequest));
	pRequest->type = type;
	switch (type)
	{
	case E_AbiInstrumentField:
		pRequest->pBuf = new DFITCAbiInstrumentField();
		break;
	case E_CancelOrderField:
		pRequest->pBuf = new DFITCCancelOrderField();
		break;
	case E_CapitalField:
		pRequest->pBuf = new DFITCCapitalField();
		break;
	case E_ExchangeInstrumentField:
		pRequest->pBuf = new DFITCExchangeInstrumentField();
		break;
	case E_Init:
		pRequest->pBuf = nullptr;
		break;
	case E_InsertOrderField:
		pRequest->pBuf = new DFITCInsertOrderField();
		break;
	case E_MatchField:
		pRequest->pBuf = new DFITCMatchField();
		break;
	case E_OrderField:
		pRequest->pBuf = new DFITCOrderField();
		break;
	case E_PositionField:
		pRequest->pBuf = new DFITCPositionField();
		break;
	case E_PositionDetailField:
		pRequest->pBuf = new DFITCPositionDetailField();
		break;
	case E_SpecificInstrumentField:
		pRequest->pBuf = new DFITCSpecificInstrumentField();
		break;
	case E_UserLoginField:
		pRequest->pBuf = new DFITCUserLoginField();
		break;
	case E_QuoteSubscribeField:
		pRequest->pBuf = new DFITCQuoteSubscribeField();
		break;
	case E_QuoteUnSubscribeField:
		pRequest->pBuf = new DFITCQuoteUnSubscribeField();
		break;
	case E_QuoteInsertField:
		pRequest->pBuf = new DFITCQuoteInsertField();
		break;
	case E_QuoteCancelField:
		pRequest->pBuf = new DFITCCancelOrderField();
		break;
	default:
		assert(false);
		break;
	}
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
	for (map<int, SRequest*>::iterator it = m_reqMap.begin(); it != m_reqMap.end(); ++it)
	{
		delete (*it).second;
	}
	m_reqMap.clear();
}

void CTraderApi::ReleaseRequestMapBuf(int nRequestID)
{
	lock_guard<mutex> cl(m_csMap);
	map<int, SRequest*>::iterator it = m_reqMap.find(nRequestID);
	if (it != m_reqMap.end())
	{
		delete it->second;
		m_reqMap.erase(nRequestID);
	}
}

void CTraderApi::AddRequestMapBuf(int nRequestID, SRequest* pRequest)
{
	if (nullptr == pRequest)
		return;

	lock_guard<mutex> cl(m_csMap);
	map<int, SRequest*>::iterator it = m_reqMap.find(nRequestID);
	if (it != m_reqMap.end())
	{
		SRequest* p = it->second;
		if (pRequest != p)//���ʵ����ָ����ͬһ�ڴ棬���ٲ���
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

	while (!m_reqList.empty() && m_bRunning)
	{
		SRequest * pRequest = m_reqList.front();
		int lRequest = ++m_lRequestID;// ����ط��Ƿ�����ԭ�Ӳ����������أ�
		switch (pRequest->type)
		{
		case E_AbiInstrumentField:
		{
									 DFITCAbiInstrumentField* body = (DFITCAbiInstrumentField*)pRequest->pBuf;
									 body->lRequestID = lRequest;
									 iRet = m_pApi->ReqQryArbitrageInstrument(body);
		}
			break;
		case E_CapitalField:
		{
							   DFITCCapitalField* body = (DFITCCapitalField*)pRequest->pBuf;
							   body->lRequestID = lRequest;
							   iRet = m_pApi->ReqQryCustomerCapital(body);
		}
			break;
		case E_ExchangeInstrumentField:
		{
										  DFITCExchangeInstrumentField* body = (DFITCExchangeInstrumentField*)pRequest->pBuf;
										  body->lRequestID = lRequest;
										  iRet = m_pApi->ReqQryExchangeInstrument(body);
		}
			break;
		case E_Init:
			iRet = ReqInit();
			if (iRet != 0 && m_bRunning)
                this_thread::sleep_for(chrono::milliseconds(1000 * 20));
			break;
		case E_InsertOrderField:
			assert(false);
			break;
		case E_MatchField:
		{
							 DFITCMatchField* body = (DFITCMatchField*)pRequest->pBuf;
							 body->lRequestID = lRequest;
							 iRet = m_pApi->ReqQryMatchInfo(body);
		}
			break;
		case E_OrderField:
		{
							 DFITCOrderField* body = (DFITCOrderField*)pRequest->pBuf;
							 body->lRequestID = lRequest;
							 iRet = m_pApi->ReqQryOrderInfo(body);
		}
			break;
		case E_PositionField:
		{
								DFITCPositionField* body = (DFITCPositionField*)pRequest->pBuf;
								body->lRequestID = lRequest;
								iRet = m_pApi->ReqQryPosition(body);
		}
			break;
		case E_PositionDetailField:
		{
									  DFITCPositionDetailField* body = (DFITCPositionDetailField*)pRequest->pBuf;
									  body->lRequestID = lRequest;
									  iRet = m_pApi->ReqQryPositionDetail(body);
		}
			break;
		case E_SpecificInstrumentField:
		{
										  DFITCSpecificInstrumentField* body = (DFITCSpecificInstrumentField*)pRequest->pBuf;
										  body->lRequestID = lRequest;
										  iRet = m_pApi->ReqQrySpecifyInstrument(body);
		}
			break;
		case E_UserLoginField:
		{
								 DFITCUserLoginField* body = (DFITCUserLoginField*)pRequest->pBuf;
								 body->lRequestID = lRequest;
								 iRet = m_pApi->ReqUserLogin(body);
		}
			break;
		case E_QuoteSubscribeField:
		{
									  DFITCQuoteSubscribeField * body = (DFITCQuoteSubscribeField*)pRequest->pBuf;
									  //body->lRequestID = lRequest;
									  iRet = m_pApi->ReqQuoteSubscribe(body);
		}
			break;
		default:
			assert(false);
			break;
		}

		if (0 == iRet)
		{
			//���سɹ�����ӵ��ѷ��ͳ�
			m_nSleep = 1;
			AddRequestMapBuf(lRequest, pRequest);

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

int CTraderApi::ReqInit()
{
	XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Connecting, 0, nullptr, 0, nullptr, 0, nullptr, 0);
	//��ʼ������
	int iRet = m_pApi->Init(m_ServerInfo.Address, this);
	if (0 == iRet)
	{
	}
	else
	{
		RspUserLoginField field = { 0 };
		field.ErrorID = iRet;
		strcpy(field.ErrorMsg, "���ӳ�ʱ");

		XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Disconnected, 0, &field, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);
	}
	return iRet;
}

void CTraderApi::OnFrontConnected()
{
	XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Connected, 0, nullptr, 0, nullptr, 0, nullptr, 0);

	////���ӳɹ����Զ�������֤���¼
	//if(m_szAuthCode.length()>0
	//	&&m_szUserProductInfo.length()>0)
	//{
	//	//������֤�������֤
	//	ReqAuthenticate();
	//}
	//else
	{
		ReqUserLogin();
	}
}

void CTraderApi::OnFrontDisconnected(int nReason)
{
	RspUserLoginField field = { 0 };
	//����ʧ�ܷ��ص���Ϣ��ƴ�Ӷ��ɣ���Ҫ��Ϊ��ͳһ���
	field.ErrorID = nReason;
	GetOnFrontDisconnectedMsg(nReason, field.ErrorMsg);

	XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Disconnected, 0, &field, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);

}

void CTraderApi::ReqUserLogin()
{
	if (nullptr == m_pApi)
		return;

	DFITCUserLoginField request = {};

	strncpy(request.accountID, m_UserInfo.UserID, sizeof(DFITCAccountIDType));
	strncpy(request.passwd, m_UserInfo.Password, sizeof(DFITCPasswdType));
	request.companyID = atoi(m_ServerInfo.BrokerID);

	//ֻ����һ���õ���m_nRequestID��û�б�Ҫÿ������m_nRequestID����0��ʼ
	m_pApi->ReqUserLogin(&request);

	XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Logining, 0, nullptr, 0, nullptr, 0, nullptr, 0);
}

void CTraderApi::OnRspUserLogin(struct DFITCUserLoginInfoRtnField * pRspUserLogin, struct DFITCErrorRtnField * pRspInfo)
{
	RspUserLoginField field = { 0 };

	if (!IsErrorRspInfo(pRspInfo)
		&& pRspUserLogin)
	{
		//strncpy(field.TradingDay, pRspUserLogin->TradingDay, sizeof(DateType));
		//strncpy(field.LoginTime, pRspUserLogin->LoginTime, sizeof(TimeType));
		sprintf(field.SessionID, "%d", pRspUserLogin->sessionID);

		XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Logined, 0, &field, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);
		XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Done, 0, nullptr, 0, nullptr, 0, nullptr, 0);

		// ���µ�¼��Ϣ�����ܻ��õ�
		memcpy(&m_RspUserLogin, pRspUserLogin, sizeof(DFITCUserLoginInfoRtnField));
		m_nMaxOrderRef = pRspUserLogin->initLocalOrderID;
		// �Լ�����ʱID��1��ʼ�����ܴ�0��ʼ
		m_nMaxOrderRef = max(m_nMaxOrderRef, 1);
		//ReqSettlementInfoConfirm();
	}
	else
	{
		field.ErrorID = pRspInfo->nErrorID;
		strncpy(field.ErrorMsg, pRspInfo->errorMsg, sizeof(pRspInfo->errorMsg));

		XRespone(ResponeType::OnConnectionStatus, m_msgQueue, this, ConnectionStatus::Disconnected, 0, &field, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);
	}

	if (pRspInfo)
		ReleaseRequestMapBuf(pRspInfo->requestID);
}

char* CTraderApi::ReqOrderInsert(
	int OrderRef,
	OrderField* pOrder1,
	OrderField* pOrder2)
{
	if (nullptr == m_pApi)
		return nullptr;

	SRequest* pRequest = MakeRequestBuf(E_InsertOrderField);
	if (nullptr == pRequest)
		return nullptr;

	DFITCInsertOrderField* body = (DFITCInsertOrderField*)pRequest->pBuf;

	strcpy(body->accountID, m_RspUserLogin.accountID);
	// ��Լ
	strncpy(body->instrumentID, pOrder1->InstrumentID, sizeof(DFITCInstrumentIDType));
	// �۸�
	body->insertPrice = pOrder1->Price;
	// ����
	body->orderAmount = (DFITCAmountType)pOrder1->Qty;
	// ����
	body->buySellType = OrderSide_2_DFITCBuySellTypeType(pOrder1->Side);
	// ��ƽ
	body->openCloseType = OpenCloseType_2_DFITCOpenCloseTypeType(pOrder1->OpenClose);
	// Ͷ��
	body->speculator = HedgeFlagType_2_DFITCSpeculatorType(pOrder1->HedgeFlag);
	body->insertType = DFITC_BASIC_ORDER;
	body->orderType = OrderType_2_DFITCOrderTypeType(pOrder1->Type);
	body->orderProperty = TimeInForce_2_DFITCOrderPropertyType(pOrder1->TimeInForce);
	body->instrumentType = DFITC_COMM_TYPE;

	//// ��Եڶ������д�������еڶ�����������Ϊ�ǽ�����������
	//if (pOrder2)
	//{
	//	body.CombOffsetFlag[1] = OpenCloseType_2_TThostFtdcOffsetFlagType(pOrder1->OpenClose);
	//	body.CombHedgeFlag[1] = HedgeFlagType_2_TThostFtdcHedgeFlagType(pOrder1->HedgeFlag);
	//	// ���������Ʋֻ��¹��ܣ�û��ʵ���
	//	body.IsSwapOrder = (body.CombOffsetFlag[0] != body.CombOffsetFlag[1]);
	//}

	//�۸�
	//body.OrderPriceType = OrderType_2_TThostFtdcOrderPriceTypeType(pOrder1->Type);


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
		body->localOrderID = nRet;

		//�����浽���У�����ֱ�ӷ���
		int n = m_pApi->ReqInsertOrder((DFITCInsertOrderField*)pRequest->pBuf);
		if (n < 0)
		{
			nRet = n;
			delete pRequest;
			return nullptr;
		}
		else
		{
			// ���ڸ���������ҵ�ԭ���������ڽ�����Ӧ��֪ͨ
			OrderIDType orderId = { 0 };
			sprintf(m_orderInsert_Id, "%d:%d", m_RspUserLogin.sessionID, nRet);

			OrderField* pField = new OrderField();
			memcpy(pField, pOrder1, sizeof(OrderField));
			strcpy(pField->ID, m_orderInsert_Id);
			m_id_platform_order.insert(pair<string, OrderField*>(m_orderInsert_Id, pField));
		}
	}
	delete pRequest;//�����ֱ��ɾ��

	return m_orderInsert_Id;
}

void CTraderApi::OnRspInsertOrder(struct DFITCOrderRspDataRtnField * pOrderRtn, struct DFITCErrorRtnField * pErrorInfo)
{
	OrderIDType orderId = { 0 };
	sprintf(orderId, "%d:%d", m_RspUserLogin.sessionID, pOrderRtn->localOrderID);

	hash_map<string, OrderField*>::iterator it = m_id_platform_order.find(orderId);
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
		pField->ErrorID = pErrorInfo->nErrorID;
		strncpy(pField->Text, pErrorInfo->errorMsg, sizeof(DFITCErrorMsgInfoType));
		XRespone(ResponeType::OnRtnOrder, m_msgQueue, this, 0, 0, pField, sizeof(OrderField), nullptr, 0, nullptr, 0);
	}
}
//
//void CTraderApi::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
//{
//	OrderIDType orderId = { 0 };
//	sprintf(orderId, "%d:%d:%s", m_RspUserLogin.FrontID, m_RspUserLogin.SessionID, pInputOrder->OrderRef);
//
//	hash_map<string, OrderField*>::iterator it = m_id_platform_order.find(orderId);
//	if (it == m_id_platform_order.end())
//	{
//		// û�ҵ�����Ӧ�������ʾ������
//		assert(false);
//	}
//	else
//	{
//		// �ҵ��ˣ�Ҫ����״̬
//		// ��ʹ���ϴε�״̬
//		OrderField* pField = it->second;
//		pField->ExecType = ExecType::ExecRejected;
//		pField->Status = OrderStatus::Rejected;
//		pField->ErrorID = pRspInfo->ErrorID;
//		strncpy(pField->Text, pRspInfo->ErrorMsg, sizeof(TThostFtdcErrorMsgType));
//		XRespone(ResponeType::OnRtnOrder, m_msgQueue, this, 0, 0, pField, sizeof(OrderField), nullptr, 0, nullptr, 0);
//	}
//}

//int CTraderApi::ReqParkedOrderInsert(
//	int OrderRef,
//	OrderField* pOrder1,
//	OrderField* pOrder2)
//{
//	if (nullptr == m_pApi)
//		return 0;
//
//	SRequest* pRequest = MakeRequestBuf(E_ParkedOrderField);
//	if (nullptr == pRequest)
//		return 0;
//
//	CThostFtdcParkedOrderField& body = pRequest->ParkedOrderField;
//
//	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TThostFtdcBrokerIDType));
//	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TThostFtdcInvestorIDType));
//
//	body.MinVolume = 1;
//	body.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
//	body.IsAutoSuspend = 0;
//	body.UserForceClose = 0;
//	body.IsSwapOrder = 0;
//
//	//��Լ
//	strncpy(body.InstrumentID, pOrder1->InstrumentID, sizeof(TThostFtdcInstrumentIDType));
//	//����
//	body.Direction = OrderSide_2_TThostFtdcDirectionType(pOrder1->Side);
//	//��ƽ
//	body.CombOffsetFlag[0] = OpenCloseType_2_TThostFtdcOffsetFlagType(pOrder1->OpenClose);
//	//Ͷ��
//	body.CombHedgeFlag[0] = HedgeFlagType_2_TThostFtdcHedgeFlagType(pOrder1->HedgeFlag);
//	//����
//	body.VolumeTotalOriginal = (int)pOrder1->Qty;
//
//	// ���������������õ�һ�������ļ۸񣬻��������������ļ۸���أ�
//	body.LimitPrice = pOrder1->Price;
//	body.StopPrice = pOrder1->StopPx;
//
//	// ��Եڶ������д�������еڶ�����������Ϊ�ǽ�����������
//	if (pOrder2)
//	{
//		body.CombOffsetFlag[1] = OpenCloseType_2_TThostFtdcOffsetFlagType(pOrder1->OpenClose);
//		body.CombHedgeFlag[1] = HedgeFlagType_2_TThostFtdcHedgeFlagType(pOrder1->HedgeFlag);
//		// ���������Ʋֻ��¹��ܣ�û��ʵ���
//		body.IsSwapOrder = (body.CombOffsetFlag[0] != body.CombOffsetFlag[1]);
//	}
//
//	//�۸�
//	//body.OrderPriceType = OrderType_2_TThostFtdcOrderPriceTypeType(pOrder1->Type);
//
//	// �м����޼�
//	switch (pOrder1->Type)
//	{
//	case Market:
//	case Stop:
//	case MarketOnClose:
//	case TrailingStop:
//		body.OrderPriceType = THOST_FTDC_OPT_AnyPrice;
//		body.TimeCondition = THOST_FTDC_TC_IOC;
//		break;
//	case Limit:
//	case StopLimit:
//	case TrailingStopLimit:
//	default:
//		body.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
//		body.TimeCondition = THOST_FTDC_TC_GFD;
//		break;
//	}
//
//	// IOC��FOK
//	switch (pOrder1->TimeInForce)
//	{
//	case IOC:
//		body.TimeCondition = THOST_FTDC_TC_IOC;
//		body.VolumeCondition = THOST_FTDC_VC_AV;
//		break;
//	case FOK:
//		body.TimeCondition = THOST_FTDC_TC_IOC;
//		body.VolumeCondition = THOST_FTDC_VC_CV;
//		//body.MinVolume = body.VolumeTotalOriginal; // ����ط��������
//		break;
//	default:
//		body.VolumeCondition = THOST_FTDC_VC_AV;
//		break;
//	}
//
//	// ������
//	switch (pOrder1->Type)
//	{
//	case Stop:
//	case TrailingStop:
//	case StopLimit:
//	case TrailingStopLimit:
//		// ������û�в��ԣ�������
//		body.ContingentCondition = THOST_FTDC_CC_Immediately;
//		break;
//	default:
//		body.ContingentCondition = THOST_FTDC_CC_Immediately;
//		break;
//	}
//
//	int nRet = 0;
//	{
//		//���ܱ���̫�죬m_nMaxOrderRef��û�иı���ύ��
//		lock_guard<mutex> cl(m_csOrderRef);
//
//		if (OrderRef < 0)
//		{
//			nRet = m_nMaxOrderRef;
//			++m_nMaxOrderRef;
//		}
//		else
//		{
//			nRet = OrderRef;
//		}
//		sprintf(body.OrderRef, "%d", nRet);
//
//		//�����浽���У�����ֱ�ӷ���
//		int n = m_pApi->ReqParkedOrderInsert(&pRequest->ParkedOrderField, ++m_lRequestID);
//		if (n < 0)
//		{
//			nRet = n;
//		}
//		else
//		{
//			// ���ڸ���������ҵ�ԭ���������ڽ�����Ӧ��֪ͨ
//			OrderIDType orderId = { 0 };
//			sprintf(orderId, "%d:%d:%d", m_RspUserLogin.FrontID, m_RspUserLogin.SessionID, nRet);
//
//			OrderField* pField = new OrderField();
//			memcpy(pField, pOrder1, sizeof(OrderField));
//			m_id_platform_order.insert(pair<string, OrderField*>(orderId, pField));
//		}
//	}
//	delete pRequest;//�����ֱ��ɾ��
//
//	return nRet;
//}

void CTraderApi::OnRtnMatchedInfo(struct DFITCMatchRtnField * pRtnMatchData)
{
	OnTrade(pRtnMatchData);
}

//int CTraderApi::ReqOrderAction(const string& szId)
//{
//	hash_map<string, CThostFtdcOrderField*>::iterator it = m_id_api_order.find(szId);
//	if (it == m_id_api_order.end())
//	{
//		// <error id="ORDER_NOT_FOUND" value="25" prompt="CTP:�����Ҳ�����Ӧ����"/>
//		//ErrorField field = { 0 };
//		//field.ErrorID = 25;
//		//sprintf(field.ErrorMsg, "ORDER_NOT_FOUND");
//
//		////TODO:Ӧ��ͨ�������ر�֪ͨ�����Ҳ���
//
//		//XRespone(ResponeType::OnRtnError, m_msgQueue, this, 0, 0, &field, sizeof(ErrorField), nullptr, 0, nullptr, 0);
//		return -100;
//	}
//	else
//	{
//		// �ҵ��˶���
//		return ReqOrderAction(it->second);
//	}
//}
//
//int CTraderApi::ReqOrderAction(CThostFtdcOrderField *pOrder)
//{
//	if (nullptr == m_pApi)
//		return 0;
//
//	SRequest* pRequest = MakeRequestBuf(E_InputOrderActionField);
//	if (nullptr == pRequest)
//		return 0;
//
//	CThostFtdcInputOrderActionField& body = pRequest->InputOrderActionField;
//
//	///���͹�˾����
//	strncpy(body.BrokerID, pOrder->BrokerID, sizeof(TThostFtdcBrokerIDType));
//	///Ͷ���ߴ���
//	strncpy(body.InvestorID, pOrder->InvestorID, sizeof(TThostFtdcInvestorIDType));
//	///��������
//	strncpy(body.OrderRef, pOrder->OrderRef, sizeof(TThostFtdcOrderRefType));
//	///ǰ�ñ��
//	body.FrontID = pOrder->FrontID;
//	///�Ự���
//	body.SessionID = pOrder->SessionID;
//	///����������
//	strncpy(body.ExchangeID, pOrder->ExchangeID, sizeof(TThostFtdcExchangeIDType));
//	///�������
//	strncpy(body.OrderSysID, pOrder->OrderSysID, sizeof(TThostFtdcOrderSysIDType));
//	///������־
//	body.ActionFlag = THOST_FTDC_AF_Delete;
//	///��Լ����
//	strncpy(body.InstrumentID, pOrder->InstrumentID, sizeof(TThostFtdcInstrumentIDType));
//
//	int nRet = m_pApi->ReqOrderAction(&pRequest->InputOrderActionField, ++m_lRequestID);
//	delete pRequest;
//	return nRet;
//}

void CTraderApi::OnRspCancelOrder(struct DFITCOrderRspDataRtnField *pOrderCanceledRtn, struct DFITCErrorRtnField *pErrorInfo)
{
	//OrderIDType orderId = { 0 };
	//sprintf(orderId, "%d:%d:%s", , pInputOrderAction->SessionID, pInputOrderAction->OrderRef);

	//hash_map<string, OrderField*>::iterator it = m_id_platform_order.find(orderId);
	//if (it == m_id_platform_order.end())
	//{
	//	// û�ҵ�����Ӧ�������ʾ������
	//	assert(false);
	//}
	//else
	//{
	//	// �ҵ��ˣ�Ҫ����״̬
	//	// ��ʹ���ϴε�״̬
	//	OrderField* pField = it->second;
	//	strcpy(pField->ID, orderId);
	//	pField->ExecType = ExecType::ExecCancelReject;
	//	pField->ErrorID = pRspInfo->ErrorID;
	//	strncpy(pField->Text, pRspInfo->ErrorMsg, sizeof(TThostFtdcErrorMsgType));
	//	XRespone(ResponeType::OnRtnOrder, m_msgQueue, this, 0, 0, pField, sizeof(OrderField), nullptr, 0, nullptr, 0);
	//}
}

//void CTraderApi::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo)
//{
//	OrderIDType orderId = { 0 };
//	sprintf(orderId, "%d:%d:%s", pOrderAction->FrontID, pOrderAction->SessionID, pOrderAction->OrderRef);
//
//	hash_map<string, OrderField*>::iterator it = m_id_platform_order.find(orderId);
//	if (it == m_id_platform_order.end())
//	{
//		// û�ҵ�����Ӧ�������ʾ������
//		assert(false);
//	}
//	else
//	{
//		// �ҵ��ˣ�Ҫ����״̬
//		// ��ʹ���ϴε�״̬
//		OrderField* pField = it->second;
//		strcpy(pField->ID, orderId);
//		pField->ExecType = ExecType::ExecCancelReject;
//		pField->ErrorID = pRspInfo->ErrorID;
//		strncpy(pField->Text, pRspInfo->ErrorMsg, sizeof(TThostFtdcErrorMsgType));
//		XRespone(ResponeType::OnRtnOrder, m_msgQueue, this, 0, 0, pField, sizeof(OrderField), nullptr, 0, nullptr, 0);
//	}
//}

void CTraderApi::OnRtnOrder(struct DFITCOrderRtnField * pRtnOrderData)
{
	OnOrder(pRtnOrderData);
}

//int CTraderApi::ReqQuoteInsert(
//	int QuoteRef,
//	OrderField* pOrderAsk,
//	OrderField* pOrderBid)
//{
//	if (nullptr == m_pApi)
//		return 0;
//
//	SRequest* pRequest = MakeRequestBuf(E_InputQuoteField);
//	if (nullptr == pRequest)
//		return 0;
//
//	CThostFtdcInputQuoteField& body = pRequest->InputQuoteField;
//
//	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TThostFtdcBrokerIDType));
//	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TThostFtdcInvestorIDType));
//
//	//��Լ,Ŀǰֻ�Ӷ���1��ȡ
//	strncpy(body.InstrumentID, pOrderAsk->InstrumentID, sizeof(TThostFtdcInstrumentIDType));
//	//��ƽ
//	body.AskOffsetFlag = OpenCloseType_2_TThostFtdcOffsetFlagType(pOrderAsk->OpenClose);
//	body.BidOffsetFlag = OpenCloseType_2_TThostFtdcOffsetFlagType(pOrderBid->OpenClose);
//	//Ͷ��
//	body.AskHedgeFlag = HedgeFlagType_2_TThostFtdcHedgeFlagType(pOrderAsk->HedgeFlag);
//	body.BidHedgeFlag = HedgeFlagType_2_TThostFtdcHedgeFlagType(pOrderBid->HedgeFlag);
//
//	//�۸�
//	body.AskPrice = pOrderAsk->Price;
//	body.BidPrice = pOrderBid->Price;
//
//	//����
//	body.AskVolume = (int)pOrderAsk->Qty;
//	body.BidVolume = (int)pOrderBid->Qty;
//
//	int nRet = 0;
//	{
//		//���ܱ���̫�죬m_nMaxOrderRef��û�иı���ύ��
//		lock_guard<mutex> cl(m_csOrderRef);
//
//		if (QuoteRef < 0)
//		{
//			nRet = m_nMaxOrderRef;
//			sprintf(body.QuoteRef, "%d", m_nMaxOrderRef);
//			sprintf(body.AskOrderRef, "%d", m_nMaxOrderRef);
//			sprintf(body.BidOrderRef, "%d", ++m_nMaxOrderRef);
//			++m_nMaxOrderRef;
//		}
//		else
//		{
//			nRet = QuoteRef;
//			sprintf(body.QuoteRef, "%d", QuoteRef);
//			sprintf(body.AskOrderRef, "%d", QuoteRef);
//			sprintf(body.BidOrderRef, "%d", ++QuoteRef);
//			++QuoteRef;
//		}
//
//		//�����浽���У�����ֱ�ӷ���
//		int n = m_pApi->ReqQuoteInsert(&pRequest->InputQuoteField, ++m_lRequestID);
//		if (n < 0)
//		{
//			nRet = n;
//		}
//	}
//	delete pRequest;//�����ֱ��ɾ��
//
//	return nRet;
//}
//
//void CTraderApi::OnRspQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
//{
//	//if (m_msgQueue)
//	//	m_msgQueue->Input_OnRspQuoteInsert(this, pInputQuote, pRspInfo, nRequestID, bIsLast);
//}
//
//void CTraderApi::OnErrRtnQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo)
//{
//	//if (m_msgQueue)
//	//	m_msgQueue->Input_OnErrRtnQuoteInsert(this, pInputQuote, pRspInfo);
//}
//
//void CTraderApi::OnRtnQuote(CThostFtdcQuoteField *pQuote)
//{
//	//if (m_msgQueue)
//	//	m_msgQueue->Input_OnRtnQuote(this, pQuote);
//}
//
//int CTraderApi::ReqQuoteAction(const string& szId)
//{
//	hash_map<string, CThostFtdcQuoteField*>::iterator it = m_id_api_quote.find(szId);
//	if (it == m_id_api_quote.end())
//	{
//		// <error id="QUOTE_NOT_FOUND" value="86" prompt="CTP:���۳����Ҳ�����Ӧ����"/>
//		ErrorField field = { 0 };
//		field.ErrorID = 86;
//		sprintf(field.ErrorMsg, "QUOTE_NOT_FOUND");
//
//		XRespone(ResponeType::OnRtnError, m_msgQueue, this, 0, 0, &field, sizeof(ErrorField), nullptr, 0, nullptr, 0);
//	}
//	else
//	{
//		// �ҵ��˶���
//		ReqQuoteAction(it->second);
//	}
//	return 0;
//}
//
//int CTraderApi::ReqQuoteAction(CThostFtdcQuoteField *pQuote)
//{
//	if (nullptr == m_pApi)
//		return 0;
//
//	SRequest* pRequest = MakeRequestBuf(E_InputQuoteActionField);
//	if (nullptr == pRequest)
//		return 0;
//
//	CThostFtdcInputQuoteActionField& body = pRequest->InputQuoteActionField;
//
//	///���͹�˾����
//	strncpy(body.BrokerID, pQuote->BrokerID, sizeof(TThostFtdcBrokerIDType));
//	///Ͷ���ߴ���
//	strncpy(body.InvestorID, pQuote->InvestorID, sizeof(TThostFtdcInvestorIDType));
//	///��������
//	strncpy(body.QuoteRef, pQuote->QuoteRef, sizeof(TThostFtdcOrderRefType));
//	///ǰ�ñ��
//	body.FrontID = pQuote->FrontID;
//	///�Ự���
//	body.SessionID = pQuote->SessionID;
//	///����������
//	strncpy(body.ExchangeID, pQuote->ExchangeID, sizeof(TThostFtdcExchangeIDType));
//	///�������
//	strncpy(body.QuoteSysID, pQuote->QuoteSysID, sizeof(TThostFtdcOrderSysIDType));
//	///������־
//	body.ActionFlag = THOST_FTDC_AF_Delete;
//	///��Լ����
//	strncpy(body.InstrumentID, pQuote->InstrumentID, sizeof(TThostFtdcInstrumentIDType));
//
//	int nRet = m_pApi->ReqQuoteAction(&pRequest->InputQuoteActionField, ++m_lRequestID);
//	delete pRequest;
//	return nRet;
//}
//
//void CTraderApi::OnRspQuoteAction(CThostFtdcInputQuoteActionField *pInputQuoteAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
//{
//	//if (m_msgQueue)
//	//	m_msgQueue->Input_OnRspQuoteAction(this, pInputQuoteAction, pRspInfo, nRequestID, bIsLast);
//}
//
//void CTraderApi::OnErrRtnQuoteAction(CThostFtdcQuoteActionField *pQuoteAction, CThostFtdcRspInfoField *pRspInfo)
//{
//	//if (m_msgQueue)
//	//	m_msgQueue->Input_OnErrRtnQuoteAction(this, pQuoteAction, pRspInfo);
//}

void CTraderApi::ReqQryCustomerCapital()
{
	if (nullptr == m_pApi)
		return;

	SRequest* pRequest = MakeRequestBuf(E_CapitalField);
	if (nullptr == pRequest)
		return;

	DFITCCapitalField* body = (DFITCCapitalField*)pRequest->pBuf;

	strcpy(body->accountID, m_RspUserLogin.accountID);

	AddToSendQueue(pRequest);
}

void CTraderApi::OnRspCustomerCapital(struct DFITCCapitalInfoRtnField * pCapitalInfoRtn, struct DFITCErrorRtnField * pErrorInfo, bool bIsLast)
{
	if (!IsErrorRspInfo(pErrorInfo))
	{
		if (pCapitalInfoRtn)
		{
			AccountField field = { 0 };
			field.PreBalance = pCapitalInfoRtn->preEquity;
			field.CurrMargin = pCapitalInfoRtn->margin;
			field.Commission = pCapitalInfoRtn->fee;
			field.CloseProfit = pCapitalInfoRtn->closeProfitLoss;
			field.PositionProfit = pCapitalInfoRtn->positionProfitLoss;
			field.Balance = pCapitalInfoRtn->todayEquity;
			field.Available = pCapitalInfoRtn->available;

			XRespone(ResponeType::OnRspQryTradingAccount, m_msgQueue, this, bIsLast, 0, &field, sizeof(AccountField), nullptr, 0, nullptr, 0);
		}
		else
		{
			XRespone(ResponeType::OnRspQryTradingAccount, m_msgQueue, this, bIsLast, 0, nullptr, 0, nullptr, 0, nullptr, 0);
		}
	}

	if (bIsLast)
		ReleaseRequestMapBuf(pErrorInfo->requestID);
}

//void CTraderApi::ReqQryPosition(const string& szInstrumentId)
//{
//	if (nullptr == m_pApi)
//		return;
//
//	SRequest* pRequest = MakeRequestBuf(E_PositionField);
//	if (nullptr == pRequest)
//		return;
//
//	DFITCPositionField* body = (DFITCPositionField*)pRequest->pBuf;
//
//	strncpy(body->accountID, m_RspUserLogin.accountID, sizeof(DFITCAccountIDType));
//	strncpy(body->instrumentID, szInstrumentId.c_str(), sizeof(DFITCInstrumentIDType));
//	//body->instrumentType;
//
//	AddToSendQueue(pRequest);
//}
//
//void CTraderApi::OnRspQryPosition(struct DFITCPositionInfoRtnField * pPositionInfoRtn, struct DFITCErrorRtnField * pErrorInfo, bool bIsLast)
//{
//	//if(m_msgQueue)
//	//	m_msgQueue->Input_OnRspQryInvestorPosition(this,pInvestorPosition,pRspInfo,nRequestID,bIsLast);
//
//	if (bIsLast)
//		ReleaseRequestMapBuf(pErrorInfo->requestID);
//}
//
//void CTraderApi::ReqQryInvestorPositionDetail(const string& szInstrumentId)
//{
//	if (nullptr == m_pApi)
//		return;
//
//	SRequest* pRequest = MakeRequestBuf(E_QryInvestorPositionDetailField);
//	if (nullptr == pRequest)
//		return;
//
//	CThostFtdcQryInvestorPositionDetailField& body = pRequest->QryInvestorPositionDetailField;
//
//	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TThostFtdcBrokerIDType));
//	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TThostFtdcInvestorIDType));
//	strncpy(body.InstrumentID, szInstrumentId.c_str(), sizeof(TThostFtdcInstrumentIDType));
//
//	AddToSendQueue(pRequest);
//}
//
//void CTraderApi::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
//{
//	//if(m_msgQueue)
//	//	m_msgQueue->Input_OnRspQryInvestorPositionDetail(this,pInvestorPositionDetail,pRspInfo,nRequestID,bIsLast);
//
//	if (bIsLast)
//		ReleaseRequestMapBuf(nRequestID);
//}

void CTraderApi::ReqQryExchangeInstrument(const string& szExchangeId, DFITCInstrumentTypeType instrumentType)
{
	if (nullptr == m_pApi)
		return;

	SRequest* pRequest = MakeRequestBuf(E_ExchangeInstrumentField);
	if (nullptr == pRequest)
		return;

	DFITCExchangeInstrumentField* body = (DFITCExchangeInstrumentField*)pRequest->pBuf;

	strcpy(body->accountID, m_RspUserLogin.accountID);
	strncpy(body->exchangeID, szExchangeId.c_str(), sizeof(DFITCExchangeIDType));
	body->instrumentType = instrumentType;

	AddToSendQueue(pRequest);
}

void CTraderApi::OnRspQryExchangeInstrument(struct DFITCExchangeInstrumentRtnField * pInstrumentData, struct DFITCErrorRtnField * pErrorInfo, bool bIsLast)
{
	if (!IsErrorRspInfo(pErrorInfo))
	{
		if (pInstrumentData)
		{
			InstrumentField field = { 0 };

			strncpy(field.InstrumentID, pInstrumentData->instrumentID, sizeof(DFITCInstrumentIDType));
			strncpy(field.ExchangeID, pInstrumentData->exchangeID, sizeof(DFITCExchangeIDType));

			strncpy(field.Symbol, pInstrumentData->instrumentID, sizeof(DFITCInstrumentIDType));

			strncpy(field.InstrumentName, pInstrumentData->VarietyName, sizeof(DFITCVarietyNameType));
			field.Type = DFITCInstrumentTypeType_2_InstrumentType(pInstrumentData->instrumentType);
			field.VolumeMultiple = pInstrumentData->contractMultiplier;
			field.PriceTick = pInstrumentData->minPriceFluctuation;
			strncpy(field.ExpireDate, pInstrumentData->instrumentMaturity, sizeof(DFITCInstrumentMaturityType));
			//field.OptionsType = TThostFtdcOptionsTypeType_2_PutCall(pInstrument->OptionsType);

			XRespone(ResponeType::OnRspQryInstrument, m_msgQueue, this, bIsLast, 0, &field, sizeof(InstrumentField), nullptr, 0, nullptr, 0);
		}
		else
		{
			XRespone(ResponeType::OnRspQryInstrument, m_msgQueue, this, bIsLast, 0, nullptr, 0, nullptr, 0, nullptr, 0);
		}
	}

	if (bIsLast&&pErrorInfo)
		ReleaseRequestMapBuf(pErrorInfo->requestID);
}


void CTraderApi::ReqQryArbitrageInstrument(const string& szExchangeId)
{
	if (NULL == m_pApi)
		return;

	SRequest* pRequest = MakeRequestBuf(E_AbiInstrumentField);
	if (NULL == pRequest)
		return;

	DFITCAbiInstrumentField* body = (DFITCAbiInstrumentField*)pRequest->pBuf;

	strcpy(body->accountID, m_RspUserLogin.accountID);
	strncpy(body->exchangeID, szExchangeId.c_str(), sizeof(DFITCExchangeIDType));

	AddToSendQueue(pRequest);
}

void CTraderApi::OnRspArbitrageInstrument(struct DFITCAbiInstrumentRtnField * pAbiInstrumentData, struct DFITCErrorRtnField * pErrorInfo, bool bIsLast)
{
	if (!IsErrorRspInfo(pErrorInfo))
	{
		if (pAbiInstrumentData)
		{
			InstrumentField field = { 0 };

			strncpy(field.InstrumentID, pAbiInstrumentData->InstrumentID, sizeof(DFITCInstrumentIDType));
			strncpy(field.ExchangeID, pAbiInstrumentData->exchangeID, sizeof(DFITCExchangeIDType));

			strncpy(field.Symbol, pAbiInstrumentData->InstrumentID, sizeof(DFITCInstrumentIDType));

			strncpy(field.InstrumentName, pAbiInstrumentData->instrumentName, sizeof(DFITCVarietyNameType));
			/*field.Type = DFITCInstrumentTypeType_2_InstrumentType(pInstrumentData->instrumentType);
			field.VolumeMultiple = pInstrumentData->contractMultiplier;
			field.PriceTick = pInstrumentData->minPriceFluctuation;
			strncpy(field.ExpireDate, pInstrumentData->instrumentMaturity, sizeof(DFITCInstrumentMaturityType));*/
			//field.OptionsType = TThostFtdcOptionsTypeType_2_PutCall(pInstrument->OptionsType);

			XRespone(ResponeType::OnRspQryInstrument, m_msgQueue, this, bIsLast, 0, &field, sizeof(InstrumentField), nullptr, 0, nullptr, 0);
		}
		else
		{
			XRespone(ResponeType::OnRspQryInstrument, m_msgQueue, this, bIsLast, 0, nullptr, 0, nullptr, 0, nullptr, 0);
		}
	}

	if (bIsLast&&pErrorInfo)
		ReleaseRequestMapBuf(pErrorInfo->requestID);
}

//void CTraderApi::ReqQryInstrumentCommissionRate(const string& szInstrumentId)
//{
//	if (nullptr == m_pApi)
//		return;
//
//	SRequest* pRequest = MakeRequestBuf(E_QryInstrumentCommissionRateField);
//	if (nullptr == pRequest)
//		return;
//
//	CThostFtdcQryInstrumentCommissionRateField& body = pRequest->QryInstrumentCommissionRateField;
//
//	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TThostFtdcBrokerIDType));
//	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TThostFtdcInvestorIDType));
//	strncpy(body.InstrumentID, szInstrumentId.c_str(), sizeof(TThostFtdcInstrumentIDType));
//
//	AddToSendQueue(pRequest);
//}
//
//void CTraderApi::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
//{
//	//if(m_msgQueue)
//	//	m_msgQueue->Input_OnRspQryInstrumentCommissionRate(this,pInstrumentCommissionRate,pRspInfo,nRequestID,bIsLast);
//
//	if (bIsLast)
//		ReleaseRequestMapBuf(nRequestID);
//}
//
//void CTraderApi::ReqQryInstrumentMarginRate(const string& szInstrumentId, TThostFtdcHedgeFlagType HedgeFlag)
//{
//	if (nullptr == m_pApi)
//		return;
//
//	SRequest* pRequest = MakeRequestBuf(E_QryInstrumentMarginRateField);
//	if (nullptr == pRequest)
//		return;
//
//	CThostFtdcQryInstrumentMarginRateField& body = pRequest->QryInstrumentMarginRateField;
//
//	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TThostFtdcBrokerIDType));
//	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TThostFtdcInvestorIDType));
//	strncpy(body.InstrumentID, szInstrumentId.c_str(), sizeof(TThostFtdcInstrumentIDType));
//	body.HedgeFlag = HedgeFlag;
//
//	AddToSendQueue(pRequest);
//}
//
//void CTraderApi::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
//{
//	//if(m_msgQueue)
//	//	m_msgQueue->Input_OnRspQryInstrumentMarginRate(this,pInstrumentMarginRate,pRspInfo,nRequestID,bIsLast);
//
//	if (bIsLast)
//		ReleaseRequestMapBuf(nRequestID);
//}
//
//void CTraderApi::ReqQryDepthMarketData(const string& szInstrumentId)
//{
//	if (nullptr == m_pApi)
//		return;
//
//	SRequest* pRequest = MakeRequestBuf(E_QryDepthMarketDataField);
//	if (nullptr == pRequest)
//		return;
//
//	CThostFtdcQryDepthMarketDataField& body = pRequest->QryDepthMarketDataField;
//
//	strncpy(body.InstrumentID, szInstrumentId.c_str(), sizeof(TThostFtdcInstrumentIDType));
//
//	AddToSendQueue(pRequest);
//}
//
//void CTraderApi::OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
//{
//	//if(m_msgQueue)
//	//	m_msgQueue->Input_OnRspQryDepthMarketData(this,pDepthMarketData,pRspInfo,nRequestID,bIsLast);
//
//	if (bIsLast)
//		ReleaseRequestMapBuf(nRequestID);
//}
//
//void CTraderApi::ReqQrySettlementInfo(const string& szTradingDay)
//{
//	if (nullptr == m_pApi)
//		return;
//
//	SRequest* pRequest = MakeRequestBuf(E_QrySettlementInfoField);
//	if (nullptr == pRequest)
//		return;
//
//	CThostFtdcQrySettlementInfoField& body = pRequest->QrySettlementInfoField;
//
//	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TThostFtdcBrokerIDType));
//	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TThostFtdcInvestorIDType));
//	strncpy(body.TradingDay, szTradingDay.c_str(), sizeof(TThostFtdcDateType));
//
//	AddToSendQueue(pRequest);
//}
//
//void CTraderApi::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
//{
//	if (!IsErrorRspInfo(pRspInfo, nRequestID, bIsLast))
//	{
//		if (pSettlementInfo)
//		{
//			SettlementInfoField field = { 0 };
//			strncpy(field.TradingDay, pSettlementInfo->TradingDay, sizeof(TThostFtdcDateType));
//			strncpy(field.Content, pSettlementInfo->Content, sizeof(TThostFtdcContentType));
//
//			XRespone(ResponeType::OnRspQrySettlementInfo, m_msgQueue, this, bIsLast, 0, &field, sizeof(SettlementInfoField), nullptr, 0, nullptr, 0);
//		}
//		else
//		{
//			XRespone(ResponeType::OnRspQrySettlementInfo, m_msgQueue, this, bIsLast, 0, nullptr, 0, nullptr, 0, nullptr, 0);
//		}
//	}
//
//	if (bIsLast)
//		ReleaseRequestMapBuf(nRequestID);
//}

//void CTraderApi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
//{
//	IsErrorRspInfo(pRspInfo, nRequestID, bIsLast);
//
//	if (bIsLast)
//		ReleaseRequestMapBuf(nRequestID);
//}
//
//void CTraderApi::ReqQryOrder()
//{
//	if (nullptr == m_pApi)
//		return;
//
//	SRequest* pRequest = MakeRequestBuf(E_QryOrderField);
//	if (nullptr == pRequest)
//		return;
//
//	CThostFtdcQryOrderField& body = pRequest->QryOrderField;
//
//	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TThostFtdcBrokerIDType));
//	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TThostFtdcInvestorIDType));
//
//	AddToSendQueue(pRequest);
//}

void CTraderApi::OnOrder(DFITCOrderRtnField *pOrder)
{
	if (nullptr == pOrder)
		return;

	OrderIDType orderId = { 0 };
	sprintf(orderId, "%ld:%ld", pOrder->sessionID, pOrder->localOrderID);
	OrderIDType orderSydId = { 0 };

	{
		// ����ԭʼ������Ϣ�����ڳ���

		hash_map<string, DFITCOrderRtnField*>::iterator it = m_id_api_order.find(orderId);
		if (it == m_id_api_order.end())
		{
			// �Ҳ����˶�������ʾ���µ�
			DFITCOrderRtnField* pField = new DFITCOrderRtnField();
			memcpy(pField, pOrder, sizeof(DFITCOrderRtnField));
			m_id_api_order.insert(pair<string, DFITCOrderRtnField*>(orderId, pField));
		}
		else
		{
			// �ҵ��˶���
			// ��Ҫ�ٸ��Ʊ������һ�ε�״̬������ֻҪ��һ�ε����ڳ������ɣ����£��������ñȽ�
			DFITCOrderRtnField* pField = it->second;
			memcpy(pField, pOrder, sizeof(DFITCOrderRtnField));
		}

		// ����SysID���ڶ���ɽ��ر��붩��
		sprintf(orderSydId, "%s:%s", pOrder->exchangeID, pOrder->OrderSysID);
		m_sysId_orderId.insert(pair<string, string>(orderSydId, orderId));
	}

	{
		// ��API�Ķ���ת�����Լ��Ľṹ��

		OrderField* pField = nullptr;
		hash_map<string, OrderField*>::iterator it = m_id_platform_order.find(orderId);
		if (it == m_id_platform_order.end())
		{
			// ����ʱ������Ϣ��û�У������Ҳ�����Ӧ�ĵ��ӣ���Ҫ����Order�Ļָ�
			pField = new OrderField();
			memset(pField, 0, sizeof(OrderField));
			strcpy(pField->ID, orderId);
			strcpy(pField->InstrumentID, pOrder->instrumentID);
			strcpy(pField->ExchangeID, pOrder->exchangeID);
			pField->HedgeFlag = DFITCSpeculatorType_2_HedgeFlagType(pOrder->speculator);
			pField->Side = DFITCBuySellTypeType_2_OrderSide(pOrder->buySellType);
			pField->Price = pOrder->insertPrice;
			pField->StopPx = 0;
			//strncpy(pField->Text, pOrder->, sizeof(TThostFtdcErrorMsgType));
			pField->OpenClose = DFITCOpenCloseTypeType_2_OpenCloseType(pOrder->openCloseType);
			pField->Status = DFITCOrderRtnField_2_OrderStatus(pOrder);
			pField->Qty = pOrder->orderAmount;
			pField->Type = DFITCOrderRtnField_2_OrderType(pOrder);
			pField->TimeInForce = DFITCOrderRtnField_2_TimeInForce(pOrder);
			pField->ExecType = ExecType::ExecNew;
			strcpy(pField->OrderID, pOrder->OrderSysID);


			// ��ӵ�map�У������������ߵĶ�ȡ������ʧ��ʱ����֪ͨ��
			m_id_platform_order.insert(pair<string, OrderField*>(orderId, pField));
		}
		else
		{
			pField = it->second;
			strcpy(pField->ID, orderId);
			//pField->LeavesQty = pOrder->;
			pField->Price = pOrder->insertPrice;
			pField->Status = DFITCOrderRtnField_2_OrderStatus(pOrder);
			pField->ExecType = DFITCOrderRtnField_2_ExecType(pOrder);
			strcpy(pField->OrderID, pOrder->OrderSysID);
			//strncpy(pField->Text, pOrder->StatusMsg, sizeof(TThostFtdcErrorMsgType));
		}

		XRespone(ResponeType::OnRtnOrder, m_msgQueue, this, 0, 0, pField, sizeof(OrderField), nullptr, 0, nullptr, 0);
	}
}

//void CTraderApi::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
//{
//	if (!IsErrorRspInfo(pRspInfo, nRequestID, bIsLast))
//	{
//		OnOrder(pOrder);
//	}
//
//	if (bIsLast)
//		ReleaseRequestMapBuf(nRequestID);
//}

//void CTraderApi::ReqQryTrade()
//{
//	if (nullptr == m_pApi)
//		return;
//
//	SRequest* pRequest = MakeRequestBuf(E_QryTradeField);
//	if (nullptr == pRequest)
//		return;
//
//	CThostFtdcQryTradeField& body = pRequest->QryTradeField;
//
//	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TThostFtdcBrokerIDType));
//	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TThostFtdcInvestorIDType));
//
//	AddToSendQueue(pRequest);
//}

void CTraderApi::OnTrade(DFITCMatchRtnField *pTrade)
{
	if (nullptr == pTrade)
		return;

	TradeField* pField = new TradeField();
	strcpy(pField->InstrumentID, pTrade->instrumentID);
	strcpy(pField->ExchangeID, pTrade->exchangeID);
	pField->Side = DFITCBuySellTypeType_2_OrderSide(pTrade->buySellType);
	pField->Qty = pTrade->matchedAmount;
	//pField->Price = pTrade->Price;
	//pField->OpenClose = TThostFtdcOffsetFlagType_2_OpenCloseType(pTrade->OffsetFlag);
	//pField->HedgeFlag = TThostFtdcHedgeFlagType_2_HedgeFlagType(pTrade->HedgeFlag);
	//pField->Commission = 0;//TODO�������Ժ�Ҫ�������
	//strncpy(pField->Time, pTrade->TradeTime, sizeof(TThostFtdcTimeType));
	//strcpy(pField->TradeID, pTrade->TradeID);

	//OrderIDType orderSysId = { 0 };
	//sprintf(orderSysId, "%s:%s", pTrade->ExchangeID, pTrade->OrderSysID);
	//hash_map<string, string>::iterator it = m_sysId_orderId.find(orderSysId);
	//if (it == m_sysId_orderId.end())
	//{
	//	// �˳ɽ��Ҳ�����Ӧ�ı���
	//	assert(false);
	//}
	//else
	//{
	//	// �ҵ���Ӧ�ı���
	//	strcpy(pField->ID, it->second.c_str());

	//	XRespone(ResponeType::OnRtnTrade, m_msgQueue, this, 0, 0, pField, sizeof(TradeField), nullptr, 0, nullptr, 0);

	//	hash_map<string, OrderField*>::iterator it2 = m_id_platform_order.find(it->second);
	//	if (it2 == m_id_platform_order.end())
	//	{
	//		// �˳ɽ��Ҳ�����Ӧ�ı���
	//		assert(false);
	//	}
	//	else
	//	{
	//		// ���¶�����״̬
	//		// �Ƿ�Ҫ֪ͨ�ӿ�
	//	}
	//}
}

//void CTraderApi::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
//{
//	if (!IsErrorRspInfo(pRspInfo, nRequestID, bIsLast))
//	{
//		OnTrade(pTrade);
//	}
//
//	if (bIsLast)
//		ReleaseRequestMapBuf(nRequestID);
//}

//void CTraderApi::OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus)
//{
//	//if(m_msgQueue)
//	//	m_msgQueue->Input_OnRtnInstrumentStatus(this,pInstrumentStatus);
//}

void CTraderApi::ReqQuoteSubscribe(const string& szExchangeId, DFITCInstrumentTypeType instrumentType)
{
	if (NULL == m_pApi)
		return;

	SRequest* pRequest = MakeRequestBuf(E_QuoteSubscribeField);
	if (NULL == pRequest)
		return;

	DFITCQuoteSubscribeField* body = (DFITCQuoteSubscribeField*)pRequest->pBuf;

	strcpy(body->accountID, m_RspUserLogin.accountID);
	strncpy(body->exchangeID, szExchangeId.c_str(), sizeof(DFITCExchangeIDType));
	body->instrumentType = instrumentType;

	AddToSendQueue(pRequest);
}

void CTraderApi::ReqQuoteUnSubscribe(const string& szExchangeId, DFITCInstrumentTypeType instrumentType)
{
	if (NULL == m_pApi)
		return;

	SRequest* pRequest = MakeRequestBuf(E_QuoteUnSubscribeField);
	if (NULL == pRequest)
		return;

	DFITCQuoteUnSubscribeField* body = (DFITCQuoteUnSubscribeField*)pRequest->pBuf;

	strcpy(body->accountID, m_RspUserLogin.accountID);
	strncpy(body->exchangeID, szExchangeId.c_str(), sizeof(DFITCExchangeIDType));
	body->instrumentType = instrumentType;

	AddToSendQueue(pRequest);
}


void CTraderApi::OnRspQuoteSubscribe(struct DFITCQuoteSubscribeRspField * pRspQuoteSubscribeData)
{

}

void CTraderApi::OnRtnQuoteSubscribe(struct DFITCQuoteSubscribeRtnField * pRtnQuoteSubscribeData)
{
	QuoteRequestField field = { 0 };

	strcpy(field.Symbol, pRtnQuoteSubscribeData->instrumentID);
	strcpy(field.InstrumentID, pRtnQuoteSubscribeData->instrumentID);
	strcpy(field.ExchangeID, pRtnQuoteSubscribeData->exchangeID);
	strcpy(field.TradingDay, pRtnQuoteSubscribeData->tradingDate);
	strcpy(field.QuoteID, pRtnQuoteSubscribeData->quoteID);
	strcpy(field.QuoteTime, pRtnQuoteSubscribeData->quoteTime);

	XRespone(ResponeType::OnRtnQuoteRequest, m_msgQueue, this, 0, 0, &field, sizeof(QuoteRequestField), nullptr, 0, nullptr, 0);
}
