#ifndef __INFO_TREE_MAKER__
#define __INFO_TREE_MAKER__

#include "interface/PluginBase.h"

class InfoTreeMaker: public PluginBase
{
public:
    //---ctors---
    InfoTreeMaker(){};

    //---dtor---
    ~InfoTreeMaker(){};

    //---utils---
    bool Begin(CfgManager& opts, uint64* index);
    bool ProcessEvent(const H4Tree& event, map<string, PluginBase*>& plugins, CfgManager& opts);
    bool End(CfgManager& opts) { return true; };
    
private:    
    //---internal data
    uint64* index_;
    TTree*  info_tree_;
    string                           trackedVariable_;
    map<string, float*>              mappedVariables_;
    map<string, map<string, float> > remap_;    
};

DEFINE_PLUGIN(InfoTreeMaker);

#endif
