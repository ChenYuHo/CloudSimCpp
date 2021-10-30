// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-        
#include "oversubscribed_fat_tree_topology.h"
#include <vector>
#include "string.h"
#include <sstream>
#include <strstream>
#include <iostream>
#include "main.h"
#include "queue.h"
#include "compositequeue.h"
#include "ecnqueue.h"

string ntoa(double n);
string itoa(uint64_t n);

extern int N;

OversubscribedFatTreeTopology::OversubscribedFatTreeTopology(Logfile* lg, EventList* ev,FirstFit * fit,queue_type q){
    logfile = lg;
    eventlist = ev;
    ff = fit;
  
    _no_of_nodes = K * K * K;
  
    qt = q;

    init_network();
}

Queue* OversubscribedFatTreeTopology::alloc_src_queue(QueueLogger* queueLogger){
    return  new PriorityQueue(speedFromMbps((uint64_t)HOST_NIC), memFromPkt(FEEDER_BUFFER), *eventlist, queueLogger);
}

Queue* OversubscribedFatTreeTopology::alloc_queue(QueueLogger* queueLogger){
    return alloc_queue(queueLogger, HOST_NIC);
}

Queue* OversubscribedFatTreeTopology::alloc_queue(QueueLogger* queueLogger, uint64_t speed){
    if (qt==RANDOM)
	return new RandomQueue(speedFromMbps(speed), memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER), *eventlist, queueLogger, memFromPkt(RANDOM_BUFFER));
    else if (qt==COMPOSITE)
	return new CompositeQueue(speedFromMbps(speed), memFromPkt(8), *eventlist, queueLogger);
    else if (qt==ECN)
	return new ECNQueue(speedFromMbps(speed), memFromPkt(2*SWITCH_BUFFER), *eventlist, queueLogger, memFromPkt(15));
    assert(0);
}

void OversubscribedFatTreeTopology::init_network(){
    QueueLoggerSampling* queueLogger;

    for (int j=0;j<NC;j++)
	for (int k=0;k<NK;k++){
	    queues_nc_nup[j][k] = NULL;
	    pipes_nc_nup[j][k] = NULL;
	    queues_nup_nc[k][j] = NULL;
	    pipes_nup_nc[k][j] = NULL;
	}

    for (int j=0;j<NK;j++)
	for (int k=0;k<NK;k++){
	    queues_nup_nlp[j][k] = NULL;
	    pipes_nup_nlp[j][k] = NULL;
	    queues_nlp_nup[k][j] = NULL;
	    pipes_nlp_nup[k][j] = NULL;
	}
  
    for (int j=0;j<NK;j++)
	for (int k=0;k<NSRV;k++){
	    queues_nlp_ns[j][k] = NULL;
	    pipes_nlp_ns[j][k] = NULL;
	    queues_ns_nlp[k][j] = NULL;
	    pipes_ns_nlp[k][j] = NULL;
	}

    // lower layer pod switch to server
    for (int j = 0; j < NK; j++) {
	for (int l = 0; l < 2*K; l++) {
	    int k = j * 2*K + l;
	    // Downlink
	    queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
	    //queueLogger = NULL;
	    logfile->addLogger(*queueLogger);

	    queues_nlp_ns[j][k] = alloc_queue(queueLogger);
	    queues_nlp_ns[j][k]->setName("LS_" + ntoa(j) + "-" + "DST_" +ntoa(k));
	    logfile->writeName(*(queues_nlp_ns[j][k]));
	  
	    pipes_nlp_ns[j][k] = new Pipe(timeFromUs(RTT), *eventlist);
	    pipes_nlp_ns[j][k]->setName("Pipe-nt-ns-" + ntoa(j) + "-" + ntoa(k));
	    logfile->writeName(*(pipes_nlp_ns[j][k]));
	  
	    // Uplink
	    queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
	    logfile->addLogger(*queueLogger);
	    queues_ns_nlp[k][j] = alloc_src_queue(queueLogger);
	    queues_ns_nlp[k][j]->setName("SRC_" + ntoa(k) + "-" + "LS_"+ntoa(j));
	    logfile->writeName(*(queues_ns_nlp[k][j]));
	  
	    pipes_ns_nlp[k][j] = new Pipe(timeFromUs(RTT), *eventlist);
	    pipes_ns_nlp[k][j]->setName("Pipe-ns-nt-" + ntoa(k) + "-" + ntoa(j));
	    logfile->writeName(*(pipes_ns_nlp[k][j]));
	  
	    if (ff){
		ff->add_queue(queues_nlp_ns[j][k]);
		ff->add_queue(queues_ns_nlp[k][j]);
	    }
	}
    }

    /*    for (int i = 0;i<NSRV;i++){
	  for (int j = 0;j<NK;j++){
	  myprintf("%p/%p ",queues_ns_nlp[i][j], queues_nlp_ns[j][i]);
	  }
	  myprintf("\n");
	  }*/
    
    //Lower layer in pod to upper layer in pod!
    for (int j = 0; j < NK; j++) {
	int podid = 2*j/K;
	//Connect the lower layer switch to the upper layer switches in the same pod
	for (int k=MIN_POD_ID(podid); k<=MAX_POD_ID(podid);k++){
	    // Downlink
	    queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
	    logfile->addLogger(*queueLogger);
	    queues_nup_nlp[k][j] = alloc_queue(queueLogger);
	    
	    queues_nup_nlp[k][j]->setName("US_" + ntoa(k) + "-" + "LS_"+ntoa(j));
	    logfile->writeName(*(queues_nup_nlp[k][j]));
	
	    pipes_nup_nlp[k][j] = new Pipe(timeFromUs(RTT), *eventlist);
	    pipes_nup_nlp[k][j]->setName("Pipe-na-nt-" + ntoa(k) + "-" + ntoa(j));
	    logfile->writeName(*(pipes_nup_nlp[k][j]));
	
	    // Uplink
	    queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
	    logfile->addLogger(*queueLogger);
	    queues_nlp_nup[j][k] = alloc_queue(queueLogger);

	    queues_nlp_nup[j][k]->setName("LS_" + ntoa(j) + "-" + "US_"+ntoa(k));
	    logfile->writeName(*(queues_nlp_nup[j][k]));
	
	    pipes_nlp_nup[j][k] = new Pipe(timeFromUs(RTT), *eventlist);
	    pipes_nlp_nup[j][k]->setName("Pipe-nt-na-" + ntoa(j) + "-" + ntoa(k));
	    logfile->writeName(*(pipes_nlp_nup[j][k]));
	
	    if (ff){
		ff->add_queue(queues_nlp_nup[j][k]);
		ff->add_queue(queues_nup_nlp[k][j]);
	    }
	}
    }

    /*for (int i = 0;i<NK;i++){
      for (int j = 0;j<NK;j++){
      myprintf("%p/%p ",queues_nlp_nup[i][j], queues_nup_nlp[j][i]);
      }
      myprintf("\n");
      }*/
    
    // Upper layer in pod to core!
    for (int j = 0; j < NK; j++) {
	int podpos = j%(K/2);
	for (int l = 0; l < K/2; l++) {
	    int k = podpos * K/2 + l;
	    // Downlink
	    queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
	    logfile->addLogger(*queueLogger);

	    queues_nup_nc[j][k] = alloc_queue(queueLogger);
	    queues_nup_nc[j][k]->setName("US_" + ntoa(j) + "-" + "CS_"+ ntoa(k));
	    logfile->writeName(*(queues_nup_nc[j][k]));
	
	    pipes_nup_nc[j][k] = new Pipe(timeFromUs(RTT), *eventlist);
	    pipes_nup_nc[j][k]->setName("Pipe-nup-nc-" + ntoa(j) + "-" + ntoa(k));
	    logfile->writeName(*(pipes_nup_nc[j][k]));
	
	    // Uplink
	
	    queueLogger = new QueueLoggerSampling(timeFromMs(1000), *eventlist);
	    logfile->addLogger(*queueLogger);
	
	    //	if (k==0&&j==0)
	    //queues_nc_nup[k][j] = alloc_queue(queueLogger,HOST_NIC/10);
	    //else
	    queues_nc_nup[k][j] = alloc_queue(queueLogger);
	    queues_nc_nup[k][j]->setName("CS_" + ntoa(k) + "-" + "US_"+ntoa(j));


	    logfile->writeName(*(queues_nc_nup[k][j]));
	
	    pipes_nc_nup[k][j] = new Pipe(timeFromUs(RTT), *eventlist);
	    pipes_nc_nup[k][j]->setName("Pipe-nc-nup-" + ntoa(k) + "-" + ntoa(j));
	    logfile->writeName(*(pipes_nc_nup[k][j]));
	
	    if (ff){
		ff->add_queue(queues_nup_nc[j][k]);
		ff->add_queue(queues_nc_nup[k][j]);
	    }
	}
    }

    /*    for (int i = 0;i<NK;i++){
	  for (int j = 0;j<NC;j++){
	  myprintf("%p/%p ",queues_nup_nc[i][j], queues_nc_nup[j][i]);
	  }
	  myprintf("\n");
	  }*/
}

void check_non_null(Route* rt);

vector<const Route*>* OversubscribedFatTreeTopology::get_paths(int src, int dest){
    vector<const Route*>* paths = new vector<const Route*>();

    Route* routeout;

    if (HOST_POD_SWITCH(src)==HOST_POD_SWITCH(dest)){
	routeout = new Route();

	routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)]);
	routeout->push_back(pipes_ns_nlp[src][HOST_POD_SWITCH(src)]);

	routeout->push_back(queues_nlp_ns[HOST_POD_SWITCH(dest)][dest]);
	routeout->push_back(pipes_nlp_ns[HOST_POD_SWITCH(dest)][dest]);

	paths->push_back(routeout);

	check_non_null(routeout);
	return paths;
    }
    else if (HOST_POD(src)==HOST_POD(dest)){
	//don't go up the hierarchy, stay in the pod only.

	int pod = HOST_POD(src);
	//there are K/2 paths between the source and the destination
	for (int upper = MIN_POD_ID(pod);upper <= MAX_POD_ID(pod); upper++){
	    routeout = new Route();
      
	    routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)]);
	    routeout->push_back(pipes_ns_nlp[src][HOST_POD_SWITCH(src)]);

	    routeout->push_back(queues_nlp_nup[HOST_POD_SWITCH(src)][upper]);
	    routeout->push_back(pipes_nlp_nup[HOST_POD_SWITCH(src)][upper]);

	    routeout->push_back(queues_nup_nlp[upper][HOST_POD_SWITCH(dest)]);
	    routeout->push_back(pipes_nup_nlp[upper][HOST_POD_SWITCH(dest)]);
      
	    routeout->push_back(queues_nlp_ns[HOST_POD_SWITCH(dest)][dest]);
	    routeout->push_back(pipes_nlp_ns[HOST_POD_SWITCH(dest)][dest]);
      
	    paths->push_back(routeout);
	    check_non_null(routeout);
	}
	return paths;
    }
    else {
	int pod = HOST_POD(src);

	for (int upper = MIN_POD_ID(pod);upper <= MAX_POD_ID(pod); upper++)
	    for (int core = (upper%(K/2)) * K / 2; core < ((upper % (K/2)) + 1)*K/2; core++){
		//upper is nup
	
		routeout = new Route();
	
		routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)]);
		routeout->push_back(pipes_ns_nlp[src][HOST_POD_SWITCH(src)]);
	
		routeout->push_back(queues_nlp_nup[HOST_POD_SWITCH(src)][upper]);
		routeout->push_back(pipes_nlp_nup[HOST_POD_SWITCH(src)][upper]);
	
		routeout->push_back(queues_nup_nc[upper][core]);
		routeout->push_back(pipes_nup_nc[upper][core]);
	
		//now take the only link down to the destination server!
	
		int upper2 = HOST_POD(dest)*K/2 + 2 * core / K;
		//myprintf("K %d HOST_POD(%d) %d core %d upper2 %d\n",K,dest,HOST_POD(dest),core, upper2);
	
		routeout->push_back(queues_nc_nup[core][upper2]);
		routeout->push_back(pipes_nc_nup[core][upper2]);
	
		routeout->push_back(queues_nup_nlp[upper2][HOST_POD_SWITCH(dest)]);
		routeout->push_back(pipes_nup_nlp[upper2][HOST_POD_SWITCH(dest)]);
	
		routeout->push_back(queues_nlp_ns[HOST_POD_SWITCH(dest)][dest]);
		routeout->push_back(pipes_nlp_ns[HOST_POD_SWITCH(dest)][dest]);
	
		paths->push_back(routeout);
		check_non_null(routeout);
	    }
	return paths;
    }
}

void OversubscribedFatTreeTopology::count_queue(Queue* queue){
    if (_link_usage.find(queue)==_link_usage.end()){
	_link_usage[queue] = 0;
    }

    _link_usage[queue] = _link_usage[queue] + 1;
}

int OversubscribedFatTreeTopology::find_lp_switch(Queue* queue){
    //first check ns_nlp
    for (int i=0;i<NSRV;i++)
	for (int j = 0;j<NK;j++)
	    if (queues_ns_nlp[i][j]==queue)
		return j;

    //only count nup to nlp
    count_queue(queue);

    for (int i=0;i<NK;i++)
	for (int j = 0;j<NK;j++)
	    if (queues_nup_nlp[i][j]==queue)
		return j;

    return -1;
}

int OversubscribedFatTreeTopology::find_up_switch(Queue* queue){
    count_queue(queue);
    //first check nc_nup
    for (int i=0;i<NC;i++)
	for (int j = 0;j<NK;j++)
	    if (queues_nc_nup[i][j]==queue)
		return j;

    //check nlp_nup
    for (int i=0;i<NK;i++)
	for (int j = 0;j<NK;j++)
	    if (queues_nlp_nup[i][j]==queue)
		return j;

    return -1;
}

int OversubscribedFatTreeTopology::find_core_switch(Queue* queue){
    count_queue(queue);
    //first check nup_nc
    for (int i=0;i<NK;i++)
	for (int j = 0;j<NC;j++)
	    if (queues_nup_nc[i][j]==queue)
		return j;

    return -1;
}

int OversubscribedFatTreeTopology::find_destination(Queue* queue){
    //first check nlp_ns
    for (int i=0;i<NK;i++)
	for (int j = 0;j<NSRV;j++)
	    if (queues_nlp_ns[i][j]==queue)
		return j;

    return -1;
}

void OversubscribedFatTreeTopology::print_path(std::ofstream &paths,int src,const Route* route){
    paths << "SRC_" << src << " ";
  
    if (route->size()/2==2){
	paths << "LS_" << find_lp_switch((RandomQueue*)route->at(1)) << " ";
	paths << "DST_" << find_destination((RandomQueue*)route->at(3)) << " ";
    } else if (route->size()/2==4){
	paths << "LS_" << find_lp_switch((RandomQueue*)route->at(1)) << " ";
	paths << "US_" << find_up_switch((RandomQueue*)route->at(3)) << " ";
	paths << "LS_" << find_lp_switch((RandomQueue*)route->at(5)) << " ";
	paths << "DST_" << find_destination((RandomQueue*)route->at(7)) << " ";
    } else if (route->size()/2==6){
	paths << "LS_" << find_lp_switch((RandomQueue*)route->at(1)) << " ";
	paths << "US_" << find_up_switch((RandomQueue*)route->at(3)) << " ";
	paths << "CS_" << find_core_switch((RandomQueue*)route->at(5)) << " ";
	paths << "US_" << find_up_switch((RandomQueue*)route->at(7)) << " ";
	paths << "LS_" << find_lp_switch((RandomQueue*)route->at(9)) << " ";
	paths << "DST_" << find_destination((RandomQueue*)route->at(11)) << " ";
    } else {
	paths << "Wrong hop count " << ntoa(route->size()/2);
    }
  
    paths << endl;
}

vector<int>* OversubscribedFatTreeTopology::get_neighbours(int src){
    vector<int>* neighbours = new vector<int>();
    int sw = HOST_POD_SWITCH(src)*2*K;
    for (int i=0;i<2*K;i++){
	int dst = i+sw;
	if (dst!=src)
	    neighbours->push_back(dst);
    }
  
    return neighbours;
}

