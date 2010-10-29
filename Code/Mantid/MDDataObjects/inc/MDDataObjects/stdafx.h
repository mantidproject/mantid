// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#ifndef H_STDAFX_VADDS
#define H_STDAFX_VADDS


#include <stdio.h>
#include <iostream>
#include <vector>
#include <list>
#include <fstream>
#include <sstream>
#include <string>
#include <cfloat>
#include <algorithm>
#include <limits>
#include <time.h>
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidAPI/Workspace.h"

#ifdef __GNUC__
#   if __GNUC__ <= 4
#		 if __GNUC_MINOR__ < 2  // then the compiler do not undertand OpenMP functions, let's define them
void omp_set_num_threads(int nThreads){};
#define  omp_get_num_threads() 1
#		endif
#	endif
#endif

template <class T>
bool isNaN(T val){
    volatile T buf=val;
    return (val!=buf);
}

#endif