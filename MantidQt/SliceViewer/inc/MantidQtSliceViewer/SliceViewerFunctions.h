#ifndef SLICEVIEWER_FUNCTIONS_H
#define SLICEVIEWER_FUNCTIONS_H

#include "DllOption.h"
#include "MantidKernel/VMD.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"

/// Checks if a slice lies within a workspace or not
bool EXPORT_OPT_MANTIDQT_SLICEVIEWER doesSliceCutThroughWorkspace(const Mantid::Kernel::VMD& min, const  Mantid::Kernel::VMD& max,
  const std::vector<Mantid::Geometry::IMDDimension_sptr> dimensions);




#endif