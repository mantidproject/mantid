// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTFITOUTPUTOPTIONSPRESENTER_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTFITOUTPUTOPTIONSPRESENTER_H_

#include "IIndirectFitOutputOptionsView.h"
#include "IndirectEditResultsDialog.h"
#include "IndirectFitOutputOptionsModel.h"

#include "DllConfig.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectFitOutputOptionsPresenter : public QObject {
  Q_OBJECT
public:
  IndirectFitOutputOptionsPresenter(IIndirectFitOutputOptionsView *view);
  IndirectFitOutputOptionsPresenter(IIndirectFitOutputOptionsModel *model,
                                    IIndirectFitOutputOptionsView *view);
  ~IndirectFitOutputOptionsPresenter() override;

  void setMultiWorkspaceOptionsVisible(bool visible);

  void setResultWorkspace(Mantid::API ::WorkspaceGroup_sptr groupWorkspace);
  void setPDFWorkspace(Mantid::API ::WorkspaceGroup_sptr groupWorkspace);
  void setPlotWorkspaces();
  void setPlotTypes(std::string const &selectedGroup);

  void removePDFWorkspace();

  bool isSelectedGroupPlottable() const;

  void setPlotting(bool plotting);
  void setPlotEnabled(bool enable);
  void setEditResultEnabled(bool enable);
  void setSaveEnabled(bool enable);

  void clearSpectraToPlot();
  std::vector<SpectrumToPlot> getSpectraToPlot() const;

  void setEditResultVisible(bool visible);

signals:
  void plotSpectra();

private slots:
  void setAvailablePlotOptions(std::string const &selectedGroup);
  void plotResult();
  void saveResult();
  void editResult();
  void replaceSingleFitResult();
  void closeEditResultDialog();

private:
  void setUpPresenter();

  void plotResult(std::string const &selectedGroup);
  void setSaving(bool saving);

  std::unique_ptr<IndirectEditResultsDialog>
  getEditResultsDialog(QWidget *parent) const;
  void setEditingResult(bool editing);

  void replaceSingleFitResult(std::string const &inputName,
                              std::string const &singleBinName,
                              std::string const &outputName);

  void displayWarning(std::string const &message);

  std::unique_ptr<IndirectEditResultsDialog> m_editResultsDialog;
  std::unique_ptr<IIndirectFitOutputOptionsModel> m_model;
  IIndirectFitOutputOptionsView *m_view;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
