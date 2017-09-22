#ifndef VTKSTRUCTUREDGRID_SILENT_H
#define VTKSTRUCTUREDGRID_SILENT_H

#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wconversion"
#endif
#include <vtkStructuredGrid.h>
#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#endif

#endif // VTKSTRUCTUREDGRID_SILENT_H
