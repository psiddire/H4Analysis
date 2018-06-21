#ifndef __HODO_BTFRECO__
#define __HODO_BTFRECO__

#include "interface/PluginBase.h"
#include "interface/PositionTree.h"

using namespace std;

class HodoBTFReco: public PluginBase
{
public:
    //---ctors---
    HodoBTFReco() {};

    //---dtor---
    ~HodoBTFReco() {};

    //---utils---
    bool Begin(CfgManager& opts, uint64* index);
    bool ProcessEvent(const H4Tree& h4Tree, map<string, PluginBase*>& plugins, CfgManager& opts);
    bool End(CfgManager& opts) { return true; };
    
private:
    map<int, int> ADC_to_PMT_map;
    map<int, int> PMT_to_hodoX_map;
    map<int, int> PMT_to_hodoY_map;
    PositionTree  hodoTree_;

    vector<float> hodoXpos;
    vector<float> hodoYpos;
    unsigned int nAdcChannels;
    unsigned int adcData[100], adcBoard[100], adcChannel[100];    
};

DEFINE_PLUGIN(HodoBTFReco);

#endif
