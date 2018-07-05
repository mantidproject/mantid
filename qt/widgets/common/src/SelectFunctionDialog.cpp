//-------------------------------------------
// Includes
//-------------------------------------------
#include "MantidQtWidgets/Common/SelectFunctionDialog.h"
#include "ui_SelectFunctionDialog.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"

#include <boost/lexical_cast.hpp>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRegExp>
#include <QVBoxLayout>

/**
 * Constructor.
 * @param parent :: A parent widget
 */
SelectFunctionDialog::SelectFunctionDialog(QWidget *parent)
    : SelectFunctionDialog(parent, std::vector<std::string>()) {}

/**
 * Constructor with categories to restrict to
 * @param parent :: A parent widget
 * @param restrictions :: List of categories to restrict to
 */
SelectFunctionDialog::SelectFunctionDialog(
    QWidget *parent, const std::vector<std::string> &restrictions)
    : QDialog(parent), m_form(new Ui::SelectFunctionDialog) {
  m_form->setupUi(this);

  auto registeredFunctions = Mantid::API::FunctionFactory::Instance().getKeys();
  // Add functions to each of the categories. If it appears in more than one
  // category then add to both
  // Store in a map. Key = category. Value = vector of fit functions belonging
  // to that category.
  std::map<std::string, std::vector<std::string>> categories;
  for (size_t i = 0; i < registeredFunctions.size(); ++i) {
    boost::shared_ptr<Mantid::API::IFunction> f =
        Mantid::API::FunctionFactory::Instance().createFunction(
            registeredFunctions[i]);
    std::vector<std::string> tempCategories = f->categories();
    for (size_t j = 0; j < tempCategories.size(); ++j) {
      categories[tempCategories[boost::lexical_cast<int>(j)]].push_back(
          registeredFunctions[i]);
    }
  }

  // Construct the QTreeWidget based on the map information of categories and
  // their respective fit functions.
  constructFunctionTree(categories, restrictions);

  connect(m_form->fitTree, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)),
          this, SLOT(accept()));
  m_form->fitTree->setToolTip("Select a function type and press OK.");
}

/**
 * Construct the QTreeWidget based on the map information of categories and
 * their respective fit functions
 * @param categoryFunctionsMap :: [input] Map of categories to functions
 * @param restrictions :: [input] List of categories to restrict choice to. If
 * empty, no restriction.
 */
void SelectFunctionDialog::constructFunctionTree(
    const std::map<std::string, std::vector<std::string>> &categoryFunctionsMap,
    const std::vector<std::string> &restrictions) {
  // Don't show category if it's not in the list (unless list is empty)
  auto showCategory = [&restrictions](const std::string &name) {
    if (restrictions.empty()) {
      return true;
    } else {
      return (std::find(restrictions.begin(), restrictions.end(), name) !=
              restrictions.end());
    }
  };

  for (const auto &entry : categoryFunctionsMap) {
    if (showCategory(entry.first)) {
      QTreeWidgetItem *category = new QTreeWidgetItem(m_form->fitTree);
      category->setText(0, QString::fromStdString(entry.first));

      for (const auto &function : entry.second) {
        QTreeWidgetItem *fit = new QTreeWidgetItem(category);
        fit->setText(0, QString::fromStdString(function));
      }
    }
  }
}

SelectFunctionDialog::~SelectFunctionDialog() { delete m_form; }

/**
 * Return selected function
 */
QString SelectFunctionDialog::getFunction() const {
  QList<QTreeWidgetItem *> items(m_form->fitTree->selectedItems());
  if (items.size() != 1) {
    return "";
  }

  if (items[0]->parent() == nullptr) {
    return "";
  }

  return items[0]->text(0);
}
