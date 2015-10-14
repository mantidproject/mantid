#ifndef VTKGLYPH3D_SILENT_H
#define VTKGLYPH3D_SILENT_H

#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 8 )
    #pragma GCC diagnostic push
  #endif
  #pragma GCC diagnostic ignored "-Wconversion"
#endif
#include <vtkGlyph3D.h>
#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 8 )
    #pragma GCC diagnostic pop
  #endif
#endif

#endif // VTKGLYPH3D_SILENT_H
