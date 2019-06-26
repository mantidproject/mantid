// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_JUMPFITMODEL_H_
#define MANTIDQTCUSTOMINTERFACESIDA_JUMPFITMODEL_H_

#include "IndirectFittingModel.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

struct JumpFitParameters {
  std::vector<std::string> widths;
  std::vector<WorkspaceIndex> widthSpectra;
  std::vector<std::string> eisf;
  std::vector<WorkspaceIndex> eisfSpectra;
};

class DLLExport JumpFitModel : public IndirectFittingModel {
public:
  using IndirectFittingModel::addWorkspace;

  void addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                    const Spectra & /*spectra*/) override;
  void removeWorkspace(DatasetIndex index) override;
  void setFitType(const std::string &fitType);

  bool zeroWidths(DatasetIndex dataIndex) const;
  bool zeroEISF(DatasetIndex dataIndex) const;

  bool isMultiFit() const override;

  std::vector<std::string> getSpectrumDependentAttributes() const override;

  std::string getFitParameterName(DatasetIndex dataIndex, WorkspaceIndex spectrum) const;
  std::vector<std::string> getWidths(DatasetIndex dataIndex) const;
  std::vector<std::string> getEISF(DatasetIndex dataIndex) const;
  boost::optional<WorkspaceIndex> getWidthSpectrum(std::size_t widthIndex,
                                                DatasetIndex dataIndex) const;
  boost::optional<WorkspaceIndex> getEISFSpectrum(std::size_t eisfIndex,
                                               DatasetIndex dataIndex) const;
  void setActiveWidth(std::size_t widthIndex, DatasetIndex dataIndex);
  void setActiveEISF(std::size_t eisfIndex, DatasetIndex dataIndex);

  std::string sequentialFitOutputName() const override;
  std::string simultaneousFitOutputName() const override;
  std::string singleFitOutputName(DatasetIndex index,
    WorkspaceIndex spectrum) const override;

private:
  std::string constructOutputName() const;
  bool allWorkspacesEqual(Mantid::API::MatrixWorkspace_sptr workspace) const;
  JumpFitParameters &
  addJumpFitParameters(Mantid::API::MatrixWorkspace *workspace,
                       const std::string &hwhmName);
  std::unordered_map<std::string, JumpFitParameters>::const_iterator
  findJumpFitParameters(DatasetIndex dataIndex) const;
  std::string getResultXAxisUnit() const override;

  std::string m_fitType;
  std::unordered_map<std::string, JumpFitParameters> m_jumpParameters;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
