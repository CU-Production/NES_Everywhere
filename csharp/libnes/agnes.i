%module agnes

%include <csharp/typemaps.i>
%include <cdata.i>
%include <stdint.i>

%{
/* Includes the header in the wrapper code */
#include "agnes.h"
%}

/* Parse the header file to generate wrappers */
%include "agnes.h"