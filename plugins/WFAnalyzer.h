#ifndef __WF_ANALYZER__
#define __WF_ANALYZER__

#include <iostream>

#include "interface/PluginBase.h"
#include "interface/DigiTree.h"
#include "interface/WFTree.h"
#include "interface/WFClass.h"
#include "interface/WFClassNINO.h"
#include "interface/WFClassClock.h"
#include "interface/WFViewer.h"

class WFAnalyzer: public PluginBase
{
public:
    //---ctors---
    WFAnalyzer(){};

    //---dtor---
    ~WFAnalyzer(){};

    //---utils---
    bool Begin(CfgManager& opts, uint64* index);
    bool ProcessEvent(const H4Tree& event, map<string, PluginBase*>& plugins, CfgManager& opts);
    bool End(CfgManager& opts) { return true; };
    
private:    
    //---internal data
    string                      srcInstance_;
    vector<string>              channelsNames_;
    vector<string>              timeRecoTypes_;
    map<string, vector<float> > timeOpts_;
    DigiTree                    digiTree_;
    WFTree                      outWFTree_;
    map<string, WFClass*>       WFs_;
    map<string, TH1*>           templates_;
};

DEFINE_PLUGIN(WFAnalyzer);

#endif
