// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_PARAVIEW_VERSION_H_
#define MANTID_KERNEL_PARAVIEW_VERSION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include <string>

namespace Mantid {
namespace Kernel {
/** Class containing static methods to return the ParaView version number.
 */
class MANTID_KERNEL_DLL ParaViewVersion {
public:
  static std::string targetVersion(); ///< The version number major.minor

private:
  ParaViewVersion(); ///< Private, unimplemented constructor. Not a class that
  /// can be instantiated.
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_PARAVIEW_VERSION_H_ */
