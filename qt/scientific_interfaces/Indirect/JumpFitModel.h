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
  std::vector<std::size_t> widthSpectra;
  std::vector<std::string> eisf;
  std::vector<std::size_t> eisfSpectra;
};

class DLLExport JumpFitModel : public IndirectFittingModel {
public:
  using IndirectFittingModel::addWorkspace;

  void addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                    const Spectra & /*spectra*/) override;
  void removeWorkspace(TableDatasetIndex index) override;
  void setFitType(const std::string &fitType);

  bool zeroWidths(TableDatasetIndex dataIndex) const;
  bool zeroEISF(TableDatasetIndex dataIndex) const;

  bool isMultiFit() const override;

  std::vector<std::string> getSpectrumDependentAttributes() const override;

  std::string getFitParameterName(TableDatasetIndex dataIndex,
                                  WorkspaceIndex spectrum) const;
  std::vector<std::string> getWidths(TableDatasetIndex dataIndex) const;
  std::vector<std::string> getEISF(TableDatasetIndex dataIndex) const;
  boost::optional<std::size_t>
  getWidthSpectrum(std::size_t widthIndex, TableDatasetIndex dataIndex) const;
  boost::optional<std::size_t>
  getEISFSpectrum(std::size_t eisfIndex, TableDatasetIndex dataIndex) const;
  void setActiveWidth(std::size_t widthIndex, TableDatasetIndex dataIndex);
  void setActiveEISF(std::size_t eisfIndex, TableDatasetIndex dataIndex);

  std::string sequentialFitOutputName() const override;
  std::string simultaneousFitOutputName() const override;
  std::string singleFitOutputName(TableDatasetIndex index,
                                  WorkspaceIndex spectrum) const override;

private:
  std::string constructOutputName() const;
  bool allWorkspacesEqual(Mantid::API::MatrixWorkspace_sptr workspace) const;
  JumpFitParameters &
  addJumpFitParameters(Mantid::API::MatrixWorkspace *workspace,
                       const std::string &hwhmName);
  std::unordered_map<std::string, JumpFitParameters>::const_iterator
  findJumpFitParameters(TableDatasetIndex dataIndex) const;
  std::string getResultXAxisUnit() const override;
  std::string getResultLogName() const override;

  std::string m_fitType;
  std::unordered_map<std::string, JumpFitParameters> m_jumpParameters;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
