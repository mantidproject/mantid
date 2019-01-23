// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VATES_CONFIGURATION_H
#define VATES_CONFIGURATION_H

#include "MantidKernel/System.h"
#include <string>

namespace Mantid {
namespace VATES {
/** Metadata container and handler to handle json data which is passed between
filters and sources through
    VTK field data

@date 1/12/2014
*/

class DLLExport VatesConfigurations {
public:
  VatesConfigurations();

  ~VatesConfigurations();

  int getMaxRecursionDepth();

  std::string getMetadataIdJson();

private:
  // The maximum recursion depth when going through the box tree.
  const int maxRecursionDepth;

  // Meta data field flag
  const std::string metaDataId;
};
} // namespace VATES
} // namespace Mantid
#endif