//-------------------------------------------
// Includes
//-------------------------------------------
#include "MantidQtMantidWidgets/SelectFunctionDialog.h"
#include "ui_SelectFunctionDialog.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"

#include <boost/lexical_cast.hpp>

#include <QLabel>
#include <QPushButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QRegExp>
#include <QGridLayout>
#include <QVBoxLayout>

/**
 * Constructor.
 * @param parent :: A parent widget
 */
SelectFunctionDialog::SelectFunctionDialog(QWidget *parent)
  :QDialog(parent),m_form(new Ui::SelectFunctionDialog)
{
  m_form->setupUi(this);

  auto registeredFunctions = Mantid::API::FunctionFactory::Instance().getKeys();
  // Add functions to each of the categories. If it appears in more than one category then add to both
  // Store in a map. Key = category. Value = vector of fit functions belonging to that category.
  std::map<std::string, std::vector<std::string> > categories;
  for (size_t i=0; i<registeredFunctions.size(); ++i)
  {
    boost::shared_ptr<Mantid::API::IFunction> f = Mantid::API::FunctionFactory::Instance().createFunction(registeredFunctions[i]);
    std::vector<std::string> tempCategories = f->categories();
    for (size_t j=0; j<tempCategories.size(); ++j)
    {
      categories[tempCategories[boost::lexical_cast<int>(j)] ].push_back(registeredFunctions[i]);
    }
  }
  
  // Construct the QTreeWidget based on the map information of categories and their respective fit functions.
  std::map<std::string, std::vector<std::string> >::const_iterator sItr = categories.end();
  for (std::map<std::string, std::vector<std::string> >::const_iterator itr = categories.begin(); itr != sItr; ++itr)
  {
    QTreeWidgetItem *category = new QTreeWidgetItem(m_form->fitTree);
    category->setText(0, QString::fromStdString(itr->first) );
    
    std::vector<std::string>::const_iterator fitItrEnd = itr->second.end();
    for (std::vector<std::string>::const_iterator fitItrBegin = itr->second.begin(); fitItrBegin != fitItrEnd; ++fitItrBegin)
    {
      QTreeWidgetItem *fit = new QTreeWidgetItem(category);
      fit->setText(0, QString::fromStdString(fitItrBegin[0]) );      
    }
  }

  connect(m_form->fitTree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(accept()));
  m_form->fitTree->setToolTip("Select a function type and press OK.");
}

SelectFunctionDialog::~SelectFunctionDialog()
{
  delete m_form;
}

/**
 * Return selected function
 */
QString SelectFunctionDialog::getFunction() const
{
  QList<QTreeWidgetItem*> items(m_form->fitTree->selectedItems() );
  if (items.size() != 1)
  {
    return "";
  }
  
  if (items[0]->parent() == NULL)
  {
    return "";
  }

  return items[0]->text(0);
}
