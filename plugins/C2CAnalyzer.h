#ifndef __C2C_Analyzer__
#define __C2C_Analyzer__

#include <iostream>
#include <math.h>

#include "TH2F.h"
#include "TVirtualFFT.h"

#include "interface/utils.h"
#include "interface/PluginBase.h"
#include "interface/DigiTree.h"
#include "interface/WFTree.h"
#include "interface/WFClass.h"
#include "interface/FFTClass.h"

class C2CAnalyzer: public PluginBase
{
public:
    //---ctors---
    C2CAnalyzer();

    //---dtor---
    ~C2CAnalyzer(){};

    //---utils---
    bool Begin(CfgManager& opts, uint64* index);
    bool ProcessEvent(const H4Tree& event, map<string, PluginBase*>& plugins, CfgManager& opts);
    bool End(CfgManager& opts);
    
private:    
    //---internal data
    uint64*                   index_;
    unsigned int              n_tot_;
    int*                      current_ch_;
    float*                    freqs_;
    float*                    re_;
    float*                    im_;
    float*                    amplitudes_;
    float*                    phases_;
    string                    fftType_;
    string                    srcInstance_;
    vector<string>            channelsNames_;
    vector<string>            templatesNames_;
    map<string, int>          channelsMap_;
    map<string, TH1F*>        templatesHistos_;
    map<string, TH2F*>        templates2dHistos_;
    map<string, FFTClass*>    FFTs_;
    map<string, WFClass*>     WFs_;
    TTree*                    fftTree_;         
    TFile* noiseTemplateFile_;
    TH1F* noiseTemplateHistoRe_;
    TH1F* noiseTemplateHistoIm_;
};

DEFINE_PLUGIN(C2CAnalyzer);

#endif
