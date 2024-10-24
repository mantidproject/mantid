// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DllOption.h"

#include <QDebug>
#include <QDoubleSpinBox>
#include <QDoubleValidator>
#include <QLineEdit>
#include <QString>
#include <QVariant>

namespace MantidQt {
namespace API {

class EXPORT_OPT_MANTIDQT_COMMON QScienceSpinBox : public QDoubleSpinBox {
  Q_OBJECT
public:
  QScienceSpinBox(QWidget *parent = nullptr);

  int decimals() const;
  void setDecimals(int value);

  QString textFromValue(double value) const override;
  double valueFromText(const QString &text) const override;

  void setLogSteps(bool logSteps);

private:
  int dispDecimals;
  QChar delimiter, thousand;
  QDoubleValidator *v;
  /// Will step in a log way (multiplicatively)
  bool m_logSteps;

private:
  void initLocalValues(QWidget *parent);
  bool isIntermediateValue(const QString &str) const;
  QVariant validateAndInterpret(QString &input, int &pos, QValidator::State &state) const;
  QValidator::State validate(QString &text, int &pos) const override;
  void fixup(QString &input) const override;
  QString stripped(const QString &t, int *pos) const;
  double round(double value) const;
  void stepBy(int steps) override;

public slots:
  void stepDown();
  void stepUp();

signals:
  void valueChangedFromArrows();
};

} // namespace API
} // namespace MantidQt
