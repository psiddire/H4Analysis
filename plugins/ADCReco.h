#ifndef __ADC_RECO__
#define __ADC_RECO__

#include "interface/PluginBase.h"

using namespace std;

class ADCReco: public PluginBase
{
public:
    //---ctors---
    ADCReco() {};

    //---dtor---
    ~ADCReco() {};

    //---utils---
    bool Begin(CfgManager& opts, uint64* index);
    bool ProcessEvent(const H4Tree& event, map<string, PluginBase*>& plugins, CfgManager& opts);
    bool End(CfgManager& opts) { return true; };
    
private:
    int  nChannels_;
    vector<string> chNames_;
    vector<int>    chNum_;
    vector<float>* values_;
    TTree*         adcTree_;
};

DEFINE_PLUGIN(ADCReco);

#endif
