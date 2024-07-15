// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/GenerateGroupingPowder.h"
#include "MantidDataObjects/GroupingWorkspace.h"

namespace Mantid {
namespace DataHandling {

/** GenerateGroupingPowder2 : Generate grouping file and par file, for powder
  scattering.

  @date 2012-07-16
*/
class MANTID_DATAHANDLING_DLL GenerateGroupingPowder2 : public GenerateGroupingPowder {
public:
  int version() const override;

private:
  void saveGroups();
  void saveAsXML();
  void saveAsPAR();
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
