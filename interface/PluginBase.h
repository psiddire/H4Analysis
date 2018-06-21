#ifndef __PLUGIN_BASE__
#define __PLUGIN_BASE__

#include <string>
#include <map>

#include "TObject.h"

#include "CfgManager/interface/CfgManager.h"
#include "CfgManager/interface/CfgManagerT.h"
#include "interface/H4Tree.h"

using namespace std;

//**********SHARED DATAFORMAT*************************************************************
struct SharedData
{    
    bool     permanent;
    string   tag;
    TObject* obj;
};

//**********PLUGIN BASE CLASS*************************************************************
class PluginBase
{
public:
    //---ctors---
    PluginBase(){};

    //---dtor---
    virtual ~PluginBase(){};    

    //---setters---
    void SetInstanceName(const string& instance){instanceName_=instance;};    

    //---getters---
    string             GetInstanceName(){return instanceName_;};
    vector<SharedData> GetSharedData(string tag="", string type="", bool permanent=true);
    
    //---utils---
    virtual bool Begin(CfgManager& opts, uint64* index) { return true; };
    virtual bool ProcessEvent(const H4Tree& event, map<string, PluginBase*>& plugins, CfgManager& opts) { return true; };
    virtual bool End(CfgManager& opts) { return true; };

protected:
    //---utils---
    void RegisterSharedData(TObject* obj, string tag, bool isPermanent);

protected:
    //---keep track of the plugin name as defined in the cfg
    string instanceName_;
    //---shared data
    vector<SharedData> data_;
};

#define DEFINE_PLUGIN(NAME)                                         \
    extern "C" PluginBase* create() {return new NAME;}              \
    extern "C" void destroy(PluginBase* plugin) {delete plugin;}

#endif
