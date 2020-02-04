// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INSTRUMENTVIEW_PLOTFITPANEVIEW_H_
#define MANTIDQT_INSTRUMENTVIEW_PLOTFITPANEVIEW_H_

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
  virtual void observeFitButton(Observer *listener) = 0;
  virtual std::pair<double, double> getRange() = 0;
  virtual Mantid::API::IFunction_sptr getFunction() = 0;
  virtual void addSpectrum(std::string wsName) = 0;
  virtual void addFitSpectrum(std::string wsName) = 0;
  virtual void addFunction(Mantid::API::IFunction_sptr func) = 0;
  virtual void updateFunction(Mantid::API::IFunction_sptr func) = 0;
  virtual void fitWarning(const std::string &message) = 0;

};
	
class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW PlotFitAnalysisPaneView
    : public QWidget, public IPlotFitAnalysisPaneView {
  Q_OBJECT

public:
  explicit PlotFitAnalysisPaneView(const double &start, const double &end,
                                   QWidget *parent = nullptr);

  void observeFitButton(Observer *listener) {
    m_fitObservable->attach(listener);
  };
  std::pair<double, double> getRange() override;
  Mantid::API::IFunction_sptr getFunction() override;
  void addSpectrum(std::string wsName) override;
  void addFitSpectrum(std::string wsName) override;
  void addFunction(Mantid::API::IFunction_sptr func) override;
  void updateFunction(Mantid::API::IFunction_sptr func) override;
  void fitWarning(const std::string &message) override;

public slots:
  void doFit();

protected:
  void setupPlotFitSplitter(const double &start, const double &end);
  QWidget *createFitPane(const double &start, const double &end);

private:
  MantidWidgets::PreviewPlot *m_plot;
  MantidWidgets::FunctionBrowser *m_fitBrowser;
  QLineEdit *m_start, *m_end;
  QSplitter *m_fitPlotLayout;
  QPushButton *m_fitButton;
  Observable *m_fitObservable;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTIDQT_INSTRUMENTVIEW_PLOTFITPANEVIEW_H_ */
