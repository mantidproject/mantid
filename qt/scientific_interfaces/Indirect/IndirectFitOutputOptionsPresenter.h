// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTFITOUTPUTOPTIONSPRESENTER_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTFITOUTPUTOPTIONSPRESENTER_H_

#include "IIndirectFitOutputOptionsView.h"
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
  IndirectFitOutputOptionsPresenter(IIndirectFitOutputOptionsModel *model,
                                    IIndirectFitOutputOptionsView *view);
  ~IndirectFitOutputOptionsPresenter() override;

  void setMultiWorkspaceOptionsVisible(bool visible);

  void setResultWorkspace(Mantid::API ::WorkspaceGroup_sptr groupWorkspace);
  void setPDFWorkspace(Mantid::API ::WorkspaceGroup_sptr groupWorkspace);
  void setPlotWorkspaces();
  void setPlotTypes(std::string const &selectedGroup);

  void removePDFWorkspace();

  bool isResultGroupPlottable();
  bool isPDFGroupPlottable();

  void setPlotting(bool plotting);
  void setPlotEnabled(bool enable);
  void setSaveEnabled(bool enable);

  void clearSpectraToPlot();
  std::vector<SpectrumToPlot> getSpectraToPlot() const;

signals:
  void plotSpectra();

private slots:
  void setAvailablePlotOptions(std::string const &selectedGroup);
  void plotResult();
  void saveResult();

private:
  void plotResult(std::string const &selectedGroup);
  void setSaving(bool saving);

  void displayWarning(std::string const &message);

  IIndirectFitOutputOptionsModel *m_model;
  IIndirectFitOutputOptionsView *m_view;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
