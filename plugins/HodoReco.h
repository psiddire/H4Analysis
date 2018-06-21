#ifndef __HODO_RECO__
#define __HODO_RECO__

#include "interface/PluginBase.h"
#include "interface/PositionTree.h"

#define HODO_X1 0
#define HODO_Y1 1
#define HODO_X2 2
#define HODO_Y2 3

using namespace std;

class HodoReco: public PluginBase
{
public:
    //---ctors---
    HodoReco() {};

    //---dtor---
    ~HodoReco() {};

    //---utils---
    bool Begin(CfgManager& opts, uint64* index);
    bool ProcessEvent(const H4Tree& event, map<string, PluginBase*>& plugins, CfgManager& opts);
    bool End(CfgManager& opts) { return true; };
    
private:
    int              nPlanes_=2;
    int              nFibers_=64;
    std::vector<int> hodoFiberOrderA_;
    std::vector<int> hodoFiberOrderB_;
    PositionTree     hodoTree_;
};

DEFINE_PLUGIN(HodoReco);

#endif
