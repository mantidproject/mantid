#ifndef VTKIMAGEDATA_SILENT_H
#define VTKIMAGEDATA_SILENT_H

#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic push
  #endif
  #pragma GCC diagnostic ignored "-Wconversion"
#endif
#include <vtkImageData.h>
#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic pop
  #endif
#endif

#endif // VTKIMAGEDATA_SILENT_H
