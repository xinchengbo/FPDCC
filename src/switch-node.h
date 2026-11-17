#ifndef SWITCH_NODE_H
#define SWITCH_NODE_H

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include <ns3/node.h>

#include "qbb-net-device.h"
#include "switch-mmu.h"
#include "pint.h"

namespace ns3 {

class Packet;

class SwitchNode : public Node {
	static const uint32_t pCnt = 257;
	static const uint32_t qCnt = 8;

	enum class CcNetState : uint8_t {
		IN = 0,  
		CN = 1,  
		NN = 2    
	};

	struct CcQueueState {
		// A
		uint64_t inBytes;

		// W
		uint64_t weightedBytes;

		uint64_t lastInBytes;

		CcNetState netState;
		
		double feedbackProb;

		CcQueueState()
			: inBytes(0),
			  weightedBytes(0),
			  lastInBytes(0),
			  netState(CcNetState::NN),
			  feedbackProb(0.0) {}
	};

	uint32_t m_ecmpSeed;
	std::unordered_map<uint32_t, std::vector<int> > m_rtTable;

	// m_bytes[inDev][outDev][qIndex]
	uint32_t m_bytes[pCnt][pCnt][qCnt];

	uint64_t m_txBytes[pCnt];

	uint32_t m_lastPktSize[pCnt];
	uint64_t m_lastPktTs[pCnt];
	double m_u[pCnt];

	CcQueueState m_ccState[pCnt][qCnt];

	//Te
	uint64_t m_ccNs;    

protected:
	bool m_ecnEnabled;
	uint32_t m_ccMode;
	uint64_t m_maxRtt;
	uint32_t m_ackHighPrio;

private:
	int GetOutDev(Ptr<const Packet> p, CustomHeader &ch);
	void SendToDev(Ptr<Packet> p, CustomHeader &ch);
	static uint32_t EcmpHash(const uint8_t *key, size_t len, uint32_t seed);

	void CheckAndSendPfc(uint32_t inDev, uint32_t qIndex);
	void CheckAndSendResume(uint32_t inDev, uint32_t qIndex);

	void UpdateCcState(void);

	CcNetState DetermineCcState(double pressure,
						uint64_t queueBytes,
						uint64_t k1) const;

	double CalcCcProb(CcNetState state,
					  double pressure,
					  uint64_t queueBytes,
					  uint64_t k1,
					  uint64_t k2) const;

	bool ShouldSendPnp(uint32_t ifIndex, uint32_t qIndex) const;

public:
	Ptr<SwitchMmu> m_mmu;

	static TypeId GetTypeId(void);
	SwitchNode();

	void SetEcmpSeed(uint32_t seed);
	void AddTableEntry(Ipv4Address &dstAddr, uint32_t intf_idx);
	void ClearTable();

	bool SwitchReceiveFromDevice(Ptr<NetDevice> device,
								 Ptr<Packet> packet,
								 CustomHeader &ch);
	void SwitchNotifyDequeue(uint32_t ifIndex,
							 uint32_t qIndex,
							 Ptr<Packet> p);

	Ptr<Packet> GenEnp(Ptr<Packet> ori_pkt,
					   CustomHeader &ch,
					   uint32_t qlen,
					   uint64_t bw);

	Ptr<Packet> GenPnp(Ptr<Packet> ori_pkt,
					   CustomHeader &ch,
					   uint32_t qlen,
					   uint64_t A,
					   uint64_t W,
					   uint64_t bw);

	int logres_shift(int b, int l);
	int log2apprx(int x, int b, int m, int l);

	void StartCc(void); 
};

} // namespace ns3

#endif /* SWITCH_NODE_H */



