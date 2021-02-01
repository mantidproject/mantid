// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/FunctionBrowser.h"
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
  virtual void observeFitButton(Observer *listener) = 0;
  virtual void observeUpdateEstimateButton(Observer *listener) = 0;
  virtual std::pair<double, double> getRange() = 0;
  virtual Mantid::API::IFunction_sptr getFunction() = 0;
  virtual void addSpectrum(const std::string &wsName) = 0;
  virtual void addFitSpectrum(const std::string &wsName) = 0;
  virtual void addFunction(Mantid::API::IFunction_sptr func) = 0;
  virtual void updateFunction(const Mantid::API::IFunction_sptr func) = 0;
  virtual void displayWarning(const std::string &message) = 0;
  virtual QWidget *getQWidget() = 0;
  virtual void setupPlotFitSplitter(const double &start, const double &end) = 0;
  virtual QWidget *createFitPane(const double &start, const double &end) = 0;
};

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW PlotFitAnalysisPaneView : public QWidget, public IPlotFitAnalysisPaneView {
  Q_OBJECT

public:
  explicit PlotFitAnalysisPaneView(const double &start, const double &end, QWidget *parent = nullptr);

  void observeFitButton(Observer *listener) override { m_fitObservable->attach(listener); };

  void observeUpdateEstimateButton(Observer *listener) override { m_updateEstimateObservable->attach(listener); };

  std::pair<double, double> getRange() override;
  Mantid::API::IFunction_sptr getFunction() override;
  void addSpectrum(const std::string &wsName) override;
  void addFitSpectrum(const std::string &wsName) override;
  void addFunction(Mantid::API::IFunction_sptr func) override;
  void updateFunction(const Mantid::API::IFunction_sptr func) override;
  void displayWarning(const std::string &message) override;
  QWidget *getQWidget() override { return static_cast<QWidget *>(this); };

public slots:
  void doFit();
  void updateEstimate();

protected:
  void setupPlotFitSplitter(const double &start, const double &end) override;
  QWidget *createFitPane(const double &start, const double &end) override;

private:
  MantidWidgets::PreviewPlot *m_plot;
  MantidWidgets::FunctionBrowser *m_fitBrowser;
  QLineEdit *m_start, *m_end;
  QSplitter *m_fitPlotLayout;
  QPushButton *m_fitButton;
  QPushButton *m_updateEstimateButton;
  Observable *m_fitObservable;
  Observable *m_updateEstimateObservable;
};
} // namespace MantidWidgets
} // namespace MantidQt
