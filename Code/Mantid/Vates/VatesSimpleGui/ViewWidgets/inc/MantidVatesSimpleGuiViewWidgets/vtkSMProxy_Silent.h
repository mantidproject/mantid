#ifndef VTKSMPROXY_SILENT_H
#define VTKSMPROXY_SILENT_H

#if defined(__INTEL_COMPILER)
  #pragma warning disable 1170
#endif
#include <vtkSMProxy.h>
#if defined(__INTEL_COMPILER)
  #pragma warning enable 1170
#endif

#endif // VTKSMPROXY_SILENT_H
