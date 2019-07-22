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

enum PlotWidget { Spectra, SpectraBin, SpectraContour, SpectraTiled } const;

class MANTIDQT_INDIRECT_DLL IndirectPlotOptionsView : public API::MantidWidget {
  Q_OBJECT

public:
  IndirectPlotOptionsView(QWidget *parent = nullptr,
                          PlotWidget const &plotType = PlotWidget::Spectra);
  virtual ~IndirectPlotOptionsView() override = default;

  void setPlotType(PlotWidget const &plotType);
  void setWorkspaceComboBoxEnabled(bool enable);
  void setIndicesLineEditEnabled(bool enable);
  void setPlotButtonEnabled(bool enable);
  void setPlotButtonText(QString const &text);

  void setIndicesRegex(QString const &regex);

  QString selectedWorkspace() const;
  void setWorkspaces(std::vector<std::string> const &workspaces);

  int numberOfWorkspaces() const;

  void removeWorkspace(QString const &workspaceName);
  void clearWorkspaces();

  QString selectedIndices() const;
  void setIndices(QString const &indices);
  void setIndicesErrorLabelVisible(bool visible);

  void addIndicesSuggestion(QString const &spectra);

  void displayWarning(QString const &message);

signals:
  void selectedWorkspaceChanged(std::string const &workspaceName);
  void selectedIndicesChanged(std::string const &indices);
  void plotSpectraClicked();
  void plotBinsClicked();
  void plotContourClicked();
  void plotTiledClicked();

  void runAsPythonScript(QString const &code, bool noOutput = false);

private slots:
  void emitSelectedWorkspaceChanged(QString const &workspaceName);
  void emitSelectedIndicesChanged();
  void emitSelectedIndicesChanged(QString const &indices);
  void emitPlotSpectraClicked();
  void emitPlotBinsClicked();
  void emitPlotContourClicked();
  void emitPlotTiledClicked();

private:
  void setupView();
  QValidator *createValidator(QString const &regex);

  bool m_fixedIndices;

  std::unique_ptr<QStringListModel> m_suggestionsModel;
  std::unique_ptr<QCompleter> m_completer;
  std::unique_ptr<Ui::IndirectPlotOptions> m_plotOptions;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTPLOTOPTIONSVIEW_H_ */
