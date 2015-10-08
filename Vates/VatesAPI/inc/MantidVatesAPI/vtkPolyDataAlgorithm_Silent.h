#ifndef VTKPOLYDATAALGORITHM_SILENT_H
#define VTKPOLYDATAALGORITHM_SILENT_H

#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 8 )
    #pragma GCC diagnostic push
  #endif
  #pragma GCC diagnostic ignored "-Wconversion"
#endif
#include <vtkPolyDataAlgorithm.h>
#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 8 )
    #pragma GCC diagnostic pop
  #endif
#endif

#endif // VTKPOLYDATAALGORITHM_SILENT_H
