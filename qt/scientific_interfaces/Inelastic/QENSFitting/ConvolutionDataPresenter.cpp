// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ConvolutionDataPresenter.h"
#include "ConvolutionAddWorkspaceDialog.h"

namespace MantidQt::CustomInterfaces::Inelastic {

ConvolutionDataPresenter::ConvolutionDataPresenter(IFitTab *tab, IDataModel *model, IFitDataView *view)
    : FitDataPresenter(tab, model, view) {}

bool ConvolutionDataPresenter::addWorkspaceFromDialog(IAddWorkspaceDialog const *dialog) {
  if (const auto convDialog = dynamic_cast<ConvolutionAddWorkspaceDialog const *>(dialog)) {
    const auto &wsName = convDialog->workspaceName();
    const auto &spectra = convDialog->workspaceIndices();
    addWorkspace(wsName, spectra);
    setResolution(convDialog->resolutionName(), wsName, spectra);
    return true;
  }
  return false;
}

void ConvolutionDataPresenter::setResolution(const std::string &resName, const std::string &wsName,
                                             const FunctionModelSpectra &spectra) {
  if (!m_model->setResolution(resName, wsName, spectra)) {
    m_model->removeSpecialValues(resName);
    displayWarning("Replaced the NaN's and infinities in " + resName + " with zeros");
  }
}

void ConvolutionDataPresenter::addTableEntry(FitDomainIndex row) {
  const auto &name = m_model->getWorkspace(row)->getName();
  const auto workspaceIndex = m_model->getSpectrum(row);
  const auto range = m_model->getFittingRange(row);
  const auto exclude = m_model->getExcludeRegion(row);
  const auto resolution = m_model->getResolutionName(m_model->getWorkspaceID(name), workspaceIndex);

  FitDataRow newRow;
  newRow.name = name;
  newRow.workspaceIndex = workspaceIndex;
  newRow.resolution = resolution;
  newRow.startX = range.first;
  newRow.endX = range.second;
  newRow.exclude = exclude;
  m_view->addTableEntry(row.value, newRow);
}

} // namespace MantidQt::CustomInterfaces::Inelastic
