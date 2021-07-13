// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectFittingModel.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

struct FqFitParameters {
  std::vector<std::string> widths;
  std::vector<std::size_t> widthSpectra;
  std::vector<std::string> eisf;
  std::vector<std::size_t> eisfSpectra;
};

class MANTIDQT_INDIRECT_DLL FqFitModel : public IndirectFittingModel {
public:
  FqFitModel();
  using IndirectFittingModel::addWorkspace;
  void addWorkspace(const std::string &workspaceName, const int &spectrum_index);
  void addWorkspace(const std::string &workspaceName) override;
  void removeWorkspace(WorkspaceID workspaceID) override;

  bool isMultiFit() const override;

  std::string getFitParameterName(WorkspaceID dataIndex, WorkspaceIndex spectrum) const;
  std::vector<std::string> getWidths(WorkspaceID dataIndex) const;
  std::vector<std::string> getEISF(WorkspaceID dataIndex) const;
  boost::optional<std::size_t> getWidthSpectrum(std::size_t widthIndex, WorkspaceID dataIndex) const;
  boost::optional<std::size_t> getEISFSpectrum(std::size_t eisfIndex, WorkspaceID dataIndex) const;
  void setActiveWidth(std::size_t widthIndex, WorkspaceID dataIndex, bool single = true);
  void setActiveEISF(std::size_t eisfIndex, WorkspaceID dataIndex, bool single = true);
  FqFitParameters createFqFitParameters(Mantid::API::MatrixWorkspace *workspace);

private:
  bool allWorkspacesEqual(const Mantid::API::MatrixWorkspace_sptr &workspace) const;
  FqFitParameters &addFqFitParameters(Mantid::API::MatrixWorkspace *workspace, const std::string &hwhmName);
  std::unordered_map<std::string, FqFitParameters>::const_iterator findFqFitParameters(WorkspaceID workspaceID) const;
  std::string getResultXAxisUnit() const override;
  std::string getResultLogName() const override;
  std::unordered_map<std::string, FqFitParameters> m_fqFitParameters;
  Mantid::API::AnalysisDataServiceImpl &m_adsInstance;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
