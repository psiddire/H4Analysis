#include "WFAnalyzer.h"

//----------Utils-------------------------------------------------------------------------
bool WFAnalyzer::Begin(CfgManager& opts, uint64* index)
{
    //---inputs---
    if(!opts.OptExist(instanceName_+".srcInstanceName"))
    {
        cout << ">>> ERROR: no source plugin specified" << endl;
        return false;
    }
    srcInstance_ = opts.GetOpt<string>(instanceName_+".srcInstanceName");
    channelsNames_ = opts.GetOpt<vector<string> >(instanceName_+".channelsNames");
    timeRecoTypes_ = opts.GetOpt<vector<string> >(instanceName_+".timeRecoTypes");

    //---channels setup
    string templateTag="prof";
    if(opts.OptExist(instanceName_+".templateTags"))
        for(auto& tag : opts.GetOpt<vector<string> >(instanceName_+".templateTags"))
            for(auto& run : opts.GetOpt<vector<string> >(tag+".runList"))
                if(run == opts.GetOpt<string>("h4reco.run"))
                    templateTag = tag;

    int nSamples = 0;
    for(auto& channel : channelsNames_)
    {        
        nSamples += opts.GetOpt<int>(channel+".nSamples");
        if(opts.OptExist(channel+".templateFit.file"))
        {            
            TFile* templateFile = TFile::Open(opts.GetOpt<string>(channel+".templateFit.file", 0).c_str(), ".READ");
            TH1* wfTemplate=(TH1*)templateFile->Get((opts.GetOpt<string>(channel+".templateFit.file", 1)+
                                                     +"_"+templateTag).c_str());
            templates_[channel] = (TH1F*) wfTemplate->Clone();
            templates_[channel] -> SetDirectory(0);
            templateFile->Close();
        }
        //---keep track of all the possible time reco method requested
        for(auto type_name : timeRecoTypes_)
        {
            if(opts.OptExist(channel+"."+type_name))
                timeOpts_[channel+"."+type_name] = opts.GetOpt<vector<float> >(channel+"."+type_name);
        }
    }
    
    //---outputs---
    string digiTreeName = opts.OptExist(instanceName_+".digiTreeName") ?
        opts.GetOpt<string>(instanceName_+".digiTreeName") : "digi";
    bool storeTree = opts.OptExist(instanceName_+".storeTree") ?
        opts.GetOpt<bool>(instanceName_+".storeTree") : true;
    RegisterSharedData(new TTree(digiTreeName.c_str(), "digi_tree"), "digi_tree", storeTree);
    digiTree_ = DigiTree(index, (TTree*)data_.back().obj);
    digiTree_.Init(channelsNames_, timeRecoTypes_);
    if(opts.GetOpt<int>(instanceName_+".fillWFtree"))
    {
        string wfTreeName = opts.OptExist(instanceName_+".wfTreeName") ?
            opts.GetOpt<string>(instanceName_+".wfTreeName") : "wf";
        RegisterSharedData(new TTree(wfTreeName.c_str(), "wf_tree"), "wf_tree", true);
        outWFTree_ = WFTree(nSamples, index, (TTree*)data_.back().obj);
        outWFTree_.Init();
    }

    return true;
}

bool WFAnalyzer::ProcessEvent(const H4Tree& h4Tree, map<string, PluginBase*>& plugins, CfgManager& opts)
{
    //---setup output event 
    int outCh=0;
    bool fillWFtree=false;
    if(opts.GetOpt<int>(instanceName_+".fillWFtree"))
        fillWFtree = *digiTree_.index % opts.GetOpt<int>(instanceName_+".WFtreePrescale") == 0;

    //---load WFs from source instance shared data
    for(auto& channel : channelsNames_)
    {
        auto shared_data = plugins[srcInstance_]->GetSharedData(srcInstance_+"_"+channel, "", false);
        if(shared_data.size() != 0){
            WFs_[channel] = (WFClass*)shared_data.at(0).obj;
	    //cout << "Channel: " << channel << " WFs[channel]: " << (WFClass*)WFs_[channel] << endl;
	}
        else
            cout << "[WFAnalyzer::" << instanceName_ << "]: channels samples not found check DigiReco step" << endl; 
    }

    //---compute reco variables
    for(auto& channel : channelsNames_)
    {
        //---skip dead channels
        if(WFs_.find(channel) == WFs_.end())
        {
            ++outCh;
            continue;
        }

        //---subtract a specified channel if requested
        if(opts.OptExist(channel+".subtractChannel") && WFs_.find(opts.GetOpt<string>(channel+".subtractChannel")) != WFs_.end())
            *WFs_[channel] -= *WFs_[opts.GetOpt<string>(channel+".subtractChannel")];        
        WFs_[channel]->SetBaselineWindow(opts.GetOpt<int>(channel+".baselineWin", 0), 
                                         opts.GetOpt<int>(channel+".baselineWin", 1));
        WFs_[channel]->SetSignalWindow(opts.GetOpt<int>(channel+".signalWin", 0), 
                                       opts.GetOpt<int>(channel+".signalWin", 1));
        WFBaseline baselineInfo = WFs_[channel]->SubtractBaseline();
        string max_function = opts.OptExist(channel+".signalWin", 3) ? opts.GetOpt<string>(channel+".signalWin", 3) : "pol2";
        WFFitResults interpolAmpMax = WFs_[channel]->GetInterpolatedAmpMax(-1,-1, opts.GetOpt<int>(channel+".signalWin", 2), max_function);

	if(WFs_[channel]->GetIntegral(opts.GetOpt<int>(channel+".baselineInt", 0),opts.GetOpt<int>(channel+".baselineInt", 1)) > -500 && WFs_[channel]->GetIntegral(opts.GetOpt<int>(channel+".baselineInt", 0),opts.GetOpt<int>(channel+".baselineInt", 1)) < 500){
	  digiTree_.b_charge[outCh] = WFs_[channel]->GetIntegral(opts.GetOpt<int>(channel+".baselineInt", 0), 
                                                               opts.GetOpt<int>(channel+".baselineInt", 1));
	}
	else{
	  continue;
	}
	if (baselineInfo.slope > -100 && baselineInfo.slope < 100)
	  digiTree_.b_slope[outCh] = baselineInfo.slope;
	else
	  continue;
	if (baselineInfo.rms < 100)
	  digiTree_.b_rms[outCh] = baselineInfo.rms;
	else
	  continue;
	if (WFs_[channel]->GetAmpMax() < 1000)
	  digiTree_.maximum[outCh] = WFs_[channel]->GetAmpMax();
	else
	  continue;
        digiTree_.time_maximum[outCh] = WFs_[channel]->GetTimeCF(1).first;
        if (interpolAmpMax.ampl < 1000)
	  digiTree_.amp_max[outCh] = interpolAmpMax.ampl;
	else
	  continue;
        digiTree_.time_max[outCh] = interpolAmpMax.time;
        if (interpolAmpMax.chi2 < 10000)
	  digiTree_.chi2_max[outCh] = interpolAmpMax.chi2;
	else
	  continue;
        digiTree_.charge_tot[outCh] = WFs_[channel]->GetModIntegral(opts.GetOpt<int>(channel+".baselineInt", 1), 
                                                                    WFs_[channel]->GetNSample());
        digiTree_.charge_sig[outCh] = WFs_[channel]->GetSignalIntegral(opts.GetOpt<int>(channel+".signalInt", 0), 
                                                                       opts.GetOpt<int>(channel+".signalInt", 1));
        //---compute time with all the requested time reconstruction method
        for(unsigned int iT=0; iT<timeRecoTypes_.size(); ++iT)
        {
            //---compute time with selected method or store default value (-99)
            if(timeOpts_.find(channel+"."+timeRecoTypes_[iT]) != timeOpts_.end())
            {
                pair<float, float> timeInfo = WFs_[channel]->GetTime(timeRecoTypes_[iT], timeOpts_[channel+"."+timeRecoTypes_[iT]]);
                digiTree_.time[outCh+iT*channelsNames_.size()] = timeInfo.first;
                digiTree_.time_chi2[outCh+iT*channelsNames_.size()] = timeInfo.second;
            }
            else
            {
                digiTree_.time[outCh+iT*channelsNames_.size()] = -99;
                digiTree_.time_chi2[outCh+iT*channelsNames_.size()] = -99;
            }
        }

        //---template fit (only specified channels)
        WFFitResults fitResults{-1, -1000, -1};
        if(opts.OptExist(channel+".templateFit.file"))
        {
            WFs_[channel]->SetTemplate(templates_[channel]);
            fitResults = WFs_[channel]->TemplateFit(opts.GetOpt<float>(channel+".templateFit.fitWin", 0),
                                                    opts.GetOpt<int>(channel+".templateFit.fitWin", 1),
                                                    opts.GetOpt<int>(channel+".templateFit.fitWin", 2));
            digiTree_.fit_ampl[outCh] = fitResults.ampl;
            digiTree_.fit_time[outCh] = fitResults.time;
            digiTree_.fit_chi2[outCh] = fitResults.chi2;
        }            
        //---calibration constant for each channel if needed
        if(opts.OptExist(channel+".calibration.calibrationConst"))
            digiTree_.calibration[outCh]=opts.GetOpt<float>(channel+".calibration.calibrationConst");
        else
            digiTree_.calibration[outCh]=1;
	
        //---WFs---
        if(fillWFtree)
        {
            auto analyzedWF = WFs_[channel]->GetSamples();
            float tUnit = WFs_[channel]->GetTUnit();
            for(unsigned int jSample=0; jSample<analyzedWF->size(); ++jSample)
            {
                outWFTree_.WF_ch.push_back(outCh);
                outWFTree_.WF_time.push_back(jSample*tUnit);
                outWFTree_.WF_val.push_back(analyzedWF->at(jSample));
            }
        }
        //---increase output tree channel counter
        ++outCh;
    }

    //---fill the output trees 
    //---reco var
    digiTree_.Fill();
    //---WFs
    if(fillWFtree)
        outWFTree_.Fill();

    return true;
}
