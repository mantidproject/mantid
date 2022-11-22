// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
#include <QString>
#include <QWidget>

#include <string>

namespace MantidQt {

namespace MantidWidgets {
class PreviewPlot;
}

namespace CustomInterfaces {

class IALFAnalysisPresenter;

class MANTIDQT_DIRECT_DLL IALFAnalysisView {
public:
  virtual ~IALFAnalysisView() = default;

  virtual QWidget *getView() = 0;

  virtual void subscribePresenter(IALFAnalysisPresenter *presenter) = 0;

  virtual std::pair<double, double> getRange() const = 0;

  virtual void addSpectrum(std::string const &wsName) = 0;
  virtual void addFitSpectrum(std::string const &wsName) = 0;

  virtual void setPeakCentre(double const centre) = 0;
  virtual double peakCentre() const = 0;
  virtual void setPeakCentreStatus(std::string const &status) = 0;

  virtual void displayWarning(std::string const &message) = 0;
};

class MANTIDQT_DIRECT_DLL ALFAnalysisView final : public QWidget, public IALFAnalysisView {
  Q_OBJECT

public:
  explicit ALFAnalysisView(double const start, double const end, QWidget *parent = nullptr);

  QWidget *getView() override;

  void subscribePresenter(IALFAnalysisPresenter *presenter) override;

  std::pair<double, double> getRange() const override;

  void addSpectrum(std::string const &wsName) override;
  void addFitSpectrum(std::string const &wsName) override;

  void setPeakCentre(double const centre) override;
  double peakCentre() const override;

  void setPeakCentreStatus(std::string const &status) override;

  void displayWarning(std::string const &message) override;

public slots:
  void notifyPeakCentreEditingFinished();
  void notifyUpdateEstimateClicked();
  void notifyFitClicked();

private:
  void setupPlotFitSplitter(double const start, double const end);
  QWidget *createFitPane(double const start, double const end);
  QWidget *setupFitRangeWidget(double const start, double const end);
  QWidget *setupFitButtonsWidget();
  QWidget *setupPeakCentreWidget(double const centre);

  MantidWidgets::PreviewPlot *m_plot;
  QLineEdit *m_start, *m_end;
  QSplitter *m_fitPlotLayout;
  QPushButton *m_fitButton;
  QPushButton *m_updateEstimateButton;
  QLineEdit *m_peakCentre;
  QLabel *m_fitStatus;

  IALFAnalysisPresenter *m_presenter;
};
} // namespace CustomInterfaces
} // namespace MantidQt
