// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTMANTIDWIDGETS_PYTHONCALC_H_
#define MANTIDQTMANTIDWIDGETS_PYTHONCALC_H_

#include "DllOption.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidQtWidgets/Common/MantidWidget.h"
#include <Poco/Path.h>
#include <QLabel>
#include <QLineEdit>
#include <QString>
#include <climits>
#include <map>
#include <string>

namespace MantidQt {
namespace MantidWidgets {
class EXPORT_OPT_MANTIDQT_COMMON pythonCalc : public API::MantidWidget {
  Q_OBJECT

public:
  const QString &python() const;
  /** Allows access to m_fails, the list of any validation errors
   *  return the map is empty if there were no errors, otherwise, the keys are
   * the internal names of the controls, the values the errors
   */
  QString
  checkNoErrors(const QHash<const QWidget *const, QLabel *> &validLbls) const;

  QString run();

protected:
  pythonCalc(QWidget *interface);
  /// this will store the executable python code when it is generated
  QString m_pyScript;
  /// stores the namees of controls with invalid entries as the keys and a
  /// discription of the error as the associated value
  std::map<const QWidget *const, std::string> m_fails;

  virtual void appendFile(const QString &pythonFile);
  virtual void loadFile(const QString &pythonFile);

  void LEChkCp(QString pythonMark, const QLineEdit *const userVal,
               Mantid::Kernel::Property *const check);
  std::string replaceErrsFind(QString pythonMark, const QString &setting,
                              Mantid::Kernel::Property *const check);

  void appendChk(const QLineEdit *const userVal,
                 Mantid::Kernel::Property *const check);
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQTMANTIDWIDGETS_PYTHONCALC_H_
