#include "LegacyVstAbi.h"
#include <vector>
#include <cmath>
#include <cstring>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
intptr_t host(legacy::Effect*, int32_t opcode, int32_t, intptr_t, void*, float) { return opcode==1 ? 2400 : 0; }
int main(int argc,char** argv) {
    if(argc!=2) return 1;
#ifdef _WIN32
    auto module=LoadLibraryA(argv[1]);
    if(!module) {const auto error=GetLastError();std::cerr<<"Cannot load DLL: "<<argv[1]<<" (Windows error "<<error<<"). Check workflow DLL filename and architecture.\n";return 2;}
    auto entry=module ? reinterpret_cast<legacy::Effect*(*)(legacy::Callback)>(GetProcAddress(module,"VSTPluginMain")) : nullptr;
    auto alias=module ? GetProcAddress(module,"main") : nullptr;
    if(!alias) {std::cerr<<"DLL loaded, but legacy main export is missing\n"; return 2;}
#else
    auto module=dlopen(argv[1],RTLD_NOW);
    auto entry=module ? reinterpret_cast<legacy::Effect*(*)(legacy::Callback)>(dlsym(module,"VSTPluginMain")) : nullptr;
#endif
    if(!entry) {std::cerr<<"Cannot load entry point\n";return 2;}
    if(entry(nullptr)!=nullptr) return 3;
    auto* e=entry(host);
    if(!e || e->magic!=0x56737450 || e->numParams<=0 || !e->processReplacing) return 4;
    e->dispatcher(e,0,0,0,nullptr,0);
    e->dispatcher(e,10,0,0,nullptr,48000);
    e->dispatcher(e,11,0,512,nullptr,0);
    e->dispatcher(e,12,0,1,nullptr,0);
    e->setParameter(e,0,0.35f);
    if(std::abs(e->getParameter(e,0)-0.35f)>0.001f) return 5;
    void* state=nullptr;
    auto size=e->dispatcher(e,23,0,0,&state,0);
    if(size<=0 || !state) return 6;
    std::vector<char> saved(static_cast<char*>(state),static_cast<char*>(state)+size);
    e->setParameter(e,0,0.9f);
    e->dispatcher(e,24,0,size,saved.data(),0);
    if(std::abs(e->getParameter(e,0)-0.35f)>0.001f) return 7;
    std::vector<float> a(1300),b(1300),outA(1300),outB(1300);
    for(size_t n=0;n<a.size();++n) a[n]=b[n]=0.1f*std::sin(n*0.03f);
    float* inputs[]{a.data(),b.data()}; float* outputs[]{outA.data(),outB.data()};
    e->processReplacing(e,inputs,outputs,1300);
    float energy=0;
    for(auto x:outA) { if(!std::isfinite(x)) return 8; energy+=std::abs(x); }
    if(energy<=0) return 9;
    e->dispatcher(e,12,0,0,nullptr,0);
    e->dispatcher(e,1,0,0,nullptr,0);
#ifdef _WIN32
    FreeLibrary(module);
#else
    dlclose(module);
#endif
    std::cout<<"PASS: native ABI, exports, parameters, state, audio, lifecycle\n";
}
