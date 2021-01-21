// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FitStatusWidget.h"

#include <QColor>
#include <QHBoxLayout>
#include <QLabel>
#include <QString>
#include <QVBoxLayout>

namespace FitStatusStrings {
const std::string FAILED{"Failed"};
const std::string SUCCESS{"success"};
const std::string CHANGESTOOSMALL{"Changes"};
} // namespace FitStatusStrings
namespace {
QPalette getFitStatusColor(const std::string &status) {
  QPalette statusPalette;
  if (status.find(FitStatusStrings::SUCCESS) != std::string::npos) {
    statusPalette.setColor(QPalette::WindowText, Qt::green);
  } else if (status.find(FitStatusStrings::FAILED) != std::string::npos) {
    statusPalette.setColor(QPalette::WindowText, Qt::red);
  } else if (status.find(FitStatusStrings::CHANGESTOOSMALL) != std::string::npos) {
    statusPalette.setColor(QPalette::WindowText, QColor(255, 165, 0));
  } else {
    statusPalette.setColor(QPalette::WindowText, Qt::black);
  }
  return statusPalette;
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

FitStatusWidget::FitStatusWidget(QWidget *parent) : QWidget(parent) {
  auto fitInformationLayout = new QVBoxLayout(this);

  auto fitStatusLayout = new QHBoxLayout(this);
  auto fitStatusLabel = new QLabel(this);
  fitStatusLabel->setText(QString::fromStdString("Status:"));
  m_fitStatus = new QLabel(this);
  fitStatusLayout->addWidget(fitStatusLabel);
  fitStatusLayout->addWidget(m_fitStatus);

  auto fitChiSquaredLayout = new QHBoxLayout(this);
  auto fitChiSquaredLabel = new QLabel(this);
  fitChiSquaredLabel->setText(QString::fromStdString("Chi squared:"));
  m_fitChiSquared = new QLabel(this);
  fitChiSquaredLayout->addWidget(fitChiSquaredLabel);
  fitChiSquaredLayout->addWidget(m_fitChiSquared);

  fitInformationLayout->addLayout(fitStatusLayout);
  fitInformationLayout->addLayout(fitChiSquaredLayout);

  setLayout(fitInformationLayout);
}

void FitStatusWidget::update(const std::string &status, const double chiSquared) {
  setFitStatus(status);
  setFitChiSquared(chiSquared);
  show();
}
void FitStatusWidget::setFitStatus(const std::string &status) {
  auto color = getFitStatusColor(status);
  m_fitStatus->setPalette(color);
  m_fitStatus->setText(QString::fromStdString(status));
}
void FitStatusWidget::setFitChiSquared(const double chiSquared) {
  m_fitChiSquared->setText(QString::fromStdString(std::to_string(chiSquared)));
}
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
