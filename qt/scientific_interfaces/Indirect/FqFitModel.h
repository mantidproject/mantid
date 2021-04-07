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

  void addWorkspace(const std::string &workspaceName) override;
  void removeWorkspace(TableDatasetIndex index) override;
  void setFitType(const std::string &fitType);

  bool zeroWidths(TableDatasetIndex dataIndex) const;
  bool zeroEISF(TableDatasetIndex dataIndex) const;

  bool isMultiFit() const override;

  std::string getFitParameterName(TableDatasetIndex dataIndex, WorkspaceIndex spectrum) const;
  std::vector<std::string> getWidths(TableDatasetIndex dataIndex) const;
  std::vector<std::string> getEISF(TableDatasetIndex dataIndex) const;
  boost::optional<std::size_t> getWidthSpectrum(std::size_t widthIndex, TableDatasetIndex dataIndex) const;
  boost::optional<std::size_t> getEISFSpectrum(std::size_t eisfIndex, TableDatasetIndex dataIndex) const;
  void setActiveWidth(std::size_t widthIndex, TableDatasetIndex dataIndex, bool single = true);
  void setActiveEISF(std::size_t eisfIndex, TableDatasetIndex dataIndex, bool single = true);

private:
  bool allWorkspacesEqual(const Mantid::API::MatrixWorkspace_sptr &workspace) const;
  FqFitParameters &addFqFitParameters(Mantid::API::MatrixWorkspace *workspace, const std::string &hwhmName);
  std::unordered_map<std::string, FqFitParameters>::const_iterator
  findFqFitParameters(TableDatasetIndex dataIndex) const;
  std::string getResultXAxisUnit() const override;
  std::string getResultLogName() const override;
  std::unordered_map<std::string, FqFitParameters> m_fqFitParameters;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
