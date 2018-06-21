#include "DFTTemplate.h"

//----------Utils-------------------------------------------------------------------------
bool DFTTemplate::Begin(CfgManager& opts, uint64* index)
{

    //---register shared FFTs
    //   nSamples is divided by two if FFT is from time to frequency domain
    srcInstance_ = opts.GetOpt<string>(instanceName_+".srcInstanceName");    
    channelsNames_ = opts.GetOpt<vector<string> >(instanceName_+".channelsNames");    
    for(auto& channel : channelsNames_)
    {
        if(!opts.OptExist(channel+".fOversampling"))
        {
            cout << ">>> DFTTemplate ERROR: configuration for channel < " << channel << " > not found." << endl;
            return false;
        }
        //---oversampling frequency in GHz
        auto fOversampling = opts.GetOpt<float>(channel+".fOversampling");
        auto tOversampling = 1./fOversampling;
        //---df = 1/t_max, t_max = tUnit*nSamples.
        oversamplingMap_[channel] = make_pair(fOversampling,
                                              opts.GetOpt<float>(channel+".tUnit")*opts.GetOpt<float>(channel+".nSamples"));
        WFs_[channel] = new WFClass(1, tOversampling);
        RegisterSharedData(WFs_[channel], channel+"_TMPL", false);
    }

    return true;
}

bool DFTTemplate::ProcessEvent(const H4Tree& event, map<string, PluginBase*>& plugins, CfgManager& opts)
{
    for(auto& channel : channelsNames_)
    {
        //---get FFT from source instance data and reset old WF
        WFs_[channel]->Reset();
        auto fft = (FFTClass*)plugins[srcInstance_]->GetSharedData(srcInstance_+"_"+channel, "", false).at(0).obj;
        
        //---build the FFT
        int n_samples = oversamplingMap_[channel].first*oversamplingMap_[channel].second;
        double data[n_samples];
        vector<double> Re, Im;
        Re.assign(fft->GetRe()->data(), fft->GetRe()->data()+fft->GetRe()->size());
        Im.assign(fft->GetIm()->data(), fft->GetIm()->data()+fft->GetIm()->size());        
        //---not so general FIXME
	cout << "fftGetRe " << fft->GetRe()->size() << endl;
	cout << "Limits " << Re.size()/2./oversamplingMap_[channel].second << " " << Re.size()/oversamplingMap_[channel].second << " " << Re.size()/oversamplingMap_[channel].first << endl;
        TH1D ampl_spectrum("ampl_spectrum", "",
                           Re.size()/2.,
                           Re.size()/2./oversamplingMap_[channel].second,
                           Re.size()/oversamplingMap_[channel].second);
        TF1 ampl_extrapolation("fextr", "expo", Re.size()/2./oversamplingMap_[channel].second, Re.size()/oversamplingMap_[channel].first);
        for(unsigned int i=1; i<=ampl_spectrum.GetNbinsX(); ++i)
            ampl_spectrum.SetBinContent(i, sqrt(pow(Re.at(Re.size()/2+i-1), 2)+pow(Im.at(Im.size()/2+i-1), 2)));
        ampl_spectrum.Fit(&ampl_extrapolation, "QRSO");
	cout << " No of samples " << n_samples << endl;
        while(Re.size() < n_samples/2)
        {
	  //cout << "Push Back " << ampl_extrapolation.Eval(Re.size()/oversamplingMap_[channel].second)/100000. << endl;
	  Re.push_back(ampl_extrapolation.Eval(Re.size()/oversamplingMap_[channel].second)/100000.);
	  Im.push_back(0.);
        }
        //---construct FFT and oversampled WF
        auto fftc2r = TVirtualFFT::FFT(1, &n_samples, "C2R");
        fftc2r->SetPointsComplex(Re.data(), Im.data());
        fftc2r->Transform();
        fftc2r->GetPoints(data);

        //---fill new, oversampled WF
        for(int iSample=0; iSample<n_samples; ++iSample)
            WFs_[channel]->AddSample(data[iSample]/n_samples);
        
        delete fftc2r;
    }
    
    return true;
}
