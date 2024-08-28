// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace DataHandling {

/** ApplyDiffCal :
 */
class MANTID_DATAHANDLING_DLL ApplyDiffCal final : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"ConvertDiffCal", "ConvertUnits", "LoadDiffCal"}; }
  const std::string category() const override;
  const std::string summary() const override;
  /// Cross-check properties with each other @see IAlgorithm::validateInputs
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;

  void loadCalFile(const Mantid::API::Workspace_sptr &inputWS, const std::string &filename);
  void getCalibrationWS(const Mantid::API::Workspace_sptr &inputWS);

  Mantid::API::ITableWorkspace_sptr m_calibrationWS;
};

} // namespace DataHandling
} // namespace Mantid
