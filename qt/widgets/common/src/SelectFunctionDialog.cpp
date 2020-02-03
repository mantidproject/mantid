// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
#include <QVBoxLayout>

namespace MantidQt {
namespace MantidWidgets {

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

  auto &factory = Mantid::API::FunctionFactory::Instance();
  auto registeredFunctions = factory.getFunctionNamesGUI();
  // Add functions to each of the categories. If it appears in more than one
  // category then add to both
  // Store in a map. Key = category. Value = vector of fit functions belonging
  // to that category.
  std::map<std::string, std::vector<std::string>> categories;
  for (const auto &registeredFunction : registeredFunctions) {
    auto f = factory.createFunction(registeredFunction);
    std::vector<std::string> tempCategories = f->categories();
    for (size_t j = 0; j < tempCategories.size(); ++j) {
      categories[tempCategories[boost::lexical_cast<int>(j)]].emplace_back(
          registeredFunction);
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

  // keeps track of categories added to the tree
  QMap<QString, QTreeWidgetItem *> categories;

  for (const auto &entry : categoryFunctionsMap) {

    QString categoryName = QString::fromStdString(entry.first);
    QStringList subCategories = categoryName.split('\\');
    if (!categories.contains(categoryName)) {
      if (subCategories.size() == 1) {
        if (showCategory(entry.first)) {
          QTreeWidgetItem *catItem =
              new QTreeWidgetItem(QStringList(categoryName));
          categories.insert(categoryName, catItem);
          m_form->fitTree->addTopLevelItem(catItem);
          for (const auto &function : entry.second) {
            QTreeWidgetItem *fit = new QTreeWidgetItem(catItem);
            fit->setText(0, QString::fromStdString(function));
          }
        }
      } else {
        // go through the path and add the folders if they don't already exist
        QString currentPath = subCategories[0];
        QTreeWidgetItem *catItem = nullptr;
        int subCategoryNo = subCategories.size();
        bool show = false;
        for (int j = 0; j < subCategoryNo; ++j) {
          if (showCategory(subCategories[j].toStdString())) {
            show = true;
          }
        }
        if (show) {
          for (int j = 0; j < subCategoryNo; ++j) {
            if (categories.contains(currentPath)) {
              catItem = categories[currentPath];
            } else {
              QTreeWidgetItem *newCatItem =
                  new QTreeWidgetItem(QStringList(subCategories[j]));
              categories.insert(currentPath, newCatItem);
              if (!catItem) {
                m_form->fitTree->addTopLevelItem(newCatItem);
              } else {
                catItem->addChild(newCatItem);
              }
              catItem = newCatItem;
            }
            if (j != subCategoryNo - 1)
              currentPath += "\\" + subCategories[j + 1];
            else {
              // This is the end of the path so add the functions
              for (const auto &function : entry.second) {
                QTreeWidgetItem *fit = new QTreeWidgetItem(catItem);
                fit->setText(0, QString::fromStdString(function));
              }
            }
          }
        }
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

} // namespace MantidWidgets
} // namespace MantidQt
