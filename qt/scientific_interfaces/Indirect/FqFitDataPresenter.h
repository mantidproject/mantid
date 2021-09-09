// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "FqFitAddWorkspaceDialog.h"
#include "FqFitDataView.h"
#include "IFQFitObserver.h"
#include "IndirectFitDataPresenter.h"
#include "IndirectFunctionBrowser/SingleFunctionTemplateBrowser.h"
#include "Notifier.h"

#include <QComboBox>
#include <QSpacerItem>

namespace {
struct FqFitParameters {
  std::vector<std::string> widths;
  std::vector<std::size_t> widthSpectra;
  std::vector<std::string> eisf;
  std::vector<std::size_t> eisfSpectra;
};
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL FqFitDataPresenter : public IndirectFitDataPresenter {
  Q_OBJECT
public:
  FqFitDataPresenter(IIndirectFitDataModel *model, IIndirectFitDataView *view,
                     IFQFitObserver *SingleFunctionTemplateBrowser);
  void addWorkspace(const std::string &workspaceName, const std::string &paramType, const int &spectrum_index) override;
  void setActiveWidth(std::size_t widthIndex, WorkspaceID dataIndex, bool single = true) override;
  void setActiveEISF(std::size_t eisfIndex, WorkspaceID dataIndex, bool single = true) override;

private slots:
  void dialogParameterTypeUpdated(FqFitAddWorkspaceDialog *dialog, const std::string &type);
  void setDialogParameterNames(FqFitAddWorkspaceDialog *dialog, const std::string &workspace);
  void setActiveParameterType(const std::string &type);
  void updateActiveWorkspaceID();
  void updateActiveWorkspaceID(WorkspaceID index);

signals:
  void spectrumChanged(WorkspaceIndex);

protected:
  void addTableEntry(FitDomainIndex row) override;

private:
  std::unique_ptr<IAddWorkspaceDialog> getAddWorkspaceDialog(QWidget *parent) const override;
  void updateParameterOptions(FqFitAddWorkspaceDialog *dialog, const FqFitParameters &parameters);
  void updateParameterTypes(FqFitAddWorkspaceDialog *dialog, FqFitParameters &parameters);
  std::vector<std::string> getParameterTypes(FqFitParameters &parameters) const;
  void setActiveWorkspaceIDToCurrentWorkspace(IAddWorkspaceDialog const *dialog);

  std::string m_activeParameterType;
  WorkspaceID m_activeWorkspaceID;

  Notifier<IFQFitObserver> m_notifier;
  Mantid::API::AnalysisDataServiceImpl &m_adsInstance;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
