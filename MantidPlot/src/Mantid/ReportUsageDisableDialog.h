// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QDialog>

class FirstTimeSetup;
class QHBoxLayout;

class ReportUsageDisableDialog : public QDialog {
  Q_OBJECT

public:
  explicit ReportUsageDisableDialog(FirstTimeSetup *parent = nullptr);

private:
  /// Adds the left side of the dialog layout into the parameter layout
  void addLeftSide(QHBoxLayout *parentLayout);

  /// Adds the right side of the dialog layout into the parameter layout
  void addRightSide(QHBoxLayout *parentLayout, FirstTimeSetup *parent);
};
