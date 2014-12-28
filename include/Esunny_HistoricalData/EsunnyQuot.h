#ifndef ESUNNY_QUOT_H
#define ESUNNY_QUOT_H

#pragma	pack(1)
struct STKDATA //��ʱ���ݽṹ
{
	char	Market[32];		//�г�������
	char	Code[16];		//��Լ����

	float	YClose;			//������
	float	YSettle;		//�����
	float	Open;			//���̼�
	float	High;			//��߼�
	float	Low;			//��ͼ�
	float	New;			//���¼�
	float	NetChg;			//�ǵ�
	float	Markup;			//�ǵ���
	float	Swing;			//���
	float	Settle;			//�����
	float	Volume;			//�ɽ���
	float	Amount;			//�ֲ���

	float	Ask[5];			//������
	float	AskVol[5];		//������
	float	Bid[5];			//�����
	float	BidVol[5];		//������			

	float	AvgPrice;		//ƽ����

	float   LimitUp;		//��ͣ��
	float   LimitDown;		//��ͣ��
	float   HistoryHigh;	//��Լ���
	float   HistoryLow;		//��Լ���

	long	YOPI;			//��ֲ�
	float   ZXSD;			//����ʵ��
	float   JXSD;			//����ʵ��
	float   CJJE;			//�ɽ����
};

struct HISTORYDATA //��ʷ����
{  
	char	time[20];	//ʱ��,���磺2009-06-01 08:00:00
	float	fOpen;		//����
	float	fHigh;		//���
	float	fLow;			//���
	float	fClose;		//����
	float	fVolume;		//�ɽ���(��)
	float	fAmount;		//�ɽ���(Ԫ)
};

struct STKHISDATA	//Ʒ����ʷ����
{
	char			Market[32];	//�г�������
	char			Code[16];	//Ʒ�ִ���
	short			nPeriod;	//����
	short			nCount;		//��ʷ������Ŀ
	struct HISTORYDATA	HisData[1];
};

struct STOCKTRACEDATA  //�ɽ���ϸ
{
	char	time[20];	//ʱ��,���磺2009-06-01 08:00:00
	float	m_NewPrice;		//���¼�
	float	m_Volume;		//����
	float	m_Amount;		//�ֲ�
	float	m_BuyPrice;		//ί���
	float	m_SellPrice;	//ί����
	float	m_BuyVol;		//������
	float	m_SellVol;		//������
};

struct STKTRACEDATA //Ʒ����ϸ
{
	char			Market[32];	//�г�������
	char			Code[16];	//Ʒ�ִ���
	unsigned short	nCount;		//��¼��Ŀ
	struct STOCKTRACEDATA	TraceData[1];
};

struct StockInfo//��Լ��Ϣ
{
	char szName[16];//������
	char szCode[16];//��Լ����
};

struct MarketInfo//�����г��ĺ�Լ��Ϣ
{
	char			Market[32];	//�г�������
	unsigned short stocknum;//��Լ��Ŀ
	struct StockInfo	stockdata[1];//��Լ��Ϣ����
};

class IEsunnyQuotClient
{
public:
	/**
	*�����������ݷ�����IP���˿�
	* @param ip ��������IP
	* @param port �������Ķ˿�
	* @return 0��ʾ���ӳɹ�������ʧ��
	*/
	virtual int __cdecl Connect(const char *ip,int port)=0;
	
	/**
	*�Ͽ���ǰ����
	*/
	virtual void __cdecl DisConnect()=0;

	/**
	*��¼�������ݷ�����
	* @param user ��¼�û���
	* @param password ��¼����
	* @return 0��ʾ��¼ָ��ͳɹ���������ʧ�ܣ��Ƿ��¼�ɹ�ͨ���ص�OnRspLogin
	*/
	virtual int __cdecl Login(const char *user,const char *password)=0;

	/**
	*����/ȡ������Ʒ�ּ�ʱ����
	* @param market �г�������
	* @param stk ��Լ����
	* @param need 1��ʾ���ģ�0��ʾȡ������
	* @return 0��ʾ��������ɹ�������ʧ�ܣ��ɹ�֮�󽫻���OnStkQuot���յ�Ʒ�ֵļ�ʱ����
	*/
	virtual int __cdecl RequestQuot(const char *market,const char *stk,int need)=0;

	/**
	*����Ʒ����ʷ����
	* @param market �г�������
	* @param stk ��Լ����
	* @param period ���� 1��1���ӣ�2��5���ӣ�3��60���ӣ�4������
	* @return 0��ʾ����ɹ�������ʧ�ܣ���ʷ���ݽ����OnRspHistoryQuot����
	*/
	virtual int __cdecl RequestHistory(const char *market,const char *stk,int period)=0;

	/**
	*����Ʒ����ϸ����
	* @param market �г�������
	* @param stk ��Լ����
	* @param date Ҫ������ϸ�����ڣ�����20090901
	* @return 0��ʾ����ɹ�������ʧ�ܣ���ʷ���ݽ����OnRspTraceData����
	*/
	virtual int __cdecl RequestTrace(const char *market,const char *stk,const char *date)=0;

	/**
	*��Զ���Ʒ�ֱȽ϶�ʱ����ѭ������AddReqStk��������һ��SendReqStk
	* @param market �г�������
	* @param stk ��Լ����
	* @param need 1��ʾ���ģ�0��ʾȡ������
	* @return 0��ʾ��������ɹ�������ʧ�ܣ��ɹ�֮�󽫻���OnStkQuot���յ�Ʒ�ֵļ�ʱ����
	*/
	virtual int __cdecl AddReqStk(const char *market,const char *stk,int need)=0;
	/**
	*������������Ʒ�ֶ�������
	* @return 0��ʾ��������ɹ�������ʧ�ܣ��ɹ�֮�󽫻���OnStkQuot���յ�Ʒ�ֵļ�ʱ����
	*/
	virtual int __cdecl SendReqStk()=0;

	/**
	*��ȡ���غ�Լ��������
	* @param market �г�������
	* @param stk ��Լ����
	* @param pData ����ṹָ��
	* @return 0��ʾ��ȡ�ɹ�������ʧ�ܣ��ɹ�֮�󽫻���pData�б���Ʒ�ֵļ�ʱ����
	*/
	virtual int __cdecl GetStkData(const char *market,const char *stk,struct STKDATA *pData)=0;
};

class IEsunnyQuotNotify
{
public:
	/**
	*��¼�����ص�����
	* @param err ����� 0��ʾ��¼�ɹ�������ʧ��
	* @param errtext ������Ϣ
	* @return 0��ʾ�ɹ�������ʧ��
	*/
	virtual int __cdecl OnRspLogin(int err,const char *errtext)=0;

	/**
	*�������ӳ���ص�����
	* @param err ����� 
	* @param errtext ������Ϣ
	* @return 0��ʾ�ɹ�������ʧ��
	*/
	virtual int __cdecl OnChannelLost(int err,const char *errtext)=0;

	/**
	*��ʱ����ص�����
	* @param pData ����ṹָ��
	* @return 0��ʾ�ɹ�������ʧ��
	*/
	virtual int __cdecl OnStkQuot(struct STKDATA *pData)=0;

	/**
	*��ʷ���鷴���ص�����
	* @param pHisData ��ʷ�������ݽṹָ��
	* @return 0��ʾ�ɹ�������ʧ��
	*/
	virtual int __cdecl OnRspHistoryQuot(struct STKHISDATA *pHisData)=0;

	/**
	*��ϸ���ݷ����ص�����
	* @param pTraceData ��ϸ���ݽṹָ��
	* @return 0��ʾ�ɹ�������ʧ��
	*/
	virtual int __cdecl OnRspTraceData(struct STKTRACEDATA *pTraceData)=0;

	/**
	*�г���Ϣ�������������յ��г���Ϣ��ʾ�����Ѿ���ʼ������ˣ�
	* @param pMarketInfo �г���Ϣ����ָ��
	* @param bLast �Ƿ�Ϊ���һ���г���bLastΪ1��ʾ�����һ����0��ʾ���滹���г�����
	* @return 0��ʾ�ɹ�������ʧ��,
	*/
	virtual int __cdecl OnRspMarketInfo(struct MarketInfo *pMarketInfo,int bLast)=0;
};

//---------------- API �ӿں���

//�����ӿ�ָ��(ʹ��ǰ����)
IEsunnyQuotClient * __cdecl CreateEsunnyQuotClient(IEsunnyQuotNotify * notify);

//�ͷŽӿ�ָ��(ʹ�ú����)
void __cdecl DelEsunnyQuotClient(IEsunnyQuotClient * client);

#pragma	pack()
#endif