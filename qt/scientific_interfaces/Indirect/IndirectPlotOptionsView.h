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
#include <QHash>
#include <QStringList>
#include <QStringListModel>

namespace MantidQt {
namespace CustomInterfaces {

enum PlotWidget { Spectra, SpectraBin, SpectraContour, SpectraTiled };

class MANTIDQT_INDIRECT_DLL IndirectPlotOptionsView : public API::MantidWidget {
  Q_OBJECT

public:
  IndirectPlotOptionsView(QWidget *parent = nullptr);
  virtual ~IndirectPlotOptionsView() override;

  virtual void
  setPlotType(PlotWidget const &plotType,
              std::map<std::string, std::string> const &availableActions);
  virtual void setWorkspaceComboBoxEnabled(bool enable);
  virtual void setIndicesLineEditEnabled(bool enable);
  virtual void setPlotButtonEnabled(bool enable);
  void setPlotButtonText(QString const &text);

  virtual void setIndicesRegex(QString const &regex);

  QString selectedWorkspace() const;
  virtual void setWorkspaces(std::vector<std::string> const &workspaces);

  virtual int numberOfWorkspaces() const;

  void removeWorkspace(QString const &workspaceName);
  virtual void clearWorkspaces();

  QString selectedIndices() const;
  virtual void setIndices(QString const &indices);
  virtual void setIndicesErrorLabelVisible(bool visible);

  virtual void addIndicesSuggestion(QString const &spectra);

  virtual void displayWarning(QString const &message);

signals:
  void selectedWorkspaceChanged(std::string const &workspaceName);
  void selectedIndicesChanged(std::string const &indices);
  void plotSpectraClicked();
  void plotBinsClicked();
  void plotContourClicked();
  void plotTiledClicked();

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
