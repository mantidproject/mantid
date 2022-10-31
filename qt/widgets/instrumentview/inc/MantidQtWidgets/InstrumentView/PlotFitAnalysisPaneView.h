// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"
#include "MantidQtWidgets/Plotting/PreviewPlot.h"

#include <QLineEdit>
#include <QObject>
#include <QPushButton>
#include <QSplitter>
#include <QString>
#include <QWidget>
#include <string>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW IPlotFitAnalysisPaneView {
public:
  IPlotFitAnalysisPaneView(){};
  virtual ~IPlotFitAnalysisPaneView(){};
  virtual void observePeakCentreLineEdit(Observer *listener) = 0;
  virtual void observeFitButton(Observer *listener) = 0;
  virtual void observeUpdateEstimateButton(Observer *listener) = 0;
  virtual std::pair<double, double> getRange() = 0;
  virtual void addSpectrum(const std::string &wsName) = 0;
  virtual void addFitSpectrum(const std::string &wsName) = 0;
  virtual void displayWarning(const std::string &message) = 0;
  virtual QWidget *getQWidget() = 0;
  virtual void setupPlotFitSplitter(const double &start, const double &end) = 0;
  virtual QWidget *createFitPane(const double &start, const double &end) = 0;
  virtual void setPeakCentre(const double centre) = 0;
  virtual double peakCentre() const = 0;
  virtual void setPeakCentreStatus(const std::string &status) = 0;
};

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW PlotFitAnalysisPaneView : public QWidget, public IPlotFitAnalysisPaneView {
  Q_OBJECT

public:
  explicit PlotFitAnalysisPaneView(const double &start, const double &end, QWidget *parent = nullptr);

  void observePeakCentreLineEdit(Observer *listener) override { m_peakCentreObservable->attach(listener); };
  void observeFitButton(Observer *listener) override { m_fitObservable->attach(listener); };
  void observeUpdateEstimateButton(Observer *listener) override { m_updateEstimateObservable->attach(listener); };

  std::pair<double, double> getRange() override;
  void addSpectrum(const std::string &wsName) override;
  void addFitSpectrum(const std::string &wsName) override;
  void displayWarning(const std::string &message) override;
  QWidget *getQWidget() override { return static_cast<QWidget *>(this); };

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
  QLineEdit *m_peakCentre;
  QSplitter *m_fitPlotLayout;
  QPushButton *m_fitButton;
  QPushButton *m_updateEstimateButton;
  Observable *m_peakCentreObservable;
  Observable *m_fitObservable;
  Observable *m_updateEstimateObservable;
};
} // namespace MantidWidgets
} // namespace MantidQt
