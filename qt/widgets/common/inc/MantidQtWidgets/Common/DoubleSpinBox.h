// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : DoubleSpinBox.h
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#pragma once

#include "DllOption.h"
#include <map>

#include <QAbstractSpinBox>
#include <QCheckBox>

//! A QDoubleSpinBox allowing to customize numbers display with respect to
// locale settings.
/**
 * It allows the user to specify a custom display format.
 */
class EXPORT_OPT_MANTIDQT_COMMON DoubleSpinBox : public QAbstractSpinBox {
  Q_OBJECT

public:
  //! Constructor.
  /**
   * \param format format used to display numbers: has the same meaning as in
   * QLocale::toString ( double i, char f = 'g', int prec = 6 )
   * \param parent parent widget (only affects placement of the dialog)
   */
  DoubleSpinBox(const char format = 'g', QWidget *parent = nullptr);

  void setSingleStep(double val);
  void setMaximum(double max);
  void setMinimum(double min);
  void setRange(double min, double max);

  double getMaximum();
  double getMinimum();

  int decimals() { return d_prec; };
  void setDecimals(int prec) {
    if (prec >= 0)
      d_prec = prec;
  };

  double value();
  bool setValue(double val);

  void setFormat(const char format, int prec = 1) {
    d_format = format;
    setDecimals(prec);
  };

  void addSpecialTextMapping(const QString &text, double value);

  QString textFromValue(double value) const;
  QValidator::State validate(QString &input, int &pos) const override;

signals:
  void valueChanged(double d);
  //! Signal emitted when the spin box gains focus
  void activated(DoubleSpinBox * /*_t1*/);

private slots:
  void interpretText(bool notify = true);

protected:
  void stepBy(int steps) override;
  StepEnabled stepEnabled() const override;
  void focusInEvent(QFocusEvent * /*event*/) override;

private:
  char d_format;
  double d_min_val;
  double d_max_val;
  double d_value;
  double d_step;
  int d_prec;

  // A set of mappins from strings which the user can enter in the box to double
  // values
  std::map<QString, double> m_specialTextMappings;
};

//! A checkable DoubleSpinBox that can be used to select the limits of a double
// interval.
class EXPORT_OPT_MANTIDQT_COMMON RangeLimitBox : public QWidget {
public:
  enum LimitType { LeftLimit, RightLimit };

  RangeLimitBox(LimitType type, QWidget *parent = nullptr);
  void setDecimals(int prec) { d_spin_box->setDecimals(prec); };
  double value();
  bool isChecked() { return d_checkbox->isChecked(); };

private:
  DoubleSpinBox *d_spin_box;
  QCheckBox *d_checkbox;
  LimitType d_type;
};
