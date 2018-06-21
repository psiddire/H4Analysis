#include "interface/FFTClass.h"

//----------Constructors------------------------------------------------------------------
FFTClass::FFTClass()
{
    vars_["ampl"];
    vars_["phase"];
    vars_["re"];
    vars_["im"];
}

FFTClass::FFTClass(int N, double* Re, double* Im)
{
    vars_["ampl"];
    vars_["phase"];
    SetPointsComplex(N, Re, Im);
}

FFTClass::FFTClass(vector<double>& Re, vector<double>& Im)
{
    vars_["ampl"];
    vars_["phase"];
    SetPointsComplex(Re, Im);
}

//----------Getters-----------------------------------------------------------------------
const vector<double>* FFTClass::Get(string var)
{
    if(var == "ampl")
        return GetAmplitudes();
    if(var == "phase")
        return GetPhases();

    return &vars_[var];
}

const vector<double>* FFTClass::GetAmplitudes()
{
    //---first check if amplitudes vector is already full
    //   if not fill it
    if(vars_["ampl"].size() == 0)
        for(unsigned int i=0; i<vars_["re"].size(); ++i)
            vars_["ampl"].push_back(sqrt(pow(vars_["re"][i], 2) + pow(vars_["im"][i], 2)));

    return &vars_["ampl"];
}

const vector<double>* FFTClass::GetPhases()
{
    //---first check if phases vector is already full
    //   if not fill it
    if(vars_["phase"].size() == 0)
    {
        for(unsigned int i=0; i<vars_["re"].size(); ++i)
            vars_["phase"].push_back(atan2(vars_["im"][i], vars_["re"][i]));
    }

    return &vars_["phase"];
}

//----------Setters-----------------------------------------------------------------------
void FFTClass::SetPointsComplex(int N, double* Re, double* Im)
{
    for(int i=0; i<N; ++i)
    {
        vars_["re"].push_back(Re[i]);
        vars_["im"].push_back(Im[i]);
    }
}

void FFTClass::SetPointsComplex(vector<double>& Re, vector<double>& Im)
{
    vars_["re"] = Re;
    vars_["im"] = Im;
}

void FFTClass::Reset()
{
    for(auto& var : vars_)
        var.second.clear();
}
