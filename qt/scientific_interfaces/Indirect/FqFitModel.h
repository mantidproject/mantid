// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//#include "FqFitDataModel.h"
#include "IndirectFittingModel.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL FqFitModel : public IndirectFittingModel {
public:
  FqFitModel();
  void removeWorkspace(WorkspaceID workspaceID) override;

  bool isMultiFit() const override;

  std::string getFitParameterName(WorkspaceID dataIndex, WorkspaceIndex spectrum) const;

private:
  bool allWorkspacesEqual(const Mantid::API::MatrixWorkspace_sptr &workspace) const;
  std::string getResultXAxisUnit() const override;
  std::string getResultLogName() const override;
  Mantid::API::AnalysisDataServiceImpl &m_adsInstance;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
