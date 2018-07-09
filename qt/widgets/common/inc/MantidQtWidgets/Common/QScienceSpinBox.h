#ifndef MANTIDQT_API_QSCIENCESPINBOX_H_
#define MANTIDQT_API_QSCIENCESPINBOX_H_

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
  QVariant validateAndInterpret(QString &input, int &pos,
                                QValidator::State &state) const;
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
#endif /* MANTIDQT_API_QSCIENCESPINBOX_H_ */
