%module ai_libai
%include <od/glue/mod.cpp.swig>

%{

#undef SWIGLUA

#include <ai/PolyBLEP.h>

#define SWIGLUA

%}

%include <ai/PolyBLEP.h>