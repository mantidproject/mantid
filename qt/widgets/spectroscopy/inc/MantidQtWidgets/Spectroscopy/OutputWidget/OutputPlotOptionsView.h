// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ui_OutputPlotOptions.h"

#include "../DllConfig.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/MantidWidget.h"

#include <memory>

#include <QCompleter>
#include <QHash>
#include <QStringList>
#include <QStringListModel>

namespace MantidQt {
namespace CustomInterfaces {

class IOutputPlotOptionsPresenter;

enum PlotWidget { Spectra, SpectraBin, SpectraSliceSurface, SpectraTiled, SpectraUnit, SpectraSliceSurfaceUnit };

class MANTID_SPECTROSCOPY_DLL IOutputPlotOptionsView {
public:
  virtual void subscribePresenter(IOutputPlotOptionsPresenter *presenter) = 0;

  virtual void setPlotType(PlotWidget const &plotType, std::map<std::string, std::string> const &availableActions) = 0;
  virtual void setWorkspaceComboBoxEnabled(bool enable) = 0;
  virtual void setUnitComboBoxEnabled(bool enable) = 0;
  virtual void setIndicesLineEditEnabled(bool enable) = 0;
  virtual void setPlotButtonEnabled(bool enable) = 0;
  virtual void setPlotButtonText(QString const &text) = 0;

  virtual void setIndicesRegex(QString const &regex) = 0;

  virtual QString selectedWorkspace() const = 0;
  virtual void setWorkspaces(std::vector<std::string> const &workspaces) = 0;

  virtual int numberOfWorkspaces() const = 0;

  virtual void removeWorkspace(QString const &workspaceName) = 0;
  virtual void clearWorkspaces() = 0;

  virtual QString selectedIndices() const = 0;
  virtual void setIndices(QString const &indices) = 0;
  virtual void setIndicesErrorLabelVisible(bool visible) = 0;

  virtual void addIndicesSuggestion(QString const &spectra) = 0;

  virtual void displayWarning(QString const &message) = 0;
};

class MANTID_SPECTROSCOPY_DLL OutputPlotOptionsView final : public API::MantidWidget, public IOutputPlotOptionsView {
  Q_OBJECT

public:
  OutputPlotOptionsView(QWidget *parent = nullptr);
  ~OutputPlotOptionsView() = default;

  void subscribePresenter(IOutputPlotOptionsPresenter *presenter) override;

  void setPlotType(PlotWidget const &plotType, std::map<std::string, std::string> const &availableActions) override;
  void setWorkspaceComboBoxEnabled(bool enable) override;
  void setUnitComboBoxEnabled(bool enable) override;
  void setIndicesLineEditEnabled(bool enable) override;
  void setPlotButtonEnabled(bool enable) override;
  void setPlotButtonText(QString const &text) override;

  void setIndicesRegex(QString const &regex) override;

  QString selectedWorkspace() const override;
  void setWorkspaces(std::vector<std::string> const &workspaces) override;

  int numberOfWorkspaces() const override;

  void removeWorkspace(QString const &workspaceName) override;
  void clearWorkspaces() override;

  QString selectedIndices() const override;
  void setIndices(QString const &indices) override;
  void setIndicesErrorLabelVisible(bool visible) override;

  void addIndicesSuggestion(QString const &spectra) override;

  void displayWarning(QString const &message) override;

private slots:
  void notifySelectedWorkspaceChanged(QString const &workspaceName);
  void notifySelectedUnitChanged(QString const &unit);
  void notifySelectedIndicesChanged();
  void notifySelectedIndicesChanged(QString const &indices);
  void notifyPlotSpectraClicked();
  void notifyPlotBinsClicked();
  void notifyShowSliceViewerClicked();
  void notifyPlotTiledClicked();
  void notifyPlot3DClicked();

private:
  void setupView();
  QValidator *createValidator(QString const &regex);

  bool m_fixedIndices;

  std::unique_ptr<QStringListModel> m_suggestionsModel;
  std::unique_ptr<QCompleter> m_completer;
  std::unique_ptr<Ui::OutputPlotOptions> m_plotOptions;

  IOutputPlotOptionsPresenter *m_presenter;
};

} // namespace CustomInterfaces
} // namespace MantidQt
