// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "ui_IndirectDockWidgetArea.h"
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
  std::unique_ptr<Ui::IndirectDockWidgetArea> m_uiForm;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt