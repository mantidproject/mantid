// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/GroupingWorkspace.h"

namespace Mantid {
namespace DataHandling {

/** GenerateGroupingPowder : Generate grouping file and par file, for powder
  scattering.

  @date 2012-07-16
*/
class MANTID_DATAHANDLING_DLL GenerateGroupingPowder : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Generate grouping by angles."; }

  int version() const override;
  const std::string category() const override;
  const std::vector<std::string> seeAlso() const override { return {"LoadDetectorsGroupingFile", "GroupDetectors"}; }

  static std::string parFilenameFromXmlFilename(const std::string &filename);

protected:
  void init() override;

  void createGroups();
  void saveAsNexus();

  DataObjects::GroupingWorkspace_sptr m_groupWS;

private:
  void saveGroups();
  void saveAsXML();
  void saveAsPAR();

  void exec() override;

  std::map<std::string, std::string> validateInputs() override;
};

} // namespace DataHandling
} // namespace Mantid
