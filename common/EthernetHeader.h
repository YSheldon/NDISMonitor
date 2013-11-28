#ifndef EHERNETHEADER
#define EHERNETHEADER

#define MAX_PROTO_TEXT_LEN	16		//��Э��������󳤶�
#define MAX_PROTO_NUM 12			//��Э������
#define MAX_PACKET_SIZE 65535
#define BUFFER_SIZE 2000

#ifdef SYSMODLE
UINT	Monitor_flag = 0;		// ���ӱ�־��1->���ӣ�0->������
UINT	Filt_flag = 0;			//���˱�־��1->���ˣ�0->������

UINT Filter(UCHAR *pPacket);	// ���˺���
VOID WritePacket2SharedMemory(UCHAR *pPacket,UINT packetsize); // д���ݰ��������ڴ溯��

#else
#define IPPROTO_IP              0               /* dummy for IP */
#define IPPROTO_ICMP            1               /* control message protocol */
#define IPPROTO_IGMP            2               /* internet group management protocol */
#define IPPROTO_GGP             3               /* gateway^2 (deprecated) */
#define IPPROTO_TCP             6               /* tcp */
#define IPPROTO_PUP             12              /* pup */
#define IPPROTO_UDP             17              /* user datagram protocol */
#define IPPROTO_IDP             22              /* xns idp */
#define IPPROTO_ND              77              /* UNOFFICIAL net disk proto */

typedef struct _tagPROTOMAP
{
	BYTE  ProtoNum;
	char ProtoText[MAX_PROTO_TEXT_LEN];
}PROTOMAP;

static PROTOMAP ProtoMap[MAX_PROTO_NUM]=
{
	{ IPPROTO_IP   , "IP"  },
	{ IPPROTO_ICMP , "ICMP" }, 
	{ IPPROTO_IGMP , "IGMP" },
	{ IPPROTO_GGP  , "GGP" }, 
	{ IPPROTO_TCP  , "TCP" }, 
	{ IPPROTO_PUP  , "PUP" }, 
	{ IPPROTO_UDP  , "UDP" }, 
	{ IPPROTO_IDP  , "IDP" }, 
	{ IPPROTO_ND   , "NP"  },  
	{ NULL         , ""     }
};
#endif

typedef struct _TCP_HEADER					//����TCP�ײ�
{
	unsigned short th_sport;				//16λԴ�˿�
	unsigned short th_dport;				//16λĿ�Ķ˿�
	unsigned long  th_seq;					//32λ���к�
	unsigned long  th_ack;					//32λȷ�Ϻ�
	unsigned char th_lenres;				//4λ�ײ�����/6λ������
	unsigned char th_flag;					//6λ��־λ
	unsigned short th_win;					//16λ���ڴ�С
	unsigned short th_checksum;				//16λУ���
	unsigned short th_urp;					//16λ��������ƫ����
}TCP_HEADER;

typedef struct _UDP_HEADER					//����UDP�ײ�
{
    unsigned short uh_sport;				//16λԴ�˿�
    unsigned short uh_dport;				//16λĿ�Ķ˿�
    unsigned short uh_len;					//16λ����
    unsigned short uh_checksum;				//16λУ���
}UDP_HEADER;

typedef struct _ICMP_HEADER					//����ICMP�ײ�
{
	unsigned char   ih_type;				//8λ����
	unsigned char   ih_code;				//8λ����
	unsigned short ih_checksum;				//16λУ��� 
}ICMP_HEADER;

typedef struct _IGMP_HEADER					//����IGMP�ײ�
{
	unsigned char   ih_type;				//8λ����
	unsigned char   ih_max_responsetime;	//8λ�����Ӧʱ��
	unsigned short ih_checksum;				//16λУ��� 
}IGMP_HEADER;


typedef struct _IP_HEADER
{
	unsigned char h_verlen;					//4λ�ײ�����,4λIP�汾��
	unsigned char tos;						//8λ��������TOS
	unsigned short total_len;				//16λ�ܳ��ȣ��ֽڣ�
	unsigned short ident;					//16λ��ʶ
	unsigned short frag_and_flags;			//3λ��־λ��13λƫ��
	unsigned char  ttl;						//8λ����ʱ�� TTL
	unsigned char proto;					//8λЭ�� (1->ICMP, 2->IGMP, 6->TCP, 17->UDP)
	unsigned short checksum;				//16λIP�ײ�У���
	unsigned long sourceIP;					//32λԴIP��ַ
	unsigned long destIP;					//32λĿ��IP��ַ
}IP_HEADER, *PIP_HEADER;

#endif