// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/UserFunctionDialog.h"
#include "MantidAPI/Expression.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/RenameParDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QKeyEvent>
#include <QMessageBox>
#include <QStringListModel>
#include <QTextStream>
#include <QUrl>

#include <algorithm>

using namespace MantidQt::MantidWidgets;
using MantidQt::API::HelpWindow;

UserFunctionDialog::UserFunctionDialog(QWidget *parent, const QString &formula) : QDialog(parent) {
  m_uiForm.setupUi(this);
  connect(m_uiForm.lstCategory, SIGNAL(currentTextChanged(const QString &)), this,
          SLOT(selectCategory(const QString &)));
  connect(m_uiForm.lstFunction, SIGNAL(currentTextChanged(const QString &)), this,
          SLOT(selectFunction(const QString &)));
  connect(m_uiForm.btnSave, SIGNAL(clicked()), this, SLOT(saveFunction()));
  connect(m_uiForm.btnRemove, SIGNAL(clicked()), this, SLOT(removeCurrentFunction()));
  connect(m_uiForm.btnAdd, SIGNAL(clicked()), this, SLOT(addExpression()));
  connect(m_uiForm.btnUse, SIGNAL(clicked()), this, SLOT(accept()));
  connect(m_uiForm.btnCancel, SIGNAL(clicked()), this, SLOT(reject()));
  connect(m_uiForm.btnHelp, SIGNAL(clicked()), this, SLOT(helpClicked()));
  connect(m_uiForm.teUserFunction, SIGNAL(textChanged()), this, SLOT(updateFunction()));
  m_uiForm.teUserFunction->installEventFilter(this);

  loadFunctions();
  updateCategories();
  if (!formula.isEmpty()) {
    QRect rect = m_uiForm.teUserFunction->cursorRect();
    QTextCursor cursor = m_uiForm.teUserFunction->cursorForPosition(rect.topLeft());
    cursor.insertText(formula);
    // updateFunction();
  }
}

/**
 * Write saved functions in the destructor
 */
UserFunctionDialog::~UserFunctionDialog() { saveToFile(); }

/**
 * Load saved functions form Mantid(.user).properties file.
 * Property: userfunctions.CategoryName.FunctionName = Expression-in-Mu
 * Parser-format
 */
void UserFunctionDialog::loadFunctions() {
  // define the built-in functions
  setFunction("Base", "abs", "abs(x)", "Absolute value of x");
  setFunction("Base", "sin", "sin(x)", "Sine of x");
  setFunction("Base", "cos", "cos(x)", "Cosine of x");
  setFunction("Base", "tan", "tan(x)", "Tangent of x");
  setFunction("Base", "asin", "asin(x)", "Arc-sine of x");
  setFunction("Base", "acos", "acos(x)", "Arc-cosine of x");
  setFunction("Base", "atan", "atan(x)", "Arc-tangent of x");
  setFunction("Base", "sinh", "sinh(x)", "Sine hyperbolic of x");
  setFunction("Base", "cosh", "cosh(x)", "Cosine hyperbolic of x");
  setFunction("Base", "tanh", "tanh(x)", "Tangent hyperbolic of x");
  setFunction("Base", "asinh", "asinh(x)", "Arc-sine hyperbolic of x");
  setFunction("Base", "acosh", "acosh(x)", "Arc-cosine hyperbolic of x");
  setFunction("Base", "atanh", "atanh(x)", "Arc-tangent hyperbolic of x");
  setFunction("Base", "log2", "log2(x)", "Logarithm to the base 2");
  setFunction("Base", "log10", "log10(x)", "Logarithm to the base 10");
  setFunction("Base", "log", "log(x)", "Logarithm to the base 10");
  setFunction("Base", "ln", "ln(x)", "Logarithm to the base e = 2.71828...");
  setFunction("Base", "exp", "exp(x)", "e to the power of x");
  setFunction("Base", "sqrt", "sqrt(x)", "Square root of x");
  setFunction("Base", "sign", "sign(x)", "Sign of x");
  setFunction("Base", "rint", "rint(x)", "Round to nearest integer");
  setFunction("Base", "erf", "erf(x)", "error function of x");
  setFunction("Base", "erfc", "erfc(x)", "Complementary error function erfc(x) = 1 - erf(x)");
  setFunction("Built-in", "Gauss", "h*exp(-s*(x-c)^2)");
  setFunction("Built-in", "ExpDecay", "h*exp(-x/t)");
  QFile funFile(QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getUserPropertiesDir()) +
                "Mantid.user.functions");
  if (funFile.exists() && funFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QTextStream in(&funFile);
    while (!in.atEnd()) {
      QString line = in.readLine();
      QStringList key_value = line.split('=');
      if (key_value.size() != 2)
        continue;
      m_funs.insert(key_value[0].trimmed(), key_value[1].trimmed());
    }
  }
}

/**
 * Update the GUI element displaying categories.
 */
void UserFunctionDialog::updateCategories() {
  // store the name of the current item
  QString currentCategory = getCurrentCategory();
  m_uiForm.lstCategory->clear();
  QSet<QString> cats = categoryNames();
  foreach (QString cat, cats) {
    m_uiForm.lstCategory->addItem(cat);
  }
  // try to restore current item selection
  auto items = m_uiForm.lstCategory->findItems(currentCategory, Qt::MatchExactly);
  if (!items.isEmpty()) {
    m_uiForm.lstCategory->setCurrentItem(items[0]);
  }
}

/**
 * Make a category current
 * @param cat :: The category to select
 */
void UserFunctionDialog::selectCategory(const QString &cat) {
  QSet<QString> funs = functionNames(cat);
  m_uiForm.lstFunction->clear();
  foreach (QString fun, funs) {
    QString value = getFunction(cat, fun);
    if (!value.isEmpty()) {
      m_uiForm.lstFunction->addItem(fun);
    }
  }
  if (m_uiForm.lstFunction->count() > 0) {
    m_uiForm.lstFunction->sortItems();
    m_uiForm.lstFunction->setCurrentRow(0);
  } else {
    m_uiForm.teExpression->clear();
  }
  m_uiForm.btnRemove->setEnabled(!isBuiltin(cat));
}

/**
 * Make a function current
 * @param fun :: The function to select
 */
void UserFunctionDialog::selectFunction(const QString &fun) {
  if (fun.isEmpty()) {
    return;
  }
  QString cat = m_uiForm.lstCategory->currentItem()->text();
  m_uiForm.teExpression->clear();

  QString value = getFunction(cat, fun);
  QString comment = getComment(cat, fun);
  if (!comment.isEmpty()) {
    value += "\n\n" + comment;
  }

  m_uiForm.teExpression->setText(value);
}

/**
 * Add selected expression to the user function
 */
void UserFunctionDialog::addExpression() {
  QString expr = m_uiForm.teExpression->toPlainText();
  int iBr = expr.indexOf('\n');
  if (iBr > 0) {
    expr.remove(iBr, expr.size());
  }

  checkParameters(expr);

  if (expr.isEmpty())
    return;

  QRect rect = m_uiForm.teUserFunction->cursorRect();
  QTextCursor cursor = m_uiForm.teUserFunction->cursorForPosition(rect.topLeft());
  if (cursor.position() > 0) {
    expr.prepend('+');
  }
  cursor.insertText(expr);

  // updateFunction();
}

/**
 * Check an expression for name clashes with user function
 * @param expr :: An expression prepared to be added to the user function.
 */
void UserFunctionDialog::checkParameters(QString &expr) {
  if (expr.isEmpty())
    return;
  QString fun = m_uiForm.teUserFunction->toPlainText();
  if (fun.isEmpty())
    return;

  // collect parameter names in sets vars1 and vars2
  Mantid::API::Expression e1;
  Mantid::API::Expression e2;
  try {
    e1.parse(fun.toStdString());
    e2.parse(expr.toStdString());
  } catch (...) {
    return;
  }
  auto vars1 = e1.getVariables();
  auto vars2 = e2.getVariables();
  vars1.erase("x");
  vars2.erase("x");

  // combine all names frm the two sets
  std::vector<std::string> all(vars1.size() + vars2.size(), "");
  std::set_union(vars1.begin(), vars1.end(), vars2.begin(), vars2.end(), all.begin());
  std::vector<std::string>::iterator it = std::find(all.begin(), all.end(), "");
  if (it != all.end()) {
    all.erase(it, all.end());
  }

  // compare variable names and collect common names
  std::vector<std::string> common(std::min<size_t>(vars1.size(), vars2.size()), "");
  std::set_intersection(vars1.begin(), vars1.end(), vars2.begin(), vars2.end(), common.begin());
  it = std::find(common.begin(), common.end(), "");
  if (it != common.end()) {
    common.erase(it, common.end());
  }

  // ask the user to rename the common names
  if (!common.empty()) {
    RenameParDialog dlg(all, common);
    if (dlg.exec() == QDialog::Accepted) {
      auto vars_new = dlg.setOutput();
      std::vector<std::string>::const_iterator v_old = common.begin();
      std::vector<std::string>::const_iterator v_new = vars_new.begin();
      for (; v_old != common.end(); ++v_old, ++v_new) {
        e2.renameAll(*v_old, *v_new);
        expr = QString::fromStdString(e2.str());
      }
    } else {
      expr = "";
    }
  }
}

/**
 * Updates the parameter list
 */
void UserFunctionDialog::updateFunction() {
  QString fun = m_uiForm.teUserFunction->toPlainText();
  Mantid::API::Expression e;
  try {
    e.parse(fun.toStdString());
  } catch (...) { // the formula could be being edited manually
    m_uiForm.leParams->setText("");
    return;
  }
  auto vars = e.getVariables();
  vars.erase("x");
  QString params;
  for (auto it = vars.begin(); it != vars.end(); ++it) {
    if (it != vars.begin()) {
      params += ",";
    }
    params += QString::fromStdString(*it);
  }
  m_uiForm.leParams->setText(params);
}

/**
 * Returns a list of category names
 */
QSet<QString> UserFunctionDialog::categoryNames() const {
  QSet<QString> out;
  QMap<QString, QString>::const_iterator it = m_funs.begin();
  for (; it != m_funs.end(); ++it) {
    QStringList cn = it.key().split('.');
    out.insert(cn[0]);
  }
  return out;
}

/**
 * Returns function names in category cat.
 * @param cat :: The category for which functions will be returned.
 * @return A set of funtion names.
 */
QSet<QString> UserFunctionDialog::functionNames(const QString &cat) const {
  QSet<QString> out;
  QMap<QString, QString>::const_iterator it = m_funs.begin();
  for (; it != m_funs.end(); ++it) {
    QStringList cn = it.key().split('.');
    if (cn[0] == cat) {
      out.insert(cn[1]);
    }
  }
  return out;
}

/**
 * Get the name of currently selected category. If no category is selected
 * returns
 * empty string.
 */
QString UserFunctionDialog::getCurrentCategory() const {
  QString cur_category;
  QListWidgetItem const *currentCategoryItem = m_uiForm.lstCategory->currentItem();
  if (currentCategoryItem) {
    cur_category = m_uiForm.lstCategory->currentItem()->text();
  }
  return cur_category;
}

/**
 * Save the constructed function for future use
 */
void UserFunctionDialog::saveFunction() {
  // select one of user-defined categories
  QString cur_category = getCurrentCategory();

  if (cur_category == "Base" || cur_category == "Built-in") {
    cur_category = "";
  }

  auto *dlg = new InputFunctionNameDialog(this, cur_category);
  if (dlg->exec() == QDialog::Accepted) {
    QString cat;
    QString fun;
    QString comment;
    dlg->getFunctionName(cat, fun, comment);
    if (fun.isEmpty()) {
      QMessageBox::critical(this, "Mantid - Error", "The function name is empty");
      return;
    }
    // check if the category already exists
    QList<QListWidgetItem *> items = m_uiForm.lstCategory->findItems(cat, Qt::MatchExactly);
    if (!items.isEmpty()) { // check if a function with this name already exists
      const QSet<QString> functions = functionNames(cat);
      QSet<QString>::const_iterator found = functions.find(fun);
      if (found != functions.end() &&
          QMessageBox::question(this, "Mantid",
                                "A function with name " + fun + " already exists in category " + cat +
                                    ".\n"
                                    "Would you like to replace it?",
                                QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
        return;
      }
    }
    QString expr = m_uiForm.teUserFunction->toPlainText();
    setFunction(cat, fun, expr, comment);
    updateCategories();
  } // QDialog::Accepted
  saveToFile();
}

void UserFunctionDialog::saveToFile() {
  QFile funFile(QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getUserPropertiesDir()) +
                "Mantid.user.functions");
  if (funFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMap<QString, QString>::const_iterator it = m_funs.begin();
    for (; it != m_funs.end(); ++it) {
      QTextStream out(&funFile);
      QStringList cn = it.key().split('.');
      if (cn[0] != "Base" && cn[0] != "Built-in") {
        out << it.key() << "=" << it.value() << '\n';
      }
    }
  }
}

/**
 * Remove the current function
 */
void UserFunctionDialog::removeCurrentFunction() {
  QString cat = m_uiForm.lstCategory->currentItem()->text();
  if (isBuiltin(cat) || (m_uiForm.lstFunction->currentItem() == nullptr)) {
    return;
  }

  QString fun = m_uiForm.lstFunction->currentItem()->text();
  if (QMessageBox::question(this, "Mantid", "Are you sure you want to remove function " + fun + "?",
                            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
    QString fun_key = cat + "." + fun;
    QMap<QString, QString>::iterator it = m_funs.find(fun_key);
    if (it != m_funs.end()) {
      m_funs.erase(it);
      it = m_funs.find(fun_key + ".comment");
      if (it != m_funs.end()) {
        m_funs.erase(it);
      }
    }
  }
  selectCategory(cat);
  saveToFile();
}

QStringList UserFunctionDialog::categories() const {
  QStringList out;
  for (int i = 0; i < m_uiForm.lstCategory->count(); ++i) {
    out << m_uiForm.lstCategory->item(i)->text();
  }
  return out;
}

bool UserFunctionDialog::eventFilter(QObject *obj, QEvent *ev) {
  if (ev->type() == QEvent::KeyPress) {
    auto *keyEvent = static_cast<QKeyEvent *>(ev);
    if (keyEvent->key() == Qt::Key_Return) {
      return true;
    }
  }

  // standard event processing
  return QObject::eventFilter(obj, ev);
}

/**
 * Get the expression for saved function in category cat with name fun. If any
 * of the
 * arguments are empty string or function does not exist return empty string.
 * @param cat :: The category
 * @param fun :: The name of the function
 * @return An expression that can be used as mu::Parser formula
 */
QString UserFunctionDialog::getFunction(const QString &cat, const QString &fun) const {
  if (cat.isEmpty() || fun.isEmpty())
    return "";
  QMap<QString, QString>::const_iterator it = m_funs.find(cat + "." + fun);
  if (it != m_funs.end())
    return it.value();
  return "";
}

/**
 * Get the comment for saved function in category cat with name fun. If any of
 * the
 * arguments are empty string or function does not exist return empty string.
 * @param cat :: The category
 * @param fun :: The name of the function
 */
QString UserFunctionDialog::getComment(const QString &cat, const QString &fun) const {
  if (cat.isEmpty() || fun.isEmpty())
    return "";
  QMap<QString, QString>::const_iterator it = m_funs.find(cat + "." + fun + ".comment");
  if (it != m_funs.end())
    return it.value();
  return "";
}

/**
 * Set an expression to a new function in category cat and with name fun. If any
 * of the
 * arguments are empty string does nothing.
 * @param cat :: The category
 * @param fun :: The name of the function
 * @param expr :: The expression
 * @param comment :: The comment
 */
void UserFunctionDialog::setFunction(const QString &cat, const QString &fun, const QString &expr,
                                     const QString &comment) {
  if (cat.isEmpty() || fun.isEmpty() || expr.isEmpty())
    return;
  // if (cat == "Base" || cat == "Built-in") return;
  QString fun_key = cat + "." + fun;
  m_funs[fun_key] = expr;
  QString cmnt_key = fun_key + ".comment";
  if (!comment.isEmpty()) {
    m_funs[cmnt_key] = comment;
  } else {
    QMap<QString, QString>::iterator it = m_funs.find(cmnt_key);
    if (it != m_funs.end()) {
      m_funs.erase(it);
    }
  }
}

/**
 * Checks if a category is a buil-in one and cannot be changed
 */
bool UserFunctionDialog::isBuiltin(const QString &cat) const { return cat == "Base" || cat == "Built-in"; }

/**
 * Open the help wiki page in the web browser.
 */
void UserFunctionDialog::helpClicked() { HelpWindow::showPage(QUrl("workbench/userfunctiondialog.html")); }

/**
 * Constructor
 * @param parent :: The parent for this dialog
 * @param category :: The initial suggestion for the category
 */
InputFunctionNameDialog::InputFunctionNameDialog(QWidget *parent, const QString &category) : QDialog(parent) {
  auto *layout = new QVBoxLayout();
  layout->addWidget(new QLabel("Enter new or select a category"));
  QStringList cats = (static_cast<UserFunctionDialog *>(parent))->categories();
  cats.removeOne("Base");
  cats.removeOne("Built-in");
  m_category = new QComboBox();
  m_category->addItems(cats);
  m_category->setEditable(true);
  int index = m_category->findText(category);
  if (index >= 0) {
    m_category->setCurrentIndex(index);
  }
  layout->addWidget(m_category);
  connect(m_category, SIGNAL(currentIndexChanged(const QString &)), parent, SLOT(selectCategory(const QString &)));
  layout->addWidget(new QLabel("Enter a name for the new function"));
  m_name = new QLineEdit();
  layout->addWidget(m_name);
  layout->addWidget(new QLabel("Enter a comment"));
  m_comment = new QTextEdit();
  layout->addWidget(m_comment);

  auto *buttons = new QDialogButtonBox();
  buttons->addButton("OK", QDialogButtonBox::AcceptRole);
  buttons->addButton("Cancel", QDialogButtonBox::RejectRole);
  buttons->setCenterButtons(true);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
  layout->addWidget(buttons);
  setLayout(layout);
}

/**
 * Return the entered category and function name and comment
 * @param category :: A string to recieve the category
 * @param name :: A string to recieve the function name
 * @param comment ::
 */
void InputFunctionNameDialog::getFunctionName(QString &category, QString &name, QString &comment) {
  category = m_category->currentText();
  name = m_name->text();
  comment = m_comment->toPlainText();
}
