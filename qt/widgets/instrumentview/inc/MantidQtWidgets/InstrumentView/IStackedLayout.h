// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

class QWidget;

namespace MantidQt::MantidWidgets {

class IStackedLayout {
public:
  virtual ~IStackedLayout() = default;

  virtual int addWidget(QWidget *) = 0;
  virtual int currentIndex() const = 0;
  virtual QWidget *currentWidget() const = 0;
  virtual void setCurrentIndex(int) = 0;
};
} // namespace MantidQt::MantidWidgets
