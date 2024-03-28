// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QWidget>

class QLabel;

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {
class FitStatusWidget : public QWidget {
  Q_OBJECT
public:
  FitStatusWidget(QWidget *parent = nullptr);
  void update(const std::string &status, const double chiSquared);

private:
  void setFitChiSquared(const double chiSquared);
  void setFitStatus(const std::string &status);
  QLabel *m_fitStatus;
  QLabel *m_fitChiSquared;
};
} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
