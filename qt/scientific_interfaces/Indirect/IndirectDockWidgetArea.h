// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "IIndirectFitDataView.h"
#include "IndirectFitPlotView.h"
#include "IndirectFitPropertyBrowser.h"

#include <QMainWindow>
#include <QObject>
#include <QWidget>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class IndirectDockWidgetArea : public QMainWindow {
  Q_OBJECT

public:
  IndirectDockWidgetArea(QWidget *parent = nullptr);
  virtual ~IndirectDockWidgetArea(){};
  void setFitDataView(IIndirectFitDataView *fitDataView);
  IndirectFitPropertyBrowser *m_fitPropertyBrowser;
  IIndirectFitDataView *m_fitDataView;
  IndirectFitPlotView *m_fitPlotView;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt