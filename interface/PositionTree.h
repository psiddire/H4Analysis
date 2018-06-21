#ifndef __POSITION_TREE__
#define __POSITION_TREE__

#include <string>
#include <vector>
#include <map>

#include "TFile.h"
#include "TTree.h"

typedef unsigned long long int uint64;

using namespace std;

class PositionTree
{
public: 
    //---ctors---
    PositionTree(){};
    PositionTree(uint64* idx, TTree* tree=NULL, int nPlanes=1);
    //---dtor---
    ~PositionTree(){};
    
    //---utils---
    void Init();
    void Fill() {tree_->Fill();};

    TTree*  tree_; 

    uint64* index;
    int n_planes;
    int n_hitsX;
    int n_hitsY;
    int*  nFibresOnX;
    int*  nFibresOnY;
    float* X;
    float* Y;
    //    int* fibers;
};

#endif
