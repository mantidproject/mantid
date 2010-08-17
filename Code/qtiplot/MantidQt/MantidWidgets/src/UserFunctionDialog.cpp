#include "MantidQtMantidWidgets/UserFunctionDialog.h"
#include "MantidQtMantidWidgets/RenameParDialog.h"
#include "MantidAPI/Expression.h"

#include <QComboBox>
#include <QStringListModel>
#include <QDialogButtonBox>
#include <QMessageBox>

#include <algorithm>

using namespace MantidQt::MantidWidgets;

UserFunctionDialog::UserFunctionDialog(QWidget *parent,const QString& formula)
:QDialog(parent)
{
  m_uiForm.setupUi(this);
  connect(m_uiForm.lstCategory,SIGNAL(currentTextChanged(const QString&)),this,SLOT(selectCategory(const QString&)));
  connect(m_uiForm.lstFunction,SIGNAL(currentTextChanged(const QString&)),this,SLOT(selectFunction(const QString&)));
  connect(m_uiForm.btnSave,SIGNAL(clicked()),this,SLOT(saveFunction()));
  connect(m_uiForm.btnAdd,SIGNAL(clicked()),this,SLOT(addExpression()));
  connect(m_uiForm.btnUse,SIGNAL(clicked()),this,SLOT(accept()));
  connect(m_uiForm.btnCancel,SIGNAL(clicked()),this,SLOT(reject()));
  loadFunctions();
  updateCategories();
  if ( !formula.isEmpty() )
  {
    QRect rect = m_uiForm.teUserFunction->cursorRect();
    QTextCursor cursor = m_uiForm.teUserFunction->cursorForPosition(rect.topLeft());
    cursor.insertText(formula);
    updateFunction();
  }
}

/**
 * Load saved functions form Mantid(.user).properties file.
 * Property: userfunctions.CategoryName.FunctionName = Expression-in-Mu::Parser-format
 */
void UserFunctionDialog::loadFunctions()
{
  // define the build-in functions
  m_funs.insert("Base.abs","abs(x)");
  m_funs.insert("Base.sin","sin(x)");
  m_funs.insert("Base.cos","cos(x)");
  //m_funs.insert("Base.","");
  m_funs.insert("Built-in.Gauss","h*exp(-s*(x-c)^2)");
  m_funs.insert("Built-in.ExpDecay","h*exp(-x/t)");
}

/**
 * Update the GUI element displaying categories.
 */
void UserFunctionDialog::updateCategories()
{
  QSet<QString> cats = names();
  foreach(QString cat ,cats)
  {
    m_uiForm.lstCategory->addItem(cat);
  }
}

/**
 * Make a category current
 * @param cat The category to select
 */
void UserFunctionDialog::selectCategory(const QString& cat)
{
  //std::string category_key = "userfunctions."+cat.toStdString();
  m_uiForm.lstFunction->clear();

  QSet<QString> funs = names(cat);
  foreach(QString fun ,funs)
  {
    QString key = cat + "." + fun;
    QString value = m_funs[key];
    if ( !value.isEmpty() )
    {
      m_uiForm.lstFunction->addItem(fun);
    }
  }
  m_uiForm.lstFunction->setCurrentRow(0);

}

/**
 * Make a function current
 * @param fun The function to select
 */
void UserFunctionDialog::selectFunction(const QString& fun)
{
  QString cat = m_uiForm.lstCategory->currentItem()->text();
  m_uiForm.teExpression->clear();

  QString fun_key = cat + "." + fun;
  QString value = m_funs[fun_key];

  QString comment = m_funs[fun_key + ".comment"];
  if ( !comment.isEmpty() )
  {
    value += "\n\n" + comment;
  }

  m_uiForm.teExpression->setText(value);
}

/**
 * Add selected expression to the user function
 */
void UserFunctionDialog::addExpression()
{
  QString expr = m_uiForm.teExpression->toPlainText();
  int iBr = expr.indexOf('\n');
  if (iBr > 0)
  {
    expr.remove(iBr,expr.size());
  }

  checkParameters(expr);

  QRect rect = m_uiForm.teUserFunction->cursorRect();
  QTextCursor cursor = m_uiForm.teUserFunction->cursorForPosition(rect.topLeft());
  if (cursor.position() > 0)
  {
    expr.prepend('+');
  }
  cursor.insertText(expr);

  updateFunction();
}

/**
 * Check an expression for name clashes with user function
 * @param expr An expression prepared to be added to the user function.
 */
void UserFunctionDialog::checkParameters(QString& expr)
{
  if (expr.isEmpty()) return;
  QString fun = m_uiForm.teUserFunction->toPlainText();
  if (fun.isEmpty()) return;

  // collect parameter names in sets vars1 and vars2
  Mantid::API::Expression e1;
  e1.parse(fun.toStdString());
  Mantid::API::Expression e2;
  e2.parse(expr.toStdString());
  std::set<std::string> vars1 = e1.getVariables();
  std::set<std::string> vars2 = e2.getVariables();
  vars1.erase("x");
  vars2.erase("x");

  // combine all names frm the two sets
  std::vector<std::string> all(vars1.size()+vars2.size(),"");
  std::set_union(vars1.begin(),vars1.end(),vars2.begin(),vars2.end(),all.begin());
  std::vector<std::string>::iterator it = std::find(all.begin(),all.end(),"");
  if (it != all.end())
  {
    all.erase(it,all.end());
  }

  // compare variable names and collect common names
  std::vector<std::string> common(std::min(vars1.size(),vars2.size()),"");
  std::set_intersection(vars1.begin(),vars1.end(),vars2.begin(),vars2.end(),common.begin());

  // ask the user to rename the common names
  if ( !common.empty() )
  {
    RenameParDialog dlg(all,common);
    if (dlg.exec() == QDialog::Accepted)
    {
      std::set<std::string> vars2_new;
      dlg.setOutput(vars2_new);
      std::set<std::string>::const_iterator v2_old = vars2.begin();
      std::set<std::string>::const_iterator v2_new = vars2_new.begin();
      for(; v2_old != vars2.end(); ++v2_old,++v2_new)
      {
        e2.renameAll(*v2_old,*v2_new);
      }
      expr = QString::fromStdString(e2.str());
    }
  }

}

/**
 * Updates the parameter list
 */
void UserFunctionDialog::updateFunction()
{
  QString fun = m_uiForm.teUserFunction->toPlainText();
  Mantid::API::Expression e;
  e.parse(fun.toStdString());
  std::set<std::string> vars = e.getVariables();
  vars.erase("x");
  QString params;
  for(std::set<std::string>::iterator it=vars.begin();it!=vars.end();++it)
  {
    if (it != vars.begin())
    {
      params += ",";
    }
    params += QString::fromStdString(*it);
  }
  m_uiForm.leParams->setText(params);

}

/**
 * Returns function names: If the input cat parameter is empty the returned set
 * contains funtion categories, otherwise it returns function names in category cat.
 * @param cat The category for which functions will be returned.
 * @return A set of funtion names.
 */
QSet<QString> UserFunctionDialog::names(const QString& cat)const
{
  QSet<QString> out;
  if (cat.isEmpty())
  {
    QMap<QString,QString>::const_iterator it = m_funs.begin();
    for(; it != m_funs.end(); ++it)
    {
      QStringList cn = it.key().split('.');
      out.insert(cn[0]);
    }
  }
  else
  {
    QMap<QString,QString>::const_iterator it = m_funs.begin();
    for(; it != m_funs.end(); ++it)
    {
      QStringList cn = it.key().split('.');
      if (cn[0] == cat)
      {
        out.insert(cn[1]);
      }
    }
  }
  return out;
}

/**
 * Save the constructed function for future use
 */
void UserFunctionDialog::saveFunction()
{
  QString cur_category = m_uiForm.lstCategory->currentItem()->text();
  if (cur_category == "Base" || cur_category == "Built-in")
  {
    cur_category = "";
  }
  
  InputFunctionNameDialog* dlg = new InputFunctionNameDialog(this,cur_category);
  if (dlg->exec() == QDialog::Accepted)
  {
    QString category;
    QString name;
    dlg->getFunctionName(category,name);
    if (name.isEmpty())
    {
      QMessageBox::critical(this,"Mantid - Error","The function name is empty");
      return;
    }
    // check if the category already exists
    QList<QListWidgetItem*> items = m_uiForm.lstCategory->findItems(category,Qt::MatchExactly);
    if ( !items.isEmpty() )
    {// check if a function with this name already exists
      const QSet<QString> functions = names(category);
      QSet<QString>::const_iterator found = functions.find(name);
      if (found != functions.end() &&
        QMessageBox::question(this,"Mantid","A function with name "+name+" already exists in category "+category+".\n"
          "Would you like to replace it?",QMessageBox::Yes|QMessageBox::No) == QMessageBox::No)
      {
        return;
      }
    }
    QString fun = m_uiForm.teUserFunction->toPlainText();
    QString fun_key = category+"."+name;
    m_funs[fun_key] = fun;
    updateCategories();
  }//QDialog::Accepted
}

QStringList UserFunctionDialog::categories()const
{
  QStringList out;
  for(int i = 0; i < m_uiForm.lstCategory->count(); ++i)
  {
    out << m_uiForm.lstCategory->item(i)->text();
  }
  return out;
}

/**
 * Constructor
 * @param category The initial suggestion for the category 
 */
InputFunctionNameDialog::InputFunctionNameDialog(QWidget *parent,const QString& category)
:QDialog(parent)
{
  QVBoxLayout* layout = new QVBoxLayout();
  layout->addWidget(new QLabel("Enter new or select a category"));
  m_category = new QComboBox();
  m_category->addItems(((UserFunctionDialog*)parent)->categories());
  m_category->setEditable(true);
  int index = m_category->findText(category);
  if (index >= 0)
  {
    m_category->setCurrentIndex(index);
  }
  layout->addWidget(m_category);
  layout->addWidget(new QLabel("Enter a name for the new function"));
  m_name = new QLineEdit();
  layout->addWidget(m_name);
  QDialogButtonBox* buttons = new QDialogButtonBox();
  buttons->addButton("OK",QDialogButtonBox::AcceptRole);
  buttons->addButton("Cancel",QDialogButtonBox::RejectRole);
  buttons->setCenterButtons(true);
  connect(buttons,SIGNAL(accepted()),this,SLOT(accept()));
  connect(buttons,SIGNAL(rejected()),this,SLOT(reject()));
  layout->addWidget(buttons);
  setLayout(layout);
}

/**
 * Return the entered category and function name
 * @param category A string to recieve the category
 * @param name A string to recieve the function name
 */
void InputFunctionNameDialog::getFunctionName(QString& category,QString& name)
{
  category = m_category->currentText();
  name = m_name->text();
}
