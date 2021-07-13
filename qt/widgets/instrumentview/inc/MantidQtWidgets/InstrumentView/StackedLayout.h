// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IStackedLayout.h"

#include <QStackedLayout>

class QWidget;

namespace MantidQt::MantidWidgets {

class StackedLayout final : public IStackedLayout, public QStackedLayout {
public:
  StackedLayout(QWidget *parent) : QStackedLayout(parent) {}

  virtual int addWidget(QWidget *parent) override { return QStackedLayout::addWidget(parent); }
  virtual int currentIndex() const override { return QStackedLayout::currentIndex(); }
  virtual QWidget *currentWidget() const override { return QStackedLayout::currentWidget(); }
  virtual void setCurrentIndex(int val) override { QStackedLayout::setCurrentIndex(val); }
};
} // namespace MantidQt::MantidWidgets
