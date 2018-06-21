#include "HodoReco.h"

//**********Utils*************************************************************************
//----------Begin*************************************************************************
bool HodoReco::Begin(CfgManager& opts, uint64* index)
{
    hodoFiberOrderA_.clear();
    hodoFiberOrderB_.clear();
  
    hodoFiberOrderA_.push_back(31);
    hodoFiberOrderA_.push_back(29);
    hodoFiberOrderA_.push_back(23);
    hodoFiberOrderA_.push_back(21);
    hodoFiberOrderA_.push_back(5);
    hodoFiberOrderA_.push_back(7);
    hodoFiberOrderA_.push_back(15);
    hodoFiberOrderA_.push_back(13);
    hodoFiberOrderA_.push_back(1);
    hodoFiberOrderA_.push_back(3);
    hodoFiberOrderA_.push_back(11);
    hodoFiberOrderA_.push_back(9);
    hodoFiberOrderA_.push_back(6);
    hodoFiberOrderA_.push_back(8);
    hodoFiberOrderA_.push_back(16);
    hodoFiberOrderA_.push_back(14);
    hodoFiberOrderA_.push_back(17);
    hodoFiberOrderA_.push_back(27);
    hodoFiberOrderA_.push_back(19);
    hodoFiberOrderA_.push_back(25);
    hodoFiberOrderA_.push_back(24);
    hodoFiberOrderA_.push_back(22);
    hodoFiberOrderA_.push_back(32);
    hodoFiberOrderA_.push_back(30);
    hodoFiberOrderA_.push_back(4);
    hodoFiberOrderA_.push_back(2);
    hodoFiberOrderA_.push_back(12);
    hodoFiberOrderA_.push_back(10);
    hodoFiberOrderA_.push_back(20);
    hodoFiberOrderA_.push_back(18);
    hodoFiberOrderA_.push_back(28);
    hodoFiberOrderA_.push_back(26);

    hodoFiberOrderB_.push_back(54);
    hodoFiberOrderB_.push_back(64);
    hodoFiberOrderB_.push_back(56);
    hodoFiberOrderB_.push_back(62);
    hodoFiberOrderB_.push_back(49);
    hodoFiberOrderB_.push_back(51);
    hodoFiberOrderB_.push_back(59);
    hodoFiberOrderB_.push_back(57);
    hodoFiberOrderB_.push_back(53);
    hodoFiberOrderB_.push_back(55);
    hodoFiberOrderB_.push_back(63);
    hodoFiberOrderB_.push_back(61);
    hodoFiberOrderB_.push_back(45);
    hodoFiberOrderB_.push_back(47);
    hodoFiberOrderB_.push_back(37);
    hodoFiberOrderB_.push_back(39);
    hodoFiberOrderB_.push_back(34);
    hodoFiberOrderB_.push_back(42);
    hodoFiberOrderB_.push_back(36);
    hodoFiberOrderB_.push_back(44);
    hodoFiberOrderB_.push_back(50);
    hodoFiberOrderB_.push_back(52);
    hodoFiberOrderB_.push_back(58);
    hodoFiberOrderB_.push_back(60);
    hodoFiberOrderB_.push_back(38);
    hodoFiberOrderB_.push_back(48);
    hodoFiberOrderB_.push_back(40);
    hodoFiberOrderB_.push_back(46);
    hodoFiberOrderB_.push_back(41);
    hodoFiberOrderB_.push_back(43);
    hodoFiberOrderB_.push_back(33);
    hodoFiberOrderB_.push_back(35);

    //---create a position tree
    bool storeTree = opts.OptExist(instanceName_+".storeTree") ?
        opts.GetOpt<bool>(instanceName_+".storeTree") : true;
    RegisterSharedData(new TTree("hodo", "hodo_tree"), "hodo_tree", storeTree);
    hodoTree_ = PositionTree(index, (TTree*)data_.back().obj, 2);
    hodoTree_.Init();
    
    return true;
}

bool HodoReco::ProcessEvent(const H4Tree& h4Tree, map<string, PluginBase*>& plugins, CfgManager& opts)
{
    std::map<int,std::map<int,bool> > hodoFiberOn;
    
    for(int i=0; i<nPlanes_; ++i)
        for(int j=0; j<nFibers_; ++j)
            hodoFiberOn[i][j] = 0;
  
    std::map<int,std::vector<int> > fibersOn;
  
    for(unsigned int i=0; i<h4Tree.nPatterns; ++i)
    {
        if(h4Tree.patternBoard[i] == 0x08020001 ||
           h4Tree.patternBoard[i] == 0x08020002)
        {
            int pos = -1; // here is where the real hodoscope mapping is done
      
            if(h4Tree.patternBoard[i] == 0x08020001)
                pos = (h4Tree.patternChannel[i]<2) ? HODO_Y2 : HODO_X2;
            else if(h4Tree.patternBoard[i] == 0x08020002)
                pos = (h4Tree.patternChannel[i]<2) ? HODO_Y1 : HODO_X1;
      
            std::vector<int>* fiberorder = (bool)(h4Tree.patternChannel[i]&0b1) ? &hodoFiberOrderB_ : &hodoFiberOrderA_;
      
            for(unsigned int j=0; j<32; ++j)
            {
                bool thisfibon = (h4Tree.pattern[i]>>j)&0b1;
                hodoFiberOn[pos][fiberorder->at(j)-1] = thisfibon;
                if(thisfibon) fibersOn[pos].push_back(fiberorder->at(j)-1);

            }
        }
    }

    for(int i=0; i<nPlanes_*2; ++i)
    {
        float value=-999;
        float offset=0;
        if(opts.OptExist(instanceName_+".hodoCorrection.hodoAlignOffset",i))
            offset=opts.GetOpt<float>("H4Hodo.hodoCorrection.hodoAlignOffset",i);

	//hodoTree_.fibers[i] = fibersOn[i].size(); 
        if(fibersOn[i].size() == 1)
            value = 0.5 * (fibersOn[i].at(0) - 32.);
        else if(fibersOn[i].size() == 2)
        {
            float val1 = fibersOn[i].at(0);
            float val2 = fibersOn[i].at(1);
            if(fabs(val2-val1) == 1)                
                value = 0.5 * (0.5*(val1+val2) - 32.);
        }   
        else if(fibersOn[i].size() == 3)
        {
            std::vector<float> vals;
            vals.push_back(fibersOn[i].at(0));
            vals.push_back(fibersOn[i].at(1));
            vals.push_back(fibersOn[i].at(2));
            std::sort(vals.begin(),vals.end());
            if(fabs(vals.at(1)-vals.at(0)) == 1 && fabs(vals.at(2)-vals.at(1)) == 1) 
                value = 0.5 * (vals.at(1) - 32.);
        }
	if ((value + offset) > -25 && (value + offset) < 25){
	  if(i%2 == 0){
            hodoTree_.X[i/nPlanes_] = value + offset;
            hodoTree_.nFibresOnX[i/nPlanes_]=fibersOn[i].size();	
	  } else{
            hodoTree_.Y[i/nPlanes_] = value + offset;
            hodoTree_.nFibresOnY[i/nPlanes_]=fibersOn[i].size();	
	  }
	}
    }
    //---fill output tree
    hodoTree_.Fill();
    
    return true;
}
