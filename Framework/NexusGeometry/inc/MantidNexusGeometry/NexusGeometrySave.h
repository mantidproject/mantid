// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVE_H_
#define MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVE_H_

#include "H5Cpp.h"
#include "MantidNexusGeometry/DllConfig.h"
#include <H5Group.h>
#include <H5DataSet.h>
#include <H5File.h>
#include <H5Object.h>
#include <iostream>
#include <memory>

namespace Mantid {

namespace Kernel {

class ProgressBase;
} // namespace Kernel

namespace Geometry {
class ComponentInfo;
}

namespace NexusGeometry {

MANTID_NEXUSGEOMETRY_DLL class HDF5FileTestUtility { // __declspec(dllexport)
public:
  HDF5FileTestUtility(const std::string &H5FilePath);
  bool hasNxClass(std::string className, std::string HDF5Path) const;

private:
  H5::H5File m_file;
};

MANTID_NEXUSGEOMETRY_DLL void
saveInstrument(const Geometry::ComponentInfo &compInfo,
               const std::string &fullPath,
               Kernel::ProgressBase *reporter = nullptr);

} // namespace NexusGeometry
} // namespace Mantid

#endif /* MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVE_H_ */
