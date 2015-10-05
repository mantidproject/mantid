#ifndef __QScienceSpinBox_H__
#define __QScienceSpinBox_H__

#include <QtGui/QDoubleSpinBox>
#include <QtGui/QDoubleValidator>
#include <QtGui/QLineEdit>
#include <QtCore/QVariant>
#include <QtCore/QDebug>
#include <QtCore/QString>


class QScienceSpinBox : public QDoubleSpinBox
{
  Q_OBJECT
public:
  QScienceSpinBox(QWidget * parent = 0);

  int decimals() const;
  void setDecimals(int value);

  QString textFromValue ( double value ) const;
  double valueFromText ( const QString & text ) const;

  void setLogSteps(bool logSteps);

private:
  int dispDecimals;
  QChar delimiter, thousand;
  QDoubleValidator * v;
  /// Will step in a log way (multiplicatively)
  bool m_logSteps;


private:
  void initLocalValues(QWidget *parent);
  bool isIntermediateValue(const QString &str) const;
  QVariant validateAndInterpret(QString &input, int &pos, QValidator::State &state) const;
  QValidator::State validate(QString &text, int &pos) const;
  void fixup(QString &input) const;
  QString stripped(const QString &t, int *pos) const;
  double round(double value) const;
  void stepBy(int steps);

public slots:
void stepDown();
void stepUp();

signals:
  void valueChangedFromArrows();

};

#endif
