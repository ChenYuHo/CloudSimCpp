#ifndef TOPOLOGY
#define TOPOLOGY
#include "network.h"
#include <memory>

class Worker;
class Switch;

class Topology {
 public:
  virtual vector<const Route*>* get_paths(int src,int dest)=0;
  virtual vector<int>* get_neighbours(int src) = 0;  
  virtual int no_of_nodes() const { abort();};
  virtual vector<shared_ptr<Worker>> workers() = 0;
    virtual vector<shared_ptr<Switch>> switches() = 0;
};

#endif
