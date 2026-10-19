#include <ns3/simulator.h>
#include <ns3/seq-ts-header.h>
#include <ns3/udp-header.h>
#include <ns3/ipv4-header.h>
#include "ns3/ppp-header.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/data-rate.h"
#include "ns3/pointer.h"
#include "rdma-hw.h"
#include "ppp-header.h"
#include "qbb-header.h"
#include "cn-header.h"
#include <cmath>
#include "ns3/core-module.h"
#include <fstream>

namespace ns3{

TypeId RdmaHw::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::RdmaHw")
		.SetParent<Object> ()
		.AddAttribute("MinRate",
				"Minimum rate of a throttled flow",
				DataRateValue(DataRate("100Mb/s")),
				MakeDataRateAccessor(&RdmaHw::m_minRate),
				MakeDataRateChecker())
		.AddAttribute("Mtu",
				"Mtu.",
				UintegerValue(1000),
				MakeUintegerAccessor(&RdmaHw::m_mtu),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute ("CcMode",
				"which mode of DCQCN is running",
				UintegerValue(0),
				MakeUintegerAccessor(&RdmaHw::m_cc_mode),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute("NACK Generation Interval",
				"The NACK Generation interval",
				DoubleValue(500.0),
				MakeDoubleAccessor(&RdmaHw::m_nack_interval),
				MakeDoubleChecker<double>())
		.AddAttribute("L2ChunkSize",
				"Layer 2 chunk size. Disable chunk mode if equals to 0.",
				UintegerValue(0),
				MakeUintegerAccessor(&RdmaHw::m_chunk),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute("L2AckInterval",
				"Layer 2 Ack intervals. Disable ack if equals to 0.",
				UintegerValue(0),
				MakeUintegerAccessor(&RdmaHw::m_ack_interval),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute("L2BackToZero",
				"Layer 2 go back to zero transmission.",
				BooleanValue(false),
				MakeBooleanAccessor(&RdmaHw::m_backto0),
				MakeBooleanChecker())
		.AddAttribute("EwmaGain",
				"Control gain parameter which determines the level of rate decrease",
				DoubleValue(1.0 / 16),
				MakeDoubleAccessor(&RdmaHw::m_g),
				MakeDoubleChecker<double>())
		.AddAttribute ("RateOnFirstCnp",
				"the fraction of rate on first CNP",
				DoubleValue(1.0),
				MakeDoubleAccessor(&RdmaHw::m_rateOnFirstCNP),
				MakeDoubleChecker<double> ())
		.AddAttribute("ClampTargetRate",
				"Clamp target rate.",
				BooleanValue(false),
				MakeBooleanAccessor(&RdmaHw::m_EcnClampTgtRate),
				MakeBooleanChecker())
		.AddAttribute("RPTimer",
				"The rate increase timer at RP in microseconds",
				DoubleValue(1500.0),
				MakeDoubleAccessor(&RdmaHw::m_rpgTimeReset),
				MakeDoubleChecker<double>())
		.AddAttribute("RateDecreaseInterval",
				"The interval of rate decrease check",
				DoubleValue(4.0),
				MakeDoubleAccessor(&RdmaHw::m_rateDecreaseInterval),
				MakeDoubleChecker<double>())
		.AddAttribute("FastRecoveryTimes",
				"The rate increase timer at RP",
				UintegerValue(5),
				MakeUintegerAccessor(&RdmaHw::m_rpgThreshold),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute("AlphaResumInterval",
				"The interval of resuming alpha",
				DoubleValue(55.0),
				MakeDoubleAccessor(&RdmaHw::m_alpha_resume_interval),
				MakeDoubleChecker<double>())
		.AddAttribute("RateAI",
				"Rate increment unit in AI period",
				DataRateValue(DataRate("5Mb/s")),
				MakeDataRateAccessor(&RdmaHw::m_rai),
				MakeDataRateChecker())
		.AddAttribute("RateHAI",
				"Rate increment unit in hyperactive AI period",
				DataRateValue(DataRate("50Mb/s")),
				MakeDataRateAccessor(&RdmaHw::m_rhai),
				MakeDataRateChecker())
		.AddAttribute("VarWin",
				"Use variable window size or not",
				BooleanValue(false),
				MakeBooleanAccessor(&RdmaHw::m_var_win),
				MakeBooleanChecker())
		.AddAttribute("FastReact",
				"Fast React to congestion feedback",
				BooleanValue(true),
				MakeBooleanAccessor(&RdmaHw::m_fast_react),
				MakeBooleanChecker())
		.AddAttribute("MiThresh",
				"Threshold of number of consecutive AI before MI",
				UintegerValue(5),
				MakeUintegerAccessor(&RdmaHw::m_miThresh),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute("TargetUtil",
				"The Target Utilization of the bottleneck bandwidth, by default 95%",
				DoubleValue(0.95),
				MakeDoubleAccessor(&RdmaHw::m_targetUtil),
				MakeDoubleChecker<double>())
		.AddAttribute("UtilHigh",
				"The upper bound of Target Utilization of the bottleneck bandwidth, by default 98%",
				DoubleValue(0.98),
				MakeDoubleAccessor(&RdmaHw::m_utilHigh),
				MakeDoubleChecker<double>())
		.AddAttribute("RateBound",
				"Bound packet sending by rate, for test only",
				BooleanValue(true),
				MakeBooleanAccessor(&RdmaHw::m_rateBound),
				MakeBooleanChecker())
		.AddAttribute("MultiRate",
				"Maintain multiple rates in HPCC",
				BooleanValue(true),
				MakeBooleanAccessor(&RdmaHw::m_multipleRate),
				MakeBooleanChecker())
		.AddAttribute("SampleFeedback",
				"Whether sample feedback or not",
				BooleanValue(false),
				MakeBooleanAccessor(&RdmaHw::m_sampleFeedback),
				MakeBooleanChecker())
		.AddAttribute("TimelyAlpha",
				"Alpha of TIMELY",
				DoubleValue(0.875),
				MakeDoubleAccessor(&RdmaHw::m_tmly_alpha),
				MakeDoubleChecker<double>())
		.AddAttribute("TimelyBeta",
				"Beta of TIMELY",
				DoubleValue(0.8),
				MakeDoubleAccessor(&RdmaHw::m_tmly_beta),
				MakeDoubleChecker<double>())
		.AddAttribute("TimelyTLow",
				"TLow of TIMELY (ns)",
				UintegerValue(50000),
				MakeUintegerAccessor(&RdmaHw::m_tmly_TLow),
				MakeUintegerChecker<uint64_t>())
		.AddAttribute("TimelyTHigh",
				"THigh of TIMELY (ns)",
				UintegerValue(500000),
				MakeUintegerAccessor(&RdmaHw::m_tmly_THigh),
				MakeUintegerChecker<uint64_t>())
		.AddAttribute("TimelyMinRtt",
				"MinRtt of TIMELY (ns)",
				UintegerValue(20000),
				MakeUintegerAccessor(&RdmaHw::m_tmly_minRtt),
				MakeUintegerChecker<uint64_t>())
		.AddAttribute("DctcpRateAI",
				"DCTCP's Rate increment unit in AI period",
				DataRateValue(DataRate("1000Mb/s")),
				MakeDataRateAccessor(&RdmaHw::m_dctcp_rai),
				MakeDataRateChecker())
		.AddAttribute("PintSmplThresh",
				"PINT's sampling threshold in rand()%65536",
				UintegerValue(65536),
				MakeUintegerAccessor(&RdmaHw::pint_smpl_thresh),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute("EnccStage",
				"ENCC's param stage",
				UintegerValue(4),
				MakeUintegerAccessor(&RdmaHw::m_encc_stage),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute("FpdccK1",
				"FPDCC sender-side normal queue upper bound K1 in bytes.",
				UintegerValue(5000),
				MakeUintegerAccessor(&RdmaHw::m_fpdccK1),
				MakeUintegerChecker<uint64_t>())
		;
	return tid;
}

RdmaHw::RdmaHw(){
}

void RdmaHw::SetNode(Ptr<Node> node){
	m_node = node;
}

void RdmaHw::Setup(QpCompleteCallback cb){
	for (uint32_t i = 0; i < m_nic.size(); i++){
		Ptr<QbbNetDevice> dev = m_nic[i].dev;
		if (dev == NULL)
			continue;
		dev->m_rdmaEQ->m_qpGrp = m_nic[i].qpGrp;
		dev->m_rdmaReceiveCb = MakeCallback(&RdmaHw::Receive, this);
		dev->m_rdmaLinkDownCb = MakeCallback(&RdmaHw::SetLinkDown, this);
		dev->m_rdmaPktSent = MakeCallback(&RdmaHw::PktSent, this);
		dev->m_rdmaEQ->m_rdmaGetNxtPkt = MakeCallback(&RdmaHw::GetNxtPacket, this);
	}
	m_qpCompleteCallback = cb;
}

uint32_t RdmaHw::GetNicIdxOfQp(Ptr<RdmaQueuePair> qp){
	auto &v = m_rtTable[qp->dip.Get()];
	if (v.size() > 0){
		return v[qp->GetHash() % v.size()];
	}else{
		NS_ASSERT_MSG(false, "We assume at least one NIC is alive");
	}
}

uint64_t RdmaHw::GetQpKey(uint32_t dip, uint16_t sport, uint16_t pg){
	return ((uint64_t)dip << 32) | ((uint64_t)sport << 16) | (uint64_t)pg;
}

Ptr<RdmaQueuePair> RdmaHw::GetQp(uint32_t dip, uint16_t sport, uint16_t pg){
	uint64_t key = GetQpKey(dip, sport, pg);
	auto it = m_qpMap.find(key);
	if (it != m_qpMap.end())
		return it->second;
	return NULL;
}

void RdmaHw::AddQueuePair(uint64_t size, uint16_t pg, Ipv4Address sip, Ipv4Address dip,
						  uint16_t sport, uint16_t dport, uint32_t win, uint64_t baseRtt,
						  Callback<void> notifyAppFinish){
	Ptr<RdmaQueuePair> qp = CreateObject<RdmaQueuePair>(pg, sip, dip, sport, dport);
	qp->SetSize(size);

	if(m_cc_mode == 12){
		qp->m_totalNum = (size == 0) ? 0 : ((size - 1) / m_mtu + 1);
		qp->m_lastNum = qp->m_totalNum;
		qp->m_firstPsn = 0;
	}

	qp->SetWin(win);
	qp->SetBaseRtt(baseRtt);
	qp->SetVarWin(m_var_win);
	qp->SetAppNotifyCallback(notifyAppFinish);

	uint32_t nic_idx = GetNicIdxOfQp(qp);
	m_nic[nic_idx].qpGrp->AddQp(qp);
	uint64_t key = GetQpKey(dip.Get(), sport, pg);
	m_qpMap[key] = qp;

	DataRate m_bps = m_nic[nic_idx].dev->GetDataRate();
	qp->m_rate = m_bps;
	qp->m_max_rate = m_bps;
	if (m_cc_mode == 1){
		qp->mlx.m_targetRate = m_bps;
	}else if (m_cc_mode == 3){
		qp->hp.m_curRate = m_bps;
		if (m_multipleRate){
			for (uint32_t i = 0; i < IntHeader::maxHop; i++)
				qp->hp.hopState[i].Rc = m_bps;
		}
	}else if (m_cc_mode == 7){
		qp->tmly.m_curRate = m_bps;
	}else if (m_cc_mode == 10){
		qp->hpccPint.m_curRate = m_bps;
	}

	m_nic[nic_idx].dev->NewQp(qp);
}

void RdmaHw::DeleteQueuePair(Ptr<RdmaQueuePair> qp){
	uint64_t key = GetQpKey(qp->dip.Get(), qp->sport, qp->m_pg);
	m_qpMap.erase(key);
}

Ptr<RdmaRxQueuePair> RdmaHw::GetRxQp(uint32_t sip, uint32_t dip, uint16_t sport,
									  uint16_t dport, uint16_t pg, bool create){
	uint64_t key = ((uint64_t)dip << 32) | ((uint64_t)pg << 16) | (uint64_t)dport;
	auto it = m_rxQpMap.find(key);
	if (it != m_rxQpMap.end())
		return it->second;
	if (create){
		Ptr<RdmaRxQueuePair> q = CreateObject<RdmaRxQueuePair>();
		q->sip = sip;
		q->dip = dip;
		q->sport = sport;
		q->dport = dport;
		q->m_ecn_source.qIndex = pg;
		m_rxQpMap[key] = q;
		return q;
	}
	return NULL;
}

uint32_t RdmaHw::GetNicIdxOfRxQp(Ptr<RdmaRxQueuePair> q){
	auto &v = m_rtTable[q->dip];
	if (v.size() > 0){
		return v[q->GetHash() % v.size()];
	}else{
		NS_ASSERT_MSG(false, "We assume at least one NIC is alive");
	}
}

void RdmaHw::DeleteRxQp(uint32_t dip, uint16_t pg, uint16_t dport){
	uint64_t key = ((uint64_t)dip << 32) | ((uint64_t)pg << 16) | (uint64_t)dport;
	m_rxQpMap.erase(key);
}

int RdmaHw::ReceiveUdp(Ptr<Packet> p, CustomHeader &ch){
	uint8_t ecnbits = ch.GetIpv4EcnBits();
	uint32_t payload_size = p->GetSize() - ch.GetSerializedSize();

	Ptr<RdmaRxQueuePair> rxQp =
		GetRxQp(ch.dip, ch.sip, ch.udp.dport, ch.udp.sport, ch.udp.pg, true);
	if (ecnbits != 0){
		rxQp->m_ecn_source.ecnbits |= ecnbits;
		rxQp->m_ecn_source.qfb++;
	}
	rxQp->m_ecn_source.total++;
	rxQp->m_milestone_rx = m_ack_interval;

	int x = ReceiverCheckSeq(ch.udp.seq, rxQp, payload_size);
	if (x == 1 || x == 2){
		qbbHeader seqh;
		seqh.SetSeq(rxQp->ReceiverNextExpectedSeq);
		seqh.SetPG(ch.udp.pg);
		seqh.SetSport(ch.udp.dport);
		seqh.SetDport(ch.udp.sport);
		seqh.SetIntHeader(ch.udp.ih);
		if (ecnbits)
			seqh.SetCnp();

		Ptr<Packet> newp =
			Create<Packet>(std::max(60-14-20-(int)seqh.GetSerializedSize(), 0));
		newp->AddHeader(seqh);

		Ipv4Header head;
		head.SetDestination(Ipv4Address(ch.sip));
		head.SetSource(Ipv4Address(ch.dip));
		head.SetProtocol(x == 1 ? 0xFC : 0xFD);
		head.SetTtl(64);
		head.SetPayloadSize(newp->GetSize());
		head.SetIdentification(rxQp->m_ipid++);
		newp->AddHeader(head);

		AddHeader(newp, 0x800);
		uint32_t nic_idx = GetNicIdxOfRxQp(rxQp);
		m_nic[nic_idx].dev->RdmaEnqueueHighPrioQ(newp);
		m_nic[nic_idx].dev->TriggerTransmit();
	}
	return 0;
}

int RdmaHw::ReceiveCnp(Ptr<Packet> p, CustomHeader &ch){
	uint32_t qIndex = ch.cnp.qIndex;
	if (qIndex == 1){
		std::cout << "TCP--ignore\n";
		return 0;
	}
	uint16_t udpport = ch.cnp.fid;
	uint8_t ecnbits = ch.cnp.ecnBits;
	uint16_t qfb = ch.cnp.qfb;
	uint16_t total = ch.cnp.total;
	uint32_t i;

	Ptr<RdmaQueuePair> qp = GetQp(ch.sip, udpport, qIndex);
	if (qp == NULL)
		std::cout << "ERROR: QCN NIC cannot find the flow\n";

	uint32_t nic_idx = GetNicIdxOfQp(qp);
	Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev;
	if (qp->m_rate == 0){
		qp->m_rate = dev->GetDataRate();
		if (m_cc_mode == 1){
			qp->mlx.m_targetRate = dev->GetDataRate();
		}else if (m_cc_mode == 3){
			qp->hp.m_curRate = dev->GetDataRate();
			if (m_multipleRate){
				for (uint32_t i = 0; i < IntHeader::maxHop; i++)
					qp->hp.hopState[i].Rc = dev->GetDataRate();
			}
		}else if (m_cc_mode == 7){
			qp->tmly.m_curRate = dev->GetDataRate();
		}else if (m_cc_mode == 10){
			qp->hpccPint.m_curRate = dev->GetDataRate();
		}
	}
	return 0;
}

int RdmaHw::ReceiveAck(Ptr<Packet> p, CustomHeader &ch){
	uint16_t qIndex = ch.ack.pg;
	uint16_t port = ch.ack.dport;
	uint32_t seq = ch.ack.seq;
	uint8_t cnp = (ch.ack.flags >> qbbHeader::FLAG_CNP) & 1;
	int i;

	Ptr<RdmaQueuePair> qp = GetQp(ch.sip, port, qIndex);
	if (qp == NULL){
		std::cout << "ERROR: " << "node:" << m_node->GetId() << ' '
				  << (ch.l3Prot == 0xFC ? "ACK" : "NACK")
				  << " NIC cannot find the flow\n";
		return 0;
	}

	uint32_t nic_idx = GetNicIdxOfQp(qp);
	Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev;

	if (m_ack_interval == 0)
		std::cout << "ERROR: shouldn't receive ack\n";
	else {
		if (!m_backto0){
			qp->Acknowledge(seq);
		}else {
			uint32_t goback_seq = seq / m_chunk * m_chunk;
			qp->Acknowledge(goback_seq);
		}
		if (qp->IsFinished()){
			QpComplete(qp);
		}
	}

	if (ch.l3Prot == 0xFD)
		RecoverQueue(qp);

	if (cnp){
		if (m_cc_mode == 1){
			cnp_received_mlx(qp);
		}
	}

	if (m_cc_mode == 3){
		HandleAckHp(qp, p, ch);
	}else if (m_cc_mode == 7){
		HandleAckTimely(qp, p, ch);
	}else if (m_cc_mode == 8){
		HandleAckDctcp(qp, p, ch);
	}else if (m_cc_mode == 10){
		HandleAckHpPint(qp, p, ch);
	}

	dev->TriggerTransmit();
	return 0;
}

int RdmaHw::Receive(Ptr<Packet> p, CustomHeader &ch){
	if (ch.l3Prot == 0x11){
		ReceiveUdp(p, ch);
	}else if (ch.l3Prot == 0xFF){
		ReceiveCnp(p, ch);
	}else if (ch.l3Prot == 0xFD){
		ReceiveAck(p, ch);
	}else if (ch.l3Prot == 0xFC){
		ReceiveAck(p, ch);
	}else if (ch.l3Prot == 0xF1){
		ReceiveEnp(p, ch);
	}else if (ch.l3Prot == 0xF2){
		ReceivePnp(p, ch);
	}
	return 0;
}

int RdmaHw::ReceiverCheckSeq(uint32_t seq, Ptr<RdmaRxQueuePair> q, uint32_t size){
	uint32_t expected = q->ReceiverNextExpectedSeq;
	if (seq == expected){
		q->ReceiverNextExpectedSeq = expected + size;
		if (q->ReceiverNextExpectedSeq >= q->m_milestone_rx){
			q->m_milestone_rx += m_ack_interval;
			return 1;
		}else if (q->ReceiverNextExpectedSeq % m_chunk == 0){
			return 1;
		}else {
			return 5;
		}
	} else if (seq > expected) {
		if (Simulator::Now() >= q->m_nackTimer || q->m_lastNACK != expected){
			q->m_nackTimer = Simulator::Now() + MicroSeconds(m_nack_interval);
			q->m_lastNACK = expected;
			if (m_backto0){
				q->ReceiverNextExpectedSeq =
					q->ReceiverNextExpectedSeq / m_chunk*m_chunk;
			}
			return 2;
		}else
			return 4;
	}else {
		return 3;
	}
}

void RdmaHw::AddHeader (Ptr<Packet> p, uint16_t protocolNumber){
	PppHeader ppp;
	ppp.SetProtocol (EtherToPpp (protocolNumber));
	p->AddHeader (ppp);
}

uint16_t RdmaHw::EtherToPpp (uint16_t proto){
	switch(proto){
		case 0x0800: return 0x0021;
		case 0x86DD: return 0x0057;
		default: NS_ASSERT_MSG (false, "PPP Protocol number not defined!");
	}
	return 0;
}

void RdmaHw::RecoverQueue(Ptr<RdmaQueuePair> qp){
	qp->snd_nxt = qp->snd_una;
}

void RdmaHw::QpComplete(Ptr<RdmaQueuePair> qp){
	NS_ASSERT(!m_qpCompleteCallback.IsNull());

	if (m_cc_mode == 1){
		Simulator::Cancel(qp->mlx.m_eventUpdateAlpha);
		Simulator::Cancel(qp->mlx.m_eventDecreaseRate);
		Simulator::Cancel(qp->mlx.m_rpTimer);
	}

	//FPDCC: Each switch record maintains an independent expiration timer.
	// All flows must be cancelled upon completion to avoid scheduled callbacks continuing to hold and access completed QPs
	if (m_cc_mode == 12){
		for (auto &entry : qp->fpdcc.pnpRecords){
			Simulator::Cancel(entry.second.validityEvent);
		}
		qp->fpdcc.pnpRecords.clear();
	}
	m_qpCompleteCallback(qp);
	qp->m_notifyAppFinish();
	DeleteQueuePair(qp);
}

void RdmaHw::SetLinkDown(Ptr<QbbNetDevice> dev){
	printf("RdmaHw: node:%u a link down\n", m_node->GetId());
}

void RdmaHw::AddTableEntry(Ipv4Address &dstAddr, uint32_t intf_idx){
	uint32_t dip = dstAddr.Get();
	m_rtTable[dip].push_back(intf_idx);
}

void RdmaHw::ClearTable(){
	m_rtTable.clear();
}

void RdmaHw::RedistributeQp(){
	for (uint32_t i = 0; i < m_nic.size(); i++){
		if (m_nic[i].dev == NULL)
			continue;
		m_nic[i].qpGrp->Clear();
	}

	for (auto &it : m_qpMap){
		Ptr<RdmaQueuePair> qp = it.second;
		uint32_t nic_idx = GetNicIdxOfQp(qp);
		m_nic[nic_idx].qpGrp->AddQp(qp);
		m_nic[nic_idx].dev->ReassignedQp(qp);
	}
}

//FPDCC: Calculate the persistence of the flow
uint8_t RdmaHw::CalculatePersistenceRaw(Ptr<RdmaQueuePair> qp) const{
	if (qp == NULL || qp->m_totalNum == 0 || m_mtu == 0 ||
		qp->m_baseRtt == 0 || qp->m_max_rate.GetBitRate() == 0){
		return 0;
	}

	const double baseRttSec =  static_cast<double>(qp->m_baseRtt) * 1e-9;
	const double baseBdpBytes = static_cast<double>(qp->m_max_rate.GetBitRate()) * baseRttSec / 8.0;
	const double baseBdpPackets = baseBdpBytes / static_cast<double>(m_mtu);

	const double S = static_cast<double>(qp->m_totalNum);
	const double R = static_cast<double>(qp->m_lastNum);

	// p = [0,1]
	const double persistence = 1.0 - (1.0 - R / S) * baseBdpPackets / (R + baseBdpPackets);

	//Quantify into [0,255]
	uint32_t raw = static_cast<uint32_t>(persistence * 255.0 + 0.5);

	if (raw > 255){raw = 255;}

	return static_cast<uint8_t>(raw);
}

Ptr<Packet> RdmaHw::GetNxtPacket(Ptr<RdmaQueuePair> qp){
	uint32_t payload_size = qp->GetBytesLeft();
	if (m_mtu < payload_size)
		payload_size = m_mtu;

	if (m_cc_mode == 12 && qp->snd_nxt == qp->m_firstPsn){
		if (qp->m_lastNum > 0){
			qp->m_lastNum--;
		}
		qp->m_firstPsn += payload_size;
	}

	Ptr<Packet> p = Create<Packet> (payload_size);

	SeqTsHeader seqTs;
	seqTs.SetSeq (qp->snd_nxt);
	seqTs.SetPG (qp->m_pg);

	if (m_cc_mode == 12){
		const uint8_t persistenceRaw = CalculatePersistenceRaw(qp);
		seqTs.ih.SetPersistence(persistenceRaw);
	}

	p->AddHeader (seqTs);

	UdpHeader udpHeader;
	udpHeader.SetDestinationPort (qp->dport);
	udpHeader.SetSourcePort (qp->sport);
	p->AddHeader (udpHeader);

	Ipv4Header ipHeader;
	ipHeader.SetSource (qp->sip);
	ipHeader.SetDestination (qp->dip);
	ipHeader.SetProtocol (0x11);
	ipHeader.SetPayloadSize (p->GetSize());
	ipHeader.SetTtl (64);
	ipHeader.SetTos (0);
	ipHeader.SetIdentification (qp->m_ipid);
	p->AddHeader(ipHeader);

	PppHeader ppp;
	ppp.SetProtocol (0x0021);
	p->AddHeader (ppp);

	qp->snd_nxt += payload_size;
	qp->m_ipid++;

	return p;
}

void RdmaHw::PktSent(Ptr<RdmaQueuePair> qp, Ptr<Packet> pkt, Time interframeGap){
	qp->lastPktSize = pkt->GetSize();
	UpdateNextAvail(qp, interframeGap, pkt->GetSize());

	static uint32_t cnt = 0, pkt_size_sum = 0;
	static uint64_t last_time = 0;
	if (m_node->GetId() == 2 && 0) {
		pkt_size_sum += pkt->GetSize();
		if (++cnt == 50) {
			uint64_t time_gap = Simulator::Now().GetTimeStep() - last_time;
			double tmp_rate =
				(static_cast<double>(pkt_size_sum)) * 8.0 / time_gap;
			printf("pkt_size_sum = %llu; time_gap = %llu; send rate = %.3lf Gbps; qp->m_rate = %llu\n",
				   (unsigned long long)pkt_size_sum,
				   (unsigned long long)time_gap,
				   tmp_rate,
				   (unsigned long long)qp->m_rate.GetBitRate());
			last_time = Simulator::Now().GetTimeStep();
			cnt = 0;
			pkt_size_sum = 0;
		}
	}
}

void RdmaHw::UpdateNextAvail(Ptr<RdmaQueuePair> qp, Time interframeGap, uint32_t pkt_size){
	Time sendingTime;
	if (m_rateBound)
		sendingTime =
			interframeGap + Seconds(qp->m_rate.CalculateTxTime(pkt_size));
	else
		sendingTime =
			interframeGap + Seconds(qp->m_max_rate.CalculateTxTime(pkt_size));
	qp->m_nextAvail = Simulator::Now() + sendingTime;
}

void RdmaHw::ChangeRate(Ptr<RdmaQueuePair> qp, DataRate new_rate){
	#if 1
	Time sendingTime =
		Seconds(qp->m_rate.CalculateTxTime(qp->lastPktSize));
	Time new_sendintTime =
		Seconds(new_rate.CalculateTxTime(qp->lastPktSize));
	qp->m_nextAvail =
		qp->m_nextAvail + new_sendintTime - sendingTime;
	uint32_t nic_idx = GetNicIdxOfQp(qp);
	m_nic[nic_idx].dev->UpdateNextAvail(qp->m_nextAvail);
	#endif

	qp->m_rate = new_rate;
}

#define PRINT_LOG 0

/******************************
 * Mellanox's version of DCQCN
 *****************************/
void RdmaHw::UpdateAlphaMlx(Ptr<RdmaQueuePair> q){
	if (q->mlx.m_alpha_cnp_arrived){
		q->mlx.m_alpha = (1 - m_g)*q->mlx.m_alpha + m_g;
	}else {
		q->mlx.m_alpha = (1 - m_g)*q->mlx.m_alpha;
	}
	q->mlx.m_alpha_cnp_arrived = false;
	ScheduleUpdateAlphaMlx(q);
}

void RdmaHw::ScheduleUpdateAlphaMlx(Ptr<RdmaQueuePair> q){
	q->mlx.m_eventUpdateAlpha =
		Simulator::Schedule(MicroSeconds(m_alpha_resume_interval),
							&RdmaHw::UpdateAlphaMlx, this, q);
}

void RdmaHw::cnp_received_mlx(Ptr<RdmaQueuePair> q){
	q->mlx.m_alpha_cnp_arrived = true;
	q->mlx.m_decrease_cnp_arrived = true;
	if (q->mlx.m_first_cnp){
		q->mlx.m_alpha = 1;
		q->mlx.m_alpha_cnp_arrived = false;
		ScheduleUpdateAlphaMlx(q);
		ScheduleDecreaseRateMlx(q, 1);
		q->mlx.m_targetRate =
			q->m_rate = m_rateOnFirstCNP * q->m_rate;
		q->mlx.m_first_cnp = false;
	}
}

void RdmaHw::CheckRateDecreaseMlx(Ptr<RdmaQueuePair> q){
	ScheduleDecreaseRateMlx(q, 0);
	if (q->mlx.m_decrease_cnp_arrived){
		bool clamp = true;
		if (!m_EcnClampTgtRate){
			if (q->mlx.m_rpTimeStage == 0)
				clamp = false;
		}
		if (clamp)
			q->mlx.m_targetRate = q->m_rate;

		q->m_rate =
			std::max(m_minRate, q->m_rate * (1 - q->mlx.m_alpha / 2));
		q->mlx.m_rpTimeStage = 0;
		q->mlx.m_decrease_cnp_arrived = false;
		Simulator::Cancel(q->mlx.m_rpTimer);
		q->mlx.m_rpTimer =
			Simulator::Schedule(MicroSeconds(m_rpgTimeReset),
								&RdmaHw::RateIncEventTimerMlx, this, q);
	}
}

void RdmaHw::ScheduleDecreaseRateMlx(Ptr<RdmaQueuePair> q, uint32_t delta){
	q->mlx.m_eventDecreaseRate =
		Simulator::Schedule(MicroSeconds(m_rateDecreaseInterval) +
							NanoSeconds(delta),
							&RdmaHw::CheckRateDecreaseMlx, this, q);
}

void RdmaHw::RateIncEventTimerMlx(Ptr<RdmaQueuePair> q){
	q->mlx.m_rpTimer =
		Simulator::Schedule(MicroSeconds(m_rpgTimeReset),
							&RdmaHw::RateIncEventTimerMlx, this, q);
	RateIncEventMlx(q);
	q->mlx.m_rpTimeStage++;
}

void RdmaHw::RateIncEventMlx(Ptr<RdmaQueuePair> q){
	if (q->mlx.m_rpTimeStage < m_rpgThreshold){
		FastRecoveryMlx(q);
	}else if (q->mlx.m_rpTimeStage == m_rpgThreshold){
		ActiveIncreaseMlx(q);
	}else {
		HyperIncreaseMlx(q);
	}
}

void RdmaHw::FastRecoveryMlx(Ptr<RdmaQueuePair> q){
	q->m_rate = (q->m_rate / 2) + (q->mlx.m_targetRate / 2);
}

void RdmaHw::ActiveIncreaseMlx(Ptr<RdmaQueuePair> q){
	uint32_t nic_idx = GetNicIdxOfQp(q);
	Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev;
	q->mlx.m_targetRate += m_rai;
	if (q->mlx.m_targetRate > dev->GetDataRate())
		q->mlx.m_targetRate = dev->GetDataRate();
	q->m_rate = (q->m_rate / 2) + (q->mlx.m_targetRate / 2);
}

void RdmaHw::HyperIncreaseMlx(Ptr<RdmaQueuePair> q){
	uint32_t nic_idx = GetNicIdxOfQp(q);
	Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev;
	q->mlx.m_targetRate += m_rhai;
	if (q->mlx.m_targetRate > dev->GetDataRate())
		q->mlx.m_targetRate = dev->GetDataRate();
	q->m_rate = (q->m_rate / 2) + (q->mlx.m_targetRate / 2);
}

/***********************
 * High Precision CC
 ***********************/
void RdmaHw::HandleAckHp(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch){
	uint32_t ack_seq = ch.ack.seq;
	if (ack_seq > qp->hp.m_lastUpdateSeq){
		UpdateRateHp(qp, p, ch, false);
	}else{
		FastReactHp(qp, p, ch);
	}
}

void RdmaHw::UpdateRateHp(Ptr<RdmaQueuePair> qp, Ptr<Packet> p,
						  CustomHeader &ch, bool fast_react){
	uint32_t next_seq = qp->snd_nxt;
	bool print = !fast_react || true;

	if (qp->hp.m_lastUpdateSeq == 0){
		qp->hp.m_lastUpdateSeq = next_seq;
		IntHeader &ih = ch.ack.ih;
		NS_ASSERT(ih.nhop <= IntHeader::maxHop);
		for (uint32_t i = 0; i < ih.nhop; i++)
			qp->hp.hop[i] = ih.hop[i];
	}else {
		IntHeader &ih = ch.ack.ih;
		if (ih.nhop <= IntHeader::maxHop){
			double max_c = 0;
			bool inStable = false;
			double U = 0;
			uint64_t dt = 0;
			bool updated[IntHeader::maxHop] = {false};
			bool updated_any = false;

			NS_ASSERT(ih.nhop <= IntHeader::maxHop);
			for (uint32_t i = 0; i < ih.nhop; i++){
				if (m_sampleFeedback){
					if (ih.hop[i].GetQlen() == 0 && fast_react)
						continue;
				}
				updated[i] = updated_any = true;

				uint64_t tau = ih.hop[i].GetTimeDelta(qp->hp.hop[i]);
				double duration = tau * 1e-9;
				double txRate =
					(ih.hop[i].GetBytesDelta(qp->hp.hop[i])) *
					8 / duration;
				double u =
					txRate / ih.hop[i].GetLineRate() +
					(double)std::min(ih.hop[i].GetQlen(),
									 qp->hp.hop[i].GetQlen()) *
					qp->m_max_rate.GetBitRate() /
					ih.hop[i].GetLineRate() /
					qp->m_win;

				if (!m_multipleRate){
					if (u > U){
						U = u;
						dt = tau;
					}
				}else {
					if (tau > qp->m_baseRtt)
						tau = qp->m_baseRtt;
					qp->hp.hopState[i].u =
						(qp->hp.hopState[i].u *
						 (qp->m_baseRtt - tau) +
						 u * tau) /
						double(qp->m_baseRtt);
				}
				qp->hp.hop[i] = ih.hop[i];
			}

			DataRate new_rate;
			int32_t new_incStage;
			DataRate new_rate_per_hop[IntHeader::maxHop];
			int32_t new_incStage_per_hop[IntHeader::maxHop];

			if (!m_multipleRate){
				if (updated_any){
					if (dt > qp->m_baseRtt)
						dt = qp->m_baseRtt;
					qp->hp.u =
						(qp->hp.u * (qp->m_baseRtt - dt) +
						 U * dt) /
						double(qp->m_baseRtt);
					max_c = qp->hp.u / m_targetUtil;

					if (max_c >= 1 ||
						qp->hp.m_incStage >= m_miThresh){
						new_rate =
							qp->hp.m_curRate / max_c + m_rai;
						new_incStage = 0;
					}else{
						new_rate = qp->hp.m_curRate + m_rai;
						new_incStage = qp->hp.m_incStage + 1;
					}

					if (new_rate < m_minRate)
						new_rate = m_minRate;
					if (new_rate > qp->m_max_rate)
						new_rate = qp->m_max_rate;
				}
			}else{
				new_rate = qp->m_max_rate;
				for (uint32_t i = 0; i < ih.nhop; i++){
					if (updated[i]){
						double c =
							qp->hp.hopState[i].u / m_targetUtil;

						if (c >= 1 ||
							qp->hp.hopState[i].incStage >= m_miThresh){
							new_rate_per_hop[i] =
								qp->hp.hopState[i].Rc / c + m_rai;
							new_incStage_per_hop[i] = 0;
						}else{
							new_rate_per_hop[i] =
								qp->hp.hopState[i].Rc + m_rai;
							new_incStage_per_hop[i] =
								qp->hp.hopState[i].incStage + 1;
						}

						if (new_rate_per_hop[i] < m_minRate)
							new_rate_per_hop[i] = m_minRate;
						if (new_rate_per_hop[i] > qp->m_max_rate)
							new_rate_per_hop[i] = qp->m_max_rate;

						if (new_rate_per_hop[i] < new_rate)
							new_rate = new_rate_per_hop[i];
					}else{
						if (qp->hp.hopState[i].Rc < new_rate)
							new_rate = qp->hp.hopState[i].Rc;
					}
				}
			}

			if (updated_any)
				ChangeRate(qp, new_rate);

			if (!fast_react){
				if (updated_any){
					qp->hp.m_curRate = new_rate;
					qp->hp.m_incStage = new_incStage;
				}
				if (m_multipleRate){
					for (uint32_t i = 0; i < ih.nhop; i++){
						if (updated[i]){
							qp->hp.hopState[i].Rc =
								new_rate_per_hop[i];
							qp->hp.hopState[i].incStage =
								new_incStage_per_hop[i];
						}
					}
				}
			}
		}

		if (!fast_react){
			if (next_seq > qp->hp.m_lastUpdateSeq)
				qp->hp.m_lastUpdateSeq = next_seq;
		}
	}
}

void RdmaHw::FastReactHp(Ptr<RdmaQueuePair> qp, Ptr<Packet> p,
						 CustomHeader &ch){
	if (m_fast_react)
		UpdateRateHp(qp, p, ch, true);
}

/**********************
 * TIMELY
 *********************/
void RdmaHw::HandleAckTimely(Ptr<RdmaQueuePair> qp, Ptr<Packet> p,
							 CustomHeader &ch){
	uint32_t ack_seq = ch.ack.seq;
	if (ack_seq > qp->tmly.m_lastUpdateSeq){
		UpdateRateTimely(qp, p, ch, false);
	}else{
		FastReactTimely(qp, p, ch);
	}
}

void RdmaHw::UpdateRateTimely(Ptr<RdmaQueuePair> qp, Ptr<Packet> p,
							  CustomHeader &ch, bool us){
	uint32_t next_seq = qp->snd_nxt;
	uint64_t rtt =
		Simulator::Now().GetTimeStep() - ch.ack.ih.ts;

	if (qp->tmly.m_lastUpdateSeq != 0){
		int64_t new_rtt_diff =
			(int64_t)rtt - (int64_t)qp->tmly.lastRtt;
		double rtt_diff =
			(1 - m_tmly_alpha) * qp->tmly.rttDiff +
			m_tmly_alpha * new_rtt_diff;
		double gradient = rtt_diff / m_tmly_minRtt;

		bool inc = false;
		double c = 0;

		if (rtt < m_tmly_TLow){
			inc = true;
		}else if (rtt > m_tmly_THigh){
			c =
				1 - m_tmly_beta *
				(1 - (double)m_tmly_THigh / rtt);
			inc = false;
		}else if (gradient <= 0){
			inc = true;
		}else{
			c = 1 - m_tmly_beta * gradient;
			if (c < 0)
				c = 0;
			inc = false;
		}

		if (inc){
			if (qp->tmly.m_incStage < 5){
				qp->m_rate = qp->tmly.m_curRate + m_rai;
			}else{
				qp->m_rate = qp->tmly.m_curRate + m_rhai;
			}

			if (qp->m_rate > qp->m_max_rate)
				qp->m_rate = qp->m_max_rate;

			if (!us){
				qp->tmly.m_curRate = qp->m_rate;
				qp->tmly.m_incStage++;
				qp->tmly.rttDiff = rtt_diff;
			}
		}else{
			qp->m_rate =
				std::max(m_minRate, qp->tmly.m_curRate * c);
			if (!us){
				qp->tmly.m_curRate = qp->m_rate;
				qp->tmly.m_incStage = 0;
				qp->tmly.rttDiff = rtt_diff;
			}
		}
	}

	if (!us && next_seq > qp->tmly.m_lastUpdateSeq){
		qp->tmly.m_lastUpdateSeq = next_seq;
		qp->tmly.lastRtt = rtt;
	}
}

void RdmaHw::FastReactTimely(Ptr<RdmaQueuePair> qp, Ptr<Packet> p,
							 CustomHeader &ch){
}

/**********************
 * DCTCP
 *********************/
void RdmaHw::HandleAckDctcp(Ptr<RdmaQueuePair> qp, Ptr<Packet> p,
							CustomHeader &ch){
	uint32_t ack_seq = ch.ack.seq;
	uint8_t cnp =
		(ch.ack.flags >> qbbHeader::FLAG_CNP) & 1;
	bool new_batch = false;

	qp->dctcp.m_ecnCnt += (cnp > 0);

	if (ack_seq > qp->dctcp.m_lastUpdateSeq){
		new_batch = true;

		if (qp->dctcp.m_lastUpdateSeq == 0){
			qp->dctcp.m_lastUpdateSeq = qp->snd_nxt;
			qp->dctcp.m_batchSizeOfAlpha =
				qp->snd_nxt / m_mtu + 1;
		}else {
			double frac =
				std::min(1.0,
						 double(qp->dctcp.m_ecnCnt) /
						 qp->dctcp.m_batchSizeOfAlpha);
			qp->dctcp.m_alpha =
				(1 - m_g) * qp->dctcp.m_alpha +
				m_g * frac;
			qp->dctcp.m_lastUpdateSeq = qp->snd_nxt;
			qp->dctcp.m_ecnCnt = 0;
			qp->dctcp.m_batchSizeOfAlpha =
				(qp->snd_nxt - ack_seq) / m_mtu + 1;
		}
	}

	if (qp->dctcp.m_caState == 1){
		if (ack_seq > qp->dctcp.m_highSeq)
			qp->dctcp.m_caState = 0;
	}

	if (cnp && qp->dctcp.m_caState == 0){
		qp->m_rate =
			std::max(m_minRate,
					 qp->m_rate *
					 (1 - qp->dctcp.m_alpha / 2));
		qp->dctcp.m_caState = 1;
		qp->dctcp.m_highSeq = qp->snd_nxt;
	}

	if (qp->dctcp.m_caState == 0 && new_batch)
		qp->m_rate =
			std::min(qp->m_max_rate,
					 qp->m_rate + m_dctcp_rai);
}

/*********************
 * HPCC-PINT
 ********************/
void RdmaHw::SetPintSmplThresh(double p){
	pint_smpl_thresh = (uint32_t)(65536 * p);
}

void RdmaHw::HandleAckHpPint(Ptr<RdmaQueuePair> qp, Ptr<Packet> p,
							 CustomHeader &ch){
	uint32_t ack_seq = ch.ack.seq;
	if (rand() % 65536 >= pint_smpl_thresh)
		return;

	if (ack_seq > qp->hpccPint.m_lastUpdateSeq){
		UpdateRateHpPint(qp, p, ch, false);
	}else{
		UpdateRateHpPint(qp, p, ch, true);
	}
}

void RdmaHw::UpdateRateHpPint(Ptr<RdmaQueuePair> qp, Ptr<Packet> p,
							  CustomHeader &ch, bool fast_react){
	uint32_t next_seq = qp->snd_nxt;

	if (qp->hpccPint.m_lastUpdateSeq == 0){
		qp->hpccPint.m_lastUpdateSeq = next_seq;
	}else {
		IntHeader &ih = ch.ack.ih;
		double U = Pint::decode_u(ih.GetPower());

		DataRate new_rate;
		int32_t new_incStage;
		double max_c = U / m_targetUtil;

		if (max_c >= 1 ||
			qp->hpccPint.m_incStage >= m_miThresh){
			new_rate =
				qp->hpccPint.m_curRate / max_c + m_rai;
			new_incStage = 0;
		}else{
			new_rate =
				qp->hpccPint.m_curRate + m_rai;
			new_incStage =
				qp->hpccPint.m_incStage + 1;
		}

		if (new_rate < m_minRate)
			new_rate = m_minRate;
		if (new_rate > qp->m_max_rate)
			new_rate = qp->m_max_rate;

		ChangeRate(qp, new_rate);

		if (!fast_react){
			qp->hpccPint.m_curRate = new_rate;
			qp->hpccPint.m_incStage = new_incStage;
		}

		if (!fast_react){
			if (next_seq > qp->hpccPint.m_lastUpdateSeq)
				qp->hpccPint.m_lastUpdateSeq = next_seq;
		}
	}
}


/******************************
 * FPDCC
 *****************************/
int RdmaHw::ReceivePnp(Ptr<Packet> p, CustomHeader &ch){
	if (m_cc_mode != 12){
		return 0;
	}
	const uint16_t flowId = ch.pnp.flowid;
	const uint16_t qid = ch.pnp.qid;
	
	Ptr<RdmaQueuePair> qp = GetQp(ch.sip, flowId, qid); 
	
	if (qp == NULL || qp->IsFinished()){
		return 0;
	}

	HandlePnpFpdcc(qp, p, ch); 
	return 0;
	
}

void RdmaHw::HandlePnpFpdcc(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch){
	const uint32_t swid = ch.pnp.swid;
	
	// Construct this PNP record
	const fpdcc_pnp_sample sample = {
		ch.pnp.ts,
		ch.pnp.qlen,
		ch.pnp.A,
		ch.pnp.W,
		ch.pnp.bw,
		ch.pnp.Z,
		ch.pnp.seq
	};
	
	auto it = qp->fpdcc.pnpRecords.find(swid);
	
	// If it does not exist, create a switch record and use the current PNP as the first baseline packet
	if (it == qp->fpdcc.pnpRecords.end()){
		fpdcc_switch_record record;
		record.samples.push_back(sample);    
		auto inserted = qp->fpdcc.pnpRecords.emplace(swid, record);   
		ResetFpdccSwitchValidityTimer(qp, swid, inserted.first->second);    //Start a 5RTT with a valid time limit
		return;
	}
	
	fpdcc_switch_record &record = it->second;
	
	if (record.samples.empty()){
		record.samples.push_back(sample);
		ResetFpdccSwitchValidityTimer(qp, swid, record);
		return;
	}

	if (sample.ts <= record.samples.back().ts ){
		return;
	}
	
	if (sample.ts - record.samples.back().ts <=  2000){
		return;
	}

	if (record.samples.size() == 2){
		record.samples.pop_front();
	}

	record.samples.push_back(sample);
	
	//Two valid PNP updates H
	UpdateFpdccSwitchMeasurement(record);

	//The current PNP has become a new valid measurement sample, and the 5 * BaseRTT validity period has been restarted
	ResetFpdccSwitchValidityTimer(qp, swid, record);
	
	TryFpdccRateAdjustment(qp, swid);
}

void RdmaHw::UpdateFpdccSwitchMeasurement(fpdcc_switch_record &record){
	if (record.samples.size() != 2){
		record.H = -1.0;
		return;
	}

	const fpdcc_pnp_sample &s1 = record.samples[0];
	const fpdcc_pnp_sample &s2 = record.samples[1];

	const uint64_t deltaTs = s2.ts - s1.ts;
	const uint64_t deltaA = s2.A - s1.A;

	const double rin = static_cast<double>(deltaA) * 8.0 * 1e9 / static_cast<double>(deltaTs);   //Entry rate

	const double bottleneckBw = static_cast<double>(s2.bw);  

	record.H = rin / bottleneckBw;   //Record the current bottleneck
}

bool RdmaHw::IsFpdccMainBottleneck( Ptr<RdmaQueuePair> qp, uint32_t currentSwid) const{ //Determine the maximum bottleneck

	auto currentIt = qp->fpdcc.pnpRecords.find(currentSwid);  
	if (currentIt == qp->fpdcc.pnpRecords.end()){
		return false;
	}  

	const fpdcc_switch_record &current = currentIt->second;
	if (current.samples.size() != 2 ||
		!std::isfinite(current.H) || current.H < 0.0 ){
		return false;
	}
	
	//entry. first is the switch number, entry. second is the record of the switch	
	for (const auto &entry : qp->fpdcc.pnpRecords){
		if (entry.first == currentSwid){
			continue;
		}

		const fpdcc_switch_record &other = entry.second;
		if (other.samples.size() != 2 ||
			!std::isfinite(other.H) || other.H < 0.0 ){
			continue;
		}

		if (other.H > current.H ||  (other.H == current.H && entry.first < currentSwid) ){
			return false;
		}
	}

	return true;
}

double RdmaHw::CalculateFpdccNewRate( Ptr<RdmaQueuePair> qp, const fpdcc_switch_record &record) const{
	if (record.samples.size() != 2){
		return -1.0;
	}
	
	const fpdcc_pnp_sample &s1 = record.samples[0];
	const fpdcc_pnp_sample &s2 = record.samples[1];
	const fpdcc_pnp_sample &latest = s2;
	const double H = record.H;
	
	const uint64_t deltaA = s2.A - s1.A;
	const uint64_t deltaW = s2.W - s1.W;
	if (deltaA == 0){
		return -1.0;
	}
	
	const double P = static_cast<double>(deltaW) / (255.0 * static_cast<double>(deltaA));  //P
	
	const double bottleneckBw = static_cast<double>(latest.bw); // B, bit/s
	const double rin = H * bottleneckBw;                        // Rin, bit/s
	const double oldRate = static_cast<double>(qp->m_rate.GetBitRate());       
	const double flowPersistence =   static_cast<double>(CalculatePersistenceRaw(qp)) / 255.0; // p
	const double targetQueue = static_cast<double>(m_fpdccK1); // Qt, Byte 
	const double queueBytes = static_cast<double>(latest.qlen);   // Q, Byte 
	
	const int64_t deltaQ = static_cast<int64_t>(s2.qlen) - static_cast<int64_t>(s1.qlen);
	
	// slow down
	if (H >= 1.0 &&  deltaQ > 0){
	    //Predict the queue depth of the current switch
	     const double queueGrowth = (rin - bottleneckBw) *  static_cast<double>(qp->m_baseRtt) * 1e-9/ 8.0;
	     double currentQueue = queueBytes + queueGrowth; 
		
	     const double R_alter = (rin - bottleneckBw) + 8.0 * (currentQueue - targetQueue) / (static_cast<double>(qp->m_baseRtt) * 1e-9 * 2);   // *2
	      
	     // Continuous load in the network
             const double persistentLoad = rin * P;   // Rin * P = Σ(Rj * pj)
	      
	     double newRate = oldRate;
	     
	     //If the total reduction amount does not exceed the continuous load, all reductions will be allocated according to the continuous responsibility of Ri * pi
	     if (R_alter <= persistentLoad){
	      
	      	const double flowReduction =  R_alter *  (oldRate * flowPersistence) /  persistentLoad; 
	      	newRate = oldRate - flowReduction;
	      	
	     }else {
	      
	        const double remainingAlter =  R_alter - persistentLoad;

                const double nonPersistentLoad = rin * (1.0 - P);

                // first
                const double persistentReduction = oldRate * flowPersistence;

                // second
                const double extraReduction = remainingAlter * (oldRate * (1.0 - flowPersistence)) / nonPersistentLoad;

                newRate = oldRate - persistentReduction - extraReduction;
	      }
	       
	      //When the rate is negative, it becomes 1% of the line speed
	      const double minRate = 0.01 * static_cast<double>(qp->m_max_rate.GetBitRate());

              if (newRate < minRate){
                 newRate = minRate;}

              return newRate;
	}
	
	//Speed up
	if(H < 1.0 &&  deltaQ <= 0){
           const double queueGrowth = (bottleneckBw - rin) *  static_cast<double>(qp->m_baseRtt) * 1e-9/ 8.0;
	   double currentQueue = queueBytes - queueGrowth;   
	   if(currentQueue < 0)
	   {
	   	currentQueue = 0;
	   }
	   
	   if(currentQueue > targetQueue)
	   {
	   	return oldRate;
	   }
	   
	    const double R_alter = (bottleneckBw - rin) + 8.0 * (targetQueue - currentQueue) / (static_cast<double>(qp->m_baseRtt) * 1e-9 * 2);
	    
	    double newRate = oldRate;
	    
	    if (P == 1.0){
   		 newRate = oldRate + R_alter * oldRate / rin;
	    }else{
	    
	    const double nonPersistentLoad = rin * (1.0 - P);
	    
	    newRate = oldRate + R_alter * oldRate * (1.0 - flowPersistence) / nonPersistentLoad;
	    }
	    
	    if(newRate >= static_cast<double>(qp->m_max_rate.GetBitRate()) )
	    {
	       newRate =  static_cast<double>(qp->m_max_rate.GetBitRate());
	    }
	    return newRate;
	}
	
	return oldRate;
}

void RdmaHw::TryFpdccRateAdjustment( Ptr<RdmaQueuePair> qp, uint32_t currentSwid){
	auto it = qp->fpdcc.pnpRecords.find(currentSwid);  
	if (it == qp->fpdcc.pnpRecords.end()){
		return;
	}

	fpdcc_switch_record &record = it->second;   //Obtain the actual records of the switch, which include two PNP and H
	
	if (record.samples.size() != 2 || !std::isfinite(record.H) || record.H < 0.0 ){
		return;
	}


	const fpdcc_pnp_sample &s1 = record.samples[0];
	const fpdcc_pnp_sample &s2 = record.samples[1];
	
	const int64_t deltaQ = static_cast<int64_t>(s2.qlen) - static_cast<int64_t>(s1.qlen);
	
	const uint64_t oldRate = qp->m_rate.GetBitRate();
	
	const bool shouldDecrease = (record.H >= 1.0 && deltaQ > 0);

	const bool shouldIncrease = (record.H < 1.0 && deltaQ <= 0);
	
	if (!shouldDecrease && !shouldIncrease){
		return;
	}
	
	//Identify the main bottleneck
	if (!IsFpdccMainBottleneck(qp, currentSwid)){
		return;
	}

	const uint32_t latestPsn = s2.seq;
	const bool psnFresh = latestPsn > qp->fpdcc.m_lastRecordPsn;   //Determine if PSN is valid
	
	// If the last real action was rate increase,
	// and explicit congestion is currently detected:
	//
	//     H >= 1 && ΔQ > 0
	//
	// then congestion is allowed to preempt immediately.
	const bool congestionPreempt =
		qp->fpdcc.m_lastAppliedH >= 0.0 &&
		qp->fpdcc.m_lastAppliedH < 1.0 &&
		shouldDecrease;

	if (!psnFresh && !congestionPreempt){
		return;
	}
	
	//rate adjustment
	const double calculatedRate = CalculateFpdccNewRate(qp, record);
	const DataRate newRate(calculatedRate);
	
	//If calculated rate equals current rate, no real rate adjustment occurs, do not update PSN.
	if (newRate.GetBitRate() == oldRate){
           return;
	}
	
	//Clear the information maintained by this switch after rate modification.
	ChangeRate(qp, DataRate(calculatedRate));
	

	// Record the adopted primary bottleneck H for later check of "immediate congestion preemption after rate increase".
	qp->fpdcc.m_lastAppliedH = record.H;;

	//Set current snd_nxt as new PSN control boundary.
	qp->fpdcc.m_lastRecordPsn = static_cast<uint32_t>(qp->snd_nxt);
	
	Simulator::Cancel(record.validityEvent);
	record.samples.clear();
	   
	   record.H = -1.0;
}
void RdmaHw::ResetFpdccSwitchValidityTimer(
	Ptr<RdmaQueuePair> qp,
	uint32_t swid,
	fpdcc_switch_record &record){  //Reset validity of the switch record.

	Simulator::Cancel(record.validityEvent);
	record.validityEvent = Simulator::Schedule(
		NanoSeconds(5 * qp->m_baseRtt),
		&RdmaHw::InvalidateFpdccSwitchRecord,
		this,
		qp,
		swid);
}

void RdmaHw::InvalidateFpdccSwitchRecord(Ptr<RdmaQueuePair> qp, uint32_t swid){   //Timer expires, switch record becomes invalid.
	if (qp == NULL || qp->IsFinished()){
		return;
	}

	auto it = qp->fpdcc.pnpRecords.find(swid);
	if (it == qp->fpdcc.pnpRecords.end()){
		return;
	}

	fpdcc_switch_record &record = it->second;
	record.H = -1.0;
	record.samples.clear();
	
	
	for (const auto &entry : qp->fpdcc.pnpRecords){
		if (!entry.second.samples.empty()){
			return;
		}
	}

	const uint64_t oldRate = qp->m_rate.GetBitRate();
	const uint64_t lineRate = qp->m_max_rate.GetBitRate();
	
	const uint64_t recoveryRate = static_cast<uint64_t>( static_cast<double>(lineRate));  //400
	
	if (oldRate >= recoveryRate){
		return;
	}

	ChangeRate(qp, DataRate(recoveryRate));
	qp->fpdcc.m_lastAppliedH = 0.0;
	qp->fpdcc.m_lastRecordPsn = static_cast<uint32_t>(qp->snd_nxt);
}


/******************************
 * ENCC
 *****************************/
int RdmaHw::ReceiveEnp(Ptr<Packet> p, CustomHeader &ch){
	uint16_t qIndex = ch.enp.qidx;
	uint16_t port = ch.enp.flowid;
	uint32_t seq = ch.enp.seq;
	int i;

	Ptr<RdmaQueuePair> qp =
		GetQp(ch.sip, port, qIndex);
	if (qp == NULL) {
		return 0;
	}

	HandleEnpEncc(qp, p, ch);
	return 0;
}

void RdmaHw::HandleEnpEncc(Ptr<RdmaQueuePair> qp,
						   Ptr<Packet> p,
						   CustomHeader &ch){
	uint32_t seq = ch.enp.seq;
	uint64_t ts = ch.enp.ts;

	if ((seq > qp->encc.m_lastRecordPsn) &&
		(ts - qp->encc.m_lastEnpTime > m_rtt*2)) {

		double incast =
			MeasureIncastEncc(qp, ch);
		UpdateRateEncc(qp, ch, incast);

		if (incast > 0)
			qp->encc.last_recored[
				ch.enp.swid].lastIncastDegree =
					incast;

		qp->encc.last_recored[
			ch.enp.swid].lastQLen =
				ch.enp.qlen;
		qp->encc.last_recored[
			ch.enp.swid].lastTs =
				ch.enp.ts;

		qp->encc.m_lastRecordPsn =
			qp->snd_nxt;
		qp->encc.m_lastEnpTime =
			Simulator::Now().GetTimeStep();

		SetTimerEncc(qp, ch);
	}
}

double RdmaHw::MeasureIncastEncc(
	Ptr<RdmaQueuePair> qp,
	CustomHeader &ch){

	double v_max = -1;
	double incast_degree = -1;
	double bw =
		ch.enp.bw /
		(8.0 * 1000000000.0);
	uint16_t idx_max = 0;
	uint64_t cur_time =
		Simulator::Now().GetTimeStep();

	for (auto it =
			 qp->encc.last_recored.begin();
		 it != qp->encc.last_recored.end();
		 ++it) {

		if ((cur_time - it->second.lastTs) <
			m_validity_period) {

			if (it->second.lastIncastDegree >
				v_max) {
				v_max =
					it->second.lastIncastDegree;
				idx_max = it->first;
			}
		}
	}

	for (auto it =
			 qp->encc.last_recored.begin();
		 it != qp->encc.last_recored.end();
		 ++it) {

		if ((it->first == ch.enp.swid) &&
			((cur_time - it->second.lastTs) <
			 m_validity_period)) {

			incast_degree =
				1 +
				(static_cast<double>(
					 ch.enp.qlen) -
				 static_cast<double>(
					 it->second.lastQLen)) /
				((ch.enp.ts -
				  it->second.lastTs) *
				 bw);

			if (incast_degree > v_max ||
				it->first == idx_max) {

				uint64_t tau;
				tau =
					ch.enp.ts -
					it->second.lastTs >
					m_rtt
					? m_rtt
					: ch.enp.ts -
					  it->second.lastTs;

				double w =
					static_cast<double>(tau) /
					m_rtt;

				incast_degree =
					(1-w) *
					it->second.lastIncastDegree +
					w * incast_degree;
			}
		}
	}

	return incast_degree;
}

void RdmaHw::UpdateRateEncc(
	Ptr<RdmaQueuePair> qp,
	CustomHeader &ch,
	double incast_degree){

	DataRate new_rate;
	double r_ai;
	double bw =
		ch.enp.bw /
		1000000000.0 / 8;

	double qlen_target =
		bw * m_rtt;

	uint32_t nic_idx =
		GetNicIdxOfQp(qp);
	DataRate m_bps =
		m_nic[nic_idx].dev->GetDataRate();

	r_ai =
		1000000000.0 *
		(qlen_target - ch.enp.qlen) /
		(m_encc_stage *
		 m_rtt *
		 incast_degree);

	if (incast_degree > 0) {
		new_rate =
			qp->m_rate /
			incast_degree;

		if (r_ai >= 0) {
			new_rate =
				new_rate + r_ai;
		}else{
			if ((r_ai*(-1)) >
				new_rate.GetBitRate())
				new_rate =
					DataRate("1Mb/s");
			else
				new_rate =
					new_rate + r_ai;
		}

		ChangeRate(
			qp,
			std::min(new_rate, m_bps));
	}
}

void RdmaHw::SetTimerEncc(
	Ptr<RdmaQueuePair> qp,
	CustomHeader &ch){

	Simulator::Cancel(
		qp->encc.m_eventTimer);

	uint32_t nic_idx =
		GetNicIdxOfQp(qp);
	DataRate m_bps =
		m_nic[nic_idx].dev->GetDataRate();

	uint64_t cur_time =
		Simulator::Now().GetTimeStep();

	if (qp->m_rate < m_bps &&
		(cur_time -
		 qp->encc.m_lastEnpTime >
		 m_rtt)) {
		ChangeRate(qp, m_bps);
	}

	qp->encc.m_eventTimer =
		Simulator::Schedule(
			NanoSeconds(m_rtt*5),
			&RdmaHw::SetTimerEncc,
			this,
			qp,
			ch);
}

}


