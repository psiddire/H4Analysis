#include "C2CAnalyzer.h"

//----------Constructor-------------------------------------------------------------------
C2CAnalyzer::C2CAnalyzer():
    n_tot_(0)
{}

//----------Utils-------------------------------------------------------------------------
bool C2CAnalyzer::Begin(CfgManager& opts, uint64* index)
{

    //---register shared FFTs
    //   nSamples is divided by two if FFT is from time to frequency domain
    srcInstance_ = opts.GetOpt<string>(instanceName_+".srcInstanceName");    
    channelsNames_ = opts.GetOpt<vector<string> >(instanceName_+".channelsNames");    
    fftType_ = opts.OptExist(instanceName_+".FFTType") ?
        opts.GetOpt<string>(instanceName_+".FFTType") : "C2C";
    bool storeFFT = opts.OptExist(instanceName_+".storeFFToutput") ?
        opts.GetOpt<bool>(instanceName_+".storeFFToutput") : false;
    for(auto& channel : channelsNames_)
    {
        if(!opts.OptExist(channel+".tUnit"))
        {
            cout << ">>> C2CAnalyzer ERROR: configuration for channel < " << channel << " > not found." << endl;
            return false;
        }
        if(fftType_ == "C2C")
        {
            FFTs_[channel] = new FFTClass();
            RegisterSharedData(FFTs_[channel], channel, storeFFT);
        }
        else
        {
            WFs_[channel] = new WFClass(1, opts.GetOpt<float>(channel+".tUnit"));
            RegisterSharedData(WFs_[channel], channel, storeFFT);
            if(opts.OptExist(instanceName_+".subtractFFTNoise")){
                noiseTemplateFile_= TFile::Open(opts.GetOpt<string>(instanceName_+".noiseTemplateFile").c_str());
                TString noiseTemplateHistoName (opts.GetOpt<string>(instanceName_+".noiseTemplateHisto"));
                noiseTemplateHistoRe_ = (TH1F*) noiseTemplateFile_->Get(noiseTemplateHistoName+"_Re_tmpl");
                noiseTemplateHistoIm_ = (TH1F*) noiseTemplateFile_->Get(noiseTemplateHistoName+"_Im_tmpl");
                if (!noiseTemplateFile_)
                {
                    cout << ">>> FFTAnalyzer ERROR: noiseTemplateFile not open " << endl;
                    return false;
                }
            }
        }
    }

    //---create and register templates histograms
    //   histograms are created with automatic binning along Y axis
    if(fftType_ == "C2C" && opts.OptExist(instanceName_+".makeTemplates"))
        templatesNames_ =  opts.GetOpt<vector<string> >(instanceName_+".makeTemplates");
    for(auto& channel : channelsNames_)
    {
        for(auto& tmpl : templatesNames_)
        {
            auto nSamples = opts.GetOpt<int>(channel+".nSamples");
            templates2dHistos_[channel+tmpl] = new TH2F((channel+tmpl).c_str(),
                                                        ("Template "+channel+" "+tmpl).c_str(),
                                                        nSamples/2, 0, nSamples/2,
                                                        10000, 0, 0);
            templatesHistos_[channel+tmpl] = new TH1F((channel+"_"+tmpl+"_tmpl").c_str(),
                                                      ("Template "+channel+" "+tmpl).c_str(),
                                                      nSamples/2, 0, nSamples/2);
            RegisterSharedData(templatesHistos_[channel+tmpl], channel+"_"+tmpl+"_tmpl", true);
        }
    }
    
    //---register output data tree if requested (default true)
    bool storeTree = opts.OptExist(instanceName_+".storeTree") && fftType_ == "C2C" ?
        opts.GetOpt<bool>(instanceName_+".storeTree") : false;
    if(storeTree)
    {
        for(auto& channel : channelsNames_)
        {
            auto point = std::find(channelsNames_.begin(), channelsNames_.end(), channel);
            if(point != channelsNames_.end())
            {
                channelsMap_[channel] = point-channelsNames_.begin();
                n_tot_ += opts.GetOpt<int>(channel+".nSamples")/2;
            }
        }
            
        string fftTreeName = opts.OptExist(instanceName_+".fftTreeName") ?
            opts.GetOpt<string>(instanceName_+".fftTreeName") : "fft";
        RegisterSharedData(new TTree(fftTreeName.c_str(), "fft_tree"), "fft_tree", storeTree);
        //---create tree branches:
        //   array size is determined by DigitizerReco channels
        index_ = index;
        current_ch_ = new int[n_tot_];
        freqs_ = new float[n_tot_];
        re_ = new float[n_tot_];
        im_ = new float[n_tot_];        
        amplitudes_ = new float[n_tot_];
        phases_ = new float[n_tot_];
        fftTree_ = (TTree*)data_.back().obj;
        fftTree_->Branch("index", index_, "index/l");
        fftTree_->Branch("n_tot", &n_tot_, "n_tot/i");
        fftTree_->Branch("ch", current_ch_, "ch[n_tot]/I");        
        fftTree_->Branch("freq", freqs_, "freq[n_tot]/F");
        fftTree_->Branch("re", re_, "re[n_tot]/F");
        fftTree_->Branch("im", im_, "im[n_tot]/F");
        fftTree_->Branch("ampl", amplitudes_, "ampl[n_tot]/F");
        fftTree_->Branch("phi", phases_, "phi[n_tot]/F");
        //---set default values
        for(unsigned int i=0; i<n_tot_; ++i)
        {
            current_ch_[i]=-1;
            freqs_[i] = -1;
            re_[i] = -10;
            im_[i] = -10;
            amplitudes_[i]=-1;
            phases_[i]=-1;
        }
    }
    else
        fftTree_ = NULL;
    
    return true;
}

bool C2CAnalyzer::ProcessEvent(const H4Tree& event, map<string, PluginBase*>& plugins, CfgManager& opts)
{
    for(auto& channel : channelsNames_)
    {
        //---FFT from time to frequency domain /// T2F
        if(fftType_ == "C2C")
        {
            //---get WF from source instance data and reset FFT
            FFTs_[channel]->Reset();
            auto wf = (WFClass*)plugins[srcInstance_]->GetSharedData(srcInstance_+"_"+channel, "", false).at(0).obj;
            auto samples = wf->GetSamples();
            auto samples_norm = *samples;
            int n_samples = samples->size();
	    double phase[n_samples];
	    for (int i=0; i < n_samples; i++){
	      phase[i]=0.0;
	    }
	    double* phasepointer = phase;
            if(opts.OptExist(instanceName_+".normalizeInput") && opts.GetOpt<bool>(instanceName_+".normalizeInput"))
            {
                float max = *std::max_element(samples_norm.begin(), samples_norm.end());
                for(auto& sample : samples_norm){
		  sample /= max;
		}
	    }
            //---build the FFT
            double Re[n_samples], Im[n_samples];
            auto fftr2c = TVirtualFFT::FFT(1, &n_samples, "C2CFORWARD");
	    //auto fftr2c = TVirtualFFT::FFT(1, &n_samples, "C2CBACKWARD");
	    cout << "samples_norm.data(): " << samples_norm.data() << endl;
	    //for(auto& sample : samples_norm)
	    //  cout <<  "sample: " << sample << endl;
	    //fftr2c->SetPoints(samples_norm.data());
	    fftr2c->SetPointsComplex(samples_norm.data(),phasepointer);
            fftr2c->Transform();
            fftr2c->GetPointsComplex(Re, Im);
	    //for(int i=0; i<500; i++)
	    //  cout << "Re and Im: " << *(Re+i) << " " << *(Im+i) << endl;
            FFTs_[channel]->SetPointsComplex(n_samples/2, Re, Im);
            map<string, const double*> var_map;
            var_map["Re"] = Re;
            var_map["Im"] = Im;
            var_map["Ampl"] = FFTs_[channel]->GetAmplitudes()->data();
            var_map["Phase"] = FFTs_[channel]->GetPhases()->data();
            if(fftTree_ || templatesNames_.size() != 0)
              {
                for(int k=0; k<n_samples/2; ++k)
                {
                    for(auto& tmpl : templatesNames_){
                        templates2dHistos_[channel+tmpl]->Fill(k, var_map[tmpl][k]);
                    }
                    if(fftTree_)
                    {
                        int index =  channelsMap_[channel] * n_samples/2 + k;
                        current_ch_[index] = channelsMap_[channel];
                        freqs_[index] = (k%(n_samples/2));
                        re_[index] = Re[index];
			im_[index] = Im[index];
			amplitudes_[index] = var_map["Ampl"][k];
			phases_[index] = var_map["Phase"][k];
			//cout << "Real " << Re[index] <<  "Imaginary " << Im[index] <<  "Amplitude " << var_map["Ampl"][k] << "Phase " << var_map["Phase"][k] << endl;
                    }
                }
            }
            delete fftr2c;
        }
        //---FFT from frequency to time domain /// F2T
        else
        {
            //---get FFT from source instance data and reset old WF
            WFs_[channel]->Reset();
            auto fft = (FFTClass*)plugins[srcInstance_]->GetSharedData(srcInstance_+"_"+channel, "", false).at(0).obj;

            //---build the FFT
            int n_samples = fft->GetAmplitudes()->size()*2;
            double data[n_samples];
            auto Re = fft->GetRe();
            auto Im = fft->GetIm();
            auto fftc2r = TVirtualFFT::FFT(1, &n_samples, "C2R");
            
            //---subtract FFT of noise from template before going back to time domain
            if(opts.OptExist(instanceName_+".subtractFFTNoise"))
            {
                double noiseRe=0,noiseIm=0,newRe=0, newIm=0;
                for(int i=0;i<n_samples/2;++i)
                {
                    noiseRe = noiseTemplateHistoRe_->GetBinContent(i);
                    newRe = *(Re->data()+i) - noiseRe; 
                    noiseIm = noiseTemplateHistoIm_->GetBinContent(i);
                    newIm = *(Im->data()+i) - noiseIm; 
                    fftc2r->SetPoint(i,newRe,newIm);
                }
            } 
            else if(opts.OptExist(instanceName_+".frequencyCut"))
            {
                for(int i=0;i<n_samples/2;++i)
                {
                    if(opts.OptExist(instanceName_+".frequencyCut"))
                    {
                        if(i<opts.GetOpt<float>(instanceName_+".frequencyCut"))
                            fftc2r->SetPoint(i,*(Re->data()+i),*(Im->data()+i));
                        else
                            fftc2r->SetPoint(i,0,0);
                    }
                }
            }
            else
                fftc2r->SetPointsComplex(Re->data(), Im->data());
            fftc2r->Transform();
            fftc2r->GetPoints(data);

            //---fill new WF
            for(int iSample=0; iSample<n_samples; ++iSample)
                WFs_[channel]->AddSample(data[iSample]/n_samples);

            delete fftc2r;
        }
    }
    //---fill FFT tree
    if(fftTree_)
        fftTree_->Fill();

    return true;
}

bool C2CAnalyzer::End(CfgManager& opts)
{
    for(auto& channel : channelsNames_)
        for(auto& tmpl : templatesNames_)
            GetIterativeProfile(templates2dHistos_[channel+tmpl], templatesHistos_[channel+tmpl]);

    return true;
}
