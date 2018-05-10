#ifndef MANTIDQTMANTIDWIDGETS_USERFUNCTIONDIALOG_H_
#define MANTIDQTMANTIDWIDGETS_USERFUNCTIONDIALOG_H_

#include "DllOption.h"
#include "ui_UserFunctionDialog.h"

#include <QMap>
#include <QSet>

class QComboBox;
class QLineEdit;
class QTextEdit;

namespace MantidQt {
namespace MantidWidgets {
/**
 * A dialog for construction a user fitting function from existing components
 */
class EXPORT_OPT_MANTIDQT_COMMON UserFunctionDialog : public QDialog {
  Q_OBJECT

public:
  UserFunctionDialog(QWidget *parent = nullptr, const QString &formula = "");
  ~UserFunctionDialog() override;
  QStringList categories() const;
  QString getFormula() const { return m_uiForm.teUserFunction->toPlainText(); }

private slots:
  void selectCategory(const QString &cat);
  void selectFunction(const QString &fun);
  void addExpression();
  void saveFunction();
  void removeCurrentFunction();
  void updateCategories();
  void updateFunction();
  void helpClicked();

private:
  bool eventFilter(QObject *obj, QEvent *ev) override;

  void loadFunctions();
  void checkParameters(QString &expr);
  QSet<QString> categoryNames() const;
  QSet<QString> functionNames(const QString &cat) const;
  QString getCurrentCategory() const;
  QString getFunction(const QString &cat, const QString &fun) const;
  QString getComment(const QString &cat, const QString &fun) const;
  void setFunction(const QString &cat, const QString &fun, const QString &expr,
                   const QString &comment = "");
  bool isBuiltin(const QString &cat) const;
  void saveToFile();

  /// User interface elements
  Ui::UserFunctionDialog m_uiForm;

  /// Container for prerecorded functions: key = category.name, value = formula
  /// Records with key = category.name.comment contain comments to corresponding
  /// functions
  QMap<QString, QString> m_funs;
};

/**
 * A dialog to enter a category and function name for a new function for saving
 */
class InputFunctionNameDialog : public QDialog {
  Q_OBJECT
public:
  InputFunctionNameDialog(QWidget *parent, const QString &category);
  void getFunctionName(QString &category, QString &name, QString &comment);

private:
  QComboBox *m_category;
  QLineEdit *m_name;
  QTextEdit *m_comment;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQTMANTIDWIDGETS_USERFUNCTIONDIALOG_H_
