// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-        

#include "switch_htsim.h"
#include "eth_pause_packet.h"
#include "queue_lossless.h"
#include "queue_lossless_input.h"

void Switch_htsim::sendPause(LosslessQueue* problem, unsigned int wait){
    cout << "Switch_htsim " << _name << " link " << problem->_name << " pause " << wait << endl;

    for (list<Queue*>::iterator it=_ports.begin(); it != _ports.end(); ++it){
	LosslessQueue* q = (LosslessQueue*)*it;

	if (q==problem)
	    continue;

	cout << "Informing " << q->_name << endl;
	EthPausePacket* pkt = EthPausePacket::newpkt(wait);
	q->getRemoteEndpoint()->receivePacket(*pkt);
    }
};

void Switch_htsim::configureLossless(){
    for (list<Queue*>::iterator it=_ports.begin(); it != _ports.end(); ++it){
	LosslessQueue* q = (LosslessQueue*)*it;
	q->setSwitch(this);
	q->initThresholds();
    }
};
/*Switch_htsim::configureLosslessInput(){
    for (list<Queue*>::iterator it=_ports.begin(); it != _ports.end(); ++it){
	LosslessInputQueue* q = (LosslessInputQueue*)*it;
	q->setSwitch(this);
	q->initThresholds();
    }
    };*/

