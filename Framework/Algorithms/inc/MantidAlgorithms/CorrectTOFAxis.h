// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

#include <vector>

namespace Mantid {
namespace API {
class SpectrumInfo;
}

namespace Algorithms {

/** CorrectTOFAxis : Corrects the time-of-flight axis with regards to
  the incident energy and the L1+L2 distance or a reference workspace.
*/
class MANTID_ALGORITHMS_DLL CorrectTOFAxis : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"ConvertToConstantL2"}; }
  const std::string category() const override;
  const std::string summary() const override;

private:
  size_t m_elasticBinIndex = EMPTY_LONG();
  API::ITableWorkspace_const_sptr m_eppTable;
  API::MatrixWorkspace_const_sptr m_inputWs;
  API::MatrixWorkspace_const_sptr m_referenceWs;
  std::vector<size_t> m_workspaceIndices;

  void init() override;
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;
  void useReferenceWorkspace(const API::MatrixWorkspace_sptr &outputWs);
  void correctManually(const API::MatrixWorkspace_sptr &outputWs);
  double averageL2(const API::SpectrumInfo &spectrumInfo);
  void averageL2AndEPP(const API::SpectrumInfo &spectrumInfo, double &l2, double &epp);
  std::vector<size_t> referenceWorkspaceIndices() const;
};

} // namespace Algorithms
} // namespace Mantid
