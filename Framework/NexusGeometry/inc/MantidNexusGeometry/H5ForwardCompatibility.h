// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDNEXUSGEOMETRY_H5FORWARDCOMPATIBILITY
#define MANTIDNEXUSGEOMETRY_H5FORWARDCOMPATIBILITY

#include "MantidNexusGeometry/DllConfig.h"
#include <H5Cpp.h>
#include <string>

// use forward compatibility for lower than 1.8.13 versions of HDF5
#if (H5_VERS_MAJOR >= 1 && H5_VERS_MINOR >= 8 && H5_VERS_RELEASE >= 13)
#define H5_OBJ_NAME(obj) obj.getObjName()
#else
#define H5_OBJ_NAME(obj)                                                       \
  Mantid::NexusGeometry::H5ForwardCompatibility::getObjName(obj)
#endif

namespace Mantid {
namespace NexusGeometry {
namespace H5ForwardCompatibility {

ssize_t getObjName(const H5::H5Object &obj, char *obj_name, size_t buf_size);
std::string getObjName(const H5::H5Object &obj);

} // namespace H5ForwardCompatibility
} // namespace NexusGeometry
} // namespace Mantid

#endif // MANTIDNEXUSGEOMETRY_H5FORWARDCOMPATIBILITY
