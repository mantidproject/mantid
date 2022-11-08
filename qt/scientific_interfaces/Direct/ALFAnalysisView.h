// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"
#include "MantidQtWidgets/Plotting/PreviewPlot.h"

#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QPushButton>
#include <QSplitter>
#include <QString>
#include <QWidget>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_DIRECT_DLL IALFAnalysisView : public QWidget {
  Q_OBJECT

public:
  IALFAnalysisView(QWidget *parent = nullptr) : QWidget(parent) {}
  virtual ~IALFAnalysisView() = default;
  virtual void observePeakCentreLineEdit(Observer *listener) = 0;
  virtual void observeFitButton(Observer *listener) = 0;
  virtual void observeUpdateEstimateButton(Observer *listener) = 0;
  virtual std::pair<double, double> getRange() = 0;
  virtual void addSpectrum(const std::string &wsName) = 0;
  virtual void addFitSpectrum(const std::string &wsName) = 0;
  virtual void displayWarning(const std::string &message) = 0;
  virtual void setupPlotFitSplitter(const double &start, const double &end) = 0;
  virtual QWidget *createFitPane(const double &start, const double &end) = 0;
  virtual void setPeakCentre(const double centre) = 0;
  virtual double peakCentre() const = 0;
  virtual void setPeakCentreStatus(const std::string &status) = 0;
};

class MANTIDQT_DIRECT_DLL ALFAnalysisView : public IALFAnalysisView {
  Q_OBJECT

public:
  explicit ALFAnalysisView(const double &start, const double &end, QWidget *parent = nullptr);

  void observePeakCentreLineEdit(Observer *listener) override { m_peakCentreObservable->attach(listener); };
  void observeFitButton(Observer *listener) override { m_fitObservable->attach(listener); };
  void observeUpdateEstimateButton(Observer *listener) override { m_updateEstimateObservable->attach(listener); };

  std::pair<double, double> getRange() override;
  void addSpectrum(const std::string &wsName) override;
  void addFitSpectrum(const std::string &wsName) override;
  void displayWarning(const std::string &message) override;

  void setPeakCentre(const double centre) override;
  double peakCentre() const override;

  void setPeakCentreStatus(const std::string &status) override;
public slots:
  void notifyPeakCentreEditingFinished();
  void notifyUpdateEstimateClicked();
  void notifyFitClicked();

protected:
  void setupPlotFitSplitter(const double &start, const double &end) override;
  QWidget *createFitPane(const double &start, const double &end) override;

private:
  QWidget *setupFitRangeWidget(const double start, const double end);
  QWidget *setupFitButtonsWidget();
  QWidget *setupPeakCentreWidget(const double centre);

  MantidWidgets::PreviewPlot *m_plot;
  QLineEdit *m_start, *m_end;
  QSplitter *m_fitPlotLayout;
  QPushButton *m_fitButton;
  QPushButton *m_updateEstimateButton;
  QLineEdit *m_peakCentre;
  QLabel *m_fitStatus;
  Observable *m_peakCentreObservable;
  Observable *m_fitObservable;
  Observable *m_updateEstimateObservable;
};
} // namespace CustomInterfaces
} // namespace MantidQt
