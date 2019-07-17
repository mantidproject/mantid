// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTPLOTOPTIONSVIEW_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTPLOTOPTIONSVIEW_H_

#include "ui_IndirectPlotOptions.h"

#include "DllConfig.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/MantidWidget.h"

#include <memory>

#include <QCompleter>
#include <QStringList>
#include <QStringListModel>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

enum PlotWidget { Spectra, SpectraContour, SpectraTiled } const;
enum Plotting { None, Spectrum, Contour, Tiled } const;

class MANTIDQT_INDIRECT_DLL IndirectPlotOptionsView : public API::MantidWidget {
  Q_OBJECT

public:
  IndirectPlotOptionsView(QWidget *parent = nullptr,
                          PlotWidget const &plotType = PlotWidget::Spectra);
  virtual ~IndirectPlotOptionsView() override = default;

  void setPlotType(PlotWidget const &plotType);
  void setOptionsEnabled(Plotting const &type = Plotting::None,
                         bool enable = false);

  void setSpectraRegex(QString const &regex);

  void setSpectra(QString const &spectra);
  void setSpectraErrorLabelVisible(bool visible);

  void addSpectraSuggestion(QString const &spectra);

signals:
  void selectedSpectraChanged(std::string const &spectra);
  void plotSpectraClicked();
  void plotContourClicked();
  void plotTiledClicked();

  void runAsPythonScript(QString const &code, bool noOutput = false);

private slots:
  void emitSelectedSpectraChanged();
  void emitSelectedSpectraChanged(QString const &spectra);
  void emitPlotSpectraClicked();
  void emitPlotContourClicked();
  void emitPlotTiledClicked();

private:
  void setupView();
  QValidator *createValidator(QString const &regex);

  void setButtonText(Plotting const &type, bool enabled);

  QString selectedSpectra() const;

  std::unique_ptr<QStringListModel> m_suggestionsModel;
  std::unique_ptr<QCompleter> m_completer;
  std::unique_ptr<Ui::IndirectPlotOptions> m_plotOptions;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTPLOTOPTIONSVIEW_H_ */
