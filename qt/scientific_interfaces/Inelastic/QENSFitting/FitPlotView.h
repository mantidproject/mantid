// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ui_FitPreviewPlot.h"

#include "DllConfig.h"
#include "IFitPlotView.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/MantidWidget.h"
#include "MantidQtWidgets/Plotting/PreviewPlot.h"

#include <QIcon>
#include <QPainter>
#include <QSplitter>
#include <QSplitterHandle>
#include <utility>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

// Used for painting an Icon onto the handle of the splitter on workbench
class SplitterHandle : public QSplitterHandle {
public:
  SplitterHandle(QIcon icon, Qt::Orientation orientation, QSplitter *parent = nullptr)
      : QSplitterHandle(orientation, parent), m_icon(std::move(icon)) {}

  void paintEvent(QPaintEvent *e) override {
    QSplitterHandle::paintEvent(e);

    QPainter painter(this);
    auto const xPos = static_cast<int>(std::round(this->size().width() / 2));
    m_icon.paint(&painter, xPos, -9, 24, 24);
  }

private:
  QIcon m_icon;
};

class Splitter : public QSplitter {
public:
  Splitter(QIcon icon, QWidget *parent = nullptr) : QSplitter(parent), m_icon(std::move(icon)) {}

  QSplitterHandle *createHandle() override { return new SplitterHandle(m_icon, Qt::Vertical, this); }

private:
  QIcon m_icon;
};

class MANTIDQT_INELASTIC_DLL FitPlotView final : public API::MantidWidget, public IFitPlotView {
  Q_OBJECT

public:
  FitPlotView(QWidget *parent = nullptr);
  ~FitPlotView();

  void subscribePresenter(IFitPlotPresenter *presenter) override;

  void watchADS(bool watch) override;

  WorkspaceIndex getSelectedSpectrum() const override;
  WorkspaceID getSelectedDataIndex() const override;
  WorkspaceID dataSelectionSize() const override;
  bool isPlotGuessChecked() const override;

  void setAvailableSpectra(WorkspaceIndex minimum, WorkspaceIndex maximum) override;
  void setAvailableSpectra(const std::vector<WorkspaceIndex>::const_iterator &from,
                           const std::vector<WorkspaceIndex>::const_iterator &to) override;

  void setMinimumSpectrum(int minimum) override;
  void setMaximumSpectrum(int maximum) override;
  void setPlotSpectrum(WorkspaceIndex spectrum) override;
  void appendToDataSelection(const std::string &dataName) override;
  void setNameInDataSelection(const std::string &dataName, WorkspaceID workspaceID) override;
  void clearDataSelection() override;

  void plotInTopPreview(const QString &name, Mantid::API::MatrixWorkspace_sptr workspace, WorkspaceIndex spectrum,
                        Qt::GlobalColor colour) override;
  void plotInBottomPreview(const QString &name, Mantid::API::MatrixWorkspace_sptr workspace, WorkspaceIndex spectrum,
                           Qt::GlobalColor colour) override;

  void removeFromTopPreview(const QString &name) override;
  void removeFromBottomPreview(const QString &name) override;

  void enablePlotGuess(bool enable) override;
  void enableSpectrumSelection(bool enable) override;
  void enableFitRangeSelection(bool enable) override;

  void setFitSingleSpectrumText(QString const &text) override;
  void setFitSingleSpectrumEnabled(bool enable) override;

  void setBackgroundLevel(double value) override;

  void setFitRange(double minimum, double maximum) override;
  void setFitRangeMinimum(double minimum) override;
  void setFitRangeMaximum(double maximum) override;
  void setFitRangeBounds(std::pair<double, double> const &bounds) override;

  void setBackgroundRangeVisible(bool visible) override;
  void setHWHMRangeVisible(bool visible) override;

  void displayMessage(const std::string &message) const override;

  void allowRedraws(bool state) override;
  void redrawPlots() override;

  void setHWHMMinimum(double minimum) override;
  void setHWHMMaximum(double maximum) override;
  void setHWHMRange(double minimum, double maximum) override;

  void clearPreviews() override;

private slots:
  void setBackgroundBounds();

  void notifyDelayedPlotSpectrumChanged();
  void notifyPlotSpectrumChanged();
  void notifyPlotSpectrumChanged(const QString &spectrum);
  void notifySelectedFitDataChanged(int /*index*/);
  void notifyPlotGuessChanged(int /*doPlotGuess*/);
  void notifyPlotCurrentPreview();
  void notifyFitSelectedSpectrum();

  void notifyStartXChanged(double value);
  void notifyEndXChanged(double value);

  void notifyHWHMMinimumChanged(double value);
  void notifyHWHMMaximumChanged(double value);

  void notifyFWHMChanged(double minimum, double maximum);
  void notifyBackgroundChanged(double value);

private:
  void createSplitterWithPlots();
  void createSplitter();
  MantidWidgets::PreviewPlot *createTopPlot();
  MantidWidgets::PreviewPlot *createBottomPlot();
  MantidWidgets::PreviewPlot *createPlot(MantidQt::MantidWidgets::PreviewPlot *plot, QSize const &minimumSize,
                                         unsigned char horizontalStretch, unsigned char verticalStretch) const;
  void setPlotSizePolicy(MantidQt::MantidWidgets::PreviewPlot *plot, unsigned char horizontalStretch,
                         unsigned char verticalStretch) const;

  std::string getSpectrumText() const;

  void addFitRangeSelector();
  void addBackgroundRangeSelector();
  void addHWHMRangeSelector();

  void clearTopPreview();
  void clearBottomPreview();

  std::unique_ptr<Ui::FitPreviewPlot> m_plotForm;
  std::unique_ptr<MantidQt::MantidWidgets::PreviewPlot> m_topPlot;
  std::unique_ptr<MantidQt::MantidWidgets::PreviewPlot> m_bottomPlot;
  std::unique_ptr<QSplitter> m_splitter;
  IFitPlotPresenter *m_presenter;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
