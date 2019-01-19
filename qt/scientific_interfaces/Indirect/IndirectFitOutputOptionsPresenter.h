// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTFITOUTPUTOPTIONSPRESENTER_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTFITOUTPUTOPTIONSPRESENTER_H_

#include "IndirectFitOutputOptionsModel.h"
#include "IndirectFitOutputOptionsView.h"

#include "DllConfig.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectFitOutputOptionsPresenter : public QObject {
  Q_OBJECT
public:
  IndirectFitOutputOptionsPresenter(IndirectFitOutputOptionsView *view);
  ~IndirectFitOutputOptionsPresenter() override;

  void setPlotWorkspace(Mantid::API ::WorkspaceGroup_sptr workspace);
  void setPlotParameters(std::vector<std::string> const &parameterNames);

  void setPlotting(bool plotting);
  void setPlotEnabled(bool enable);
  void setSaveEnabled(bool enable);

  void clearSpectraToPlot();
  std::vector<SpectrumToPlot> getSpectraToPlot() const;

signals:
  void plotSpectra();

private slots:
  void plotResult();
  void saveResult();

private:
  void setSaving(bool saving);

  void displayWarning(std::string const &message);

  std::unique_ptr<IndirectFitOutputOptionsModel> m_model;
  std::unique_ptr<IndirectFitOutputOptionsView> m_view;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
