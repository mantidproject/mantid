// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "FitPropertyBrowser.h"

#include <QMainWindow>
#include <QObject>
#include <QWidget>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class FitDataView;
class IndirectFitPlotView;

class MANTIDQT_INELASTIC_DLL DockWidgetArea : public QMainWindow {
  Q_OBJECT

public:
  DockWidgetArea(QWidget *parent = nullptr);
  virtual ~DockWidgetArea(){};
  void setFitDataView(FitDataView *fitDataView);
  IndirectFitPropertyBrowser *m_fitPropertyBrowser;
  FitDataView *m_fitDataView;
  IndirectFitPlotView *m_fitPlotView;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt