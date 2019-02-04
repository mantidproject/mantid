// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesAPI/VatesConfigurations.h"

namespace Mantid {
namespace VATES {
/** Metadata container and handler to handle json data which is passed between
filters and sources through
    VTK field data

@date 31/11/2014
*/

VatesConfigurations::VatesConfigurations()
    : maxRecursionDepth(10000), metaDataId("VATES_Metadata_Json") {}

VatesConfigurations::~VatesConfigurations() {}

int VatesConfigurations::getMaxRecursionDepth() {
  return this->maxRecursionDepth;
}

std::string VatesConfigurations::getMetadataIdJson() {
  return this->metaDataId;
}
} // namespace VATES
} // namespace Mantid