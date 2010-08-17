#ifndef MANTIDQTMANTIDWIDGETS_USERFUNCTIONDIALOG_H_
#define MANTIDQTMANTIDWIDGETS_USERFUNCTIONDIALOG_H_

#include "MantidQtMantidWidgets/ui_UserFunctionDialog.h"
#include "WidgetDllOption.h"

#include <QMap>
#include <QSet>

class QComboBox;
class QLineEdit;

namespace MantidQt
{
  namespace MantidWidgets
  {
    /**
     * A dialog for construction a user fitting function from existing components
     */
	  class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS UserFunctionDialog : public QDialog
    {
      Q_OBJECT

    public:
      UserFunctionDialog(QWidget *parent=NULL,const QString& formula = "");
      QStringList categories()const;
      QString getFormula()const{return m_uiForm.teUserFunction->toPlainText();}
    public slots:
      void selectCategory(const QString& cat);
      void selectFunction(const QString& fun);
      void addExpression();
      void saveFunction();
      void updateCategories();
    protected:
      /// User interface elements
      Ui::UserFunctionDialog m_uiForm;

      void loadFunctions();
      void checkParameters(QString& expr);
      void updateFunction();
      QSet<QString> names(const QString& key = "")const;
    private:

      /// Container for prerecorded functions: key = category.name, value = formula
      /// Records with key = category.name.comment contain comments to corresponding functions
      QMap<QString,QString> m_funs;

	  };

    /**
     * A dialog to enter a category and function name for a new function for saving
     */
    class InputFunctionNameDialog: public QDialog
    {
      Q_OBJECT
    public:
      InputFunctionNameDialog(QWidget *parent,const QString& category);
      void getFunctionName(QString& category,QString& name);
    private:
      QComboBox* m_category;
      QLineEdit* m_name;
    };

  }
}

#endif //MANTIDQTMANTIDWIDGETS_USERFUNCTIONDIALOG_H_
