// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTPLOTOPTIONSPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTPLOTOPTIONSPRESENTER_H_

#include "IndirectPlotOptionsModel.h"
#include "IndirectPlotOptionsView.h"
#include "IndirectTab.h"

#include "DllConfig.h"

#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectPlotOptionsPresenter : public QObject {
  Q_OBJECT

public:
  IndirectPlotOptionsPresenter(
      IndirectPlotOptionsView *view, IndirectTab *parent,
      PlotWidget const &plotType = PlotWidget::Spectra);
  ~IndirectPlotOptionsPresenter() override = default;

  void setWorkspace(std::string const &plotWorkspace);
  void removeWorkspace();

signals:
  void runAsPythonScript(QString const &code, bool noOutput = false);

private slots:
  void spectraChanged(std::string const &spectra);
  void plotSpectra();
  void plotContour();
  void plotTiled();

private:
  void setupPresenter();

  void setSpectra();

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  void runPythonCode(boost::optional<std::string> const &plotString);
#endif

  API::PythonRunner m_pythonRunner;
  std::unique_ptr<IndirectPlotOptionsView> m_view;
  std::unique_ptr<IndirectPlotOptionsModel> m_model;
  IndirectTab *m_parentTab;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTPLOTOPTIONSPRESENTER_H_ */
