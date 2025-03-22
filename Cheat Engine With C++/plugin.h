#pragma once
#include <mutex>
#include <vector>
#include <minwindef.h>
#include <minwinbase.h>



struct TPlugin {
    std::string dllname;
    std::string filepath;
    HANDLE hmodule;
    std::string name;
    int pluginversion;
    bool dotnet;
    bool enabled;
    TGetVersion GetVersion;
    TInitializePlugin EnablePlugin;
    TDisablePlugin DisablePlugin;
    int nextid;
    std::vector<TPluginfunctionType0> RegisteredFunctions0;
    std::vector<TPluginfunctionType1> RegisteredFunctions1;
    std::vector<TPluginfunctionType2> RegisteredFunctions2;
    std::vector<TPluginfunctionType3> RegisteredFunctions3;
    std::vector<TPluginfunctionType4> RegisteredFunctions4;
    std::vector<TPluginfunctionType5> RegisteredFunctions5;
    std::vector<TPluginfunctionType6> RegisteredFunctions6;
    std::vector<TPluginfunctionType7> RegisteredFunctions7;
};

class plugin {
private:
    std::recursive_mutex pluginCS;
    std::vector<TPlugin*> plugins;

    std::string GetDLLFilePath(int pluginid);
    std::string DotNetPluginGetPluginName(const std::string& dllname);
    int DotNetLoadPlugin(const std::string& dllname);

public:
    int GetPluginID(const std::string& dllname);
    std::string GetPluginName(const std::string& dllname);
    int LoadPlugin(const std::string& dllname);
    void UnloadPlugin(int pluginid);
    void FillCheckListBox(TCheckListbox* clb);
    void EnablePlugin(int pluginid);
    void DisablePlugin(int pluginid);
    void handleAutoAssemblerPlugin(char*** line, int phase, int id);
    void handledisassemblerContextPopup(uintptr_t* address);
    void handledisassemblerplugins(uintptr_t* address, void* addressStringPointer, void* bytestringpointer,
        void* opcodestringpointer, void* specialstringpointer, Color* textcolor);
    int handledebuggerplugins(DEBUG_EVENT devent);
    bool handlenewprocessplugins(DWORD processid, uintptr_t* peprocess, bool created);
    bool handlechangedpointers(int section);
    int registerfunction(int pluginid, int functiontype, void* init);
    bool unregisterfunction(int pluginid, int functionid);


    plugin();
    ~plugin();
};