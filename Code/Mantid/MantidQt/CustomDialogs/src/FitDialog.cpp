//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidQtCustomDialogs/FitDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
// Qt
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QUrl>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QFileInfo>

// Mantid
#include "MantidKernel/Property.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/IFunctionMD.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"

using namespace MantidQt::API;

namespace MantidQt
{
namespace CustomDialogs
{

// Declare the dialog. Name must match the class name
DECLARE_DIALOG(FitDialog);

//------------------------------------------------------
// InputWorkspaceWidget methods
//------------------------------------------------------

/**
 * Constructor.
 * @param parent :: Parent dialog.
 * @param wsPropName :: Input workspace property name. 
 */
InputWorkspaceWidget::InputWorkspaceWidget(FitDialog* parent, int domainIndex):
QWidget(parent),
m_fitDialog(parent),
m_domainIndex(domainIndex),
m_dynamicProperties(NULL)
{
  m_wsPropName = "InputWorkspace";
  if ( domainIndex > 0 )
  {
    m_wsPropName += "_" + QString::number(domainIndex);
  }
  m_layout = new QVBoxLayout(this);
  m_workspaceName = new QComboBox(this);
  m_layout->addWidget(m_workspaceName);

  QStringList allowedValues = getAllowedPropertyValues(m_wsPropName);
  m_workspaceName->clear();
  m_workspaceName->insertItems(0,allowedValues);
  connect(m_workspaceName, SIGNAL(currentIndexChanged(int)),this, SLOT(setDynamicProperties()));

  setDynamicProperties();
}

/**
 * Is ws name set?
 */
bool InputWorkspaceWidget::isWSNameSet() const
{
  QString wsName = m_workspaceName->currentText();
  return !wsName.isEmpty();
}

/**
 * Is the workspace MW?
 */
bool InputWorkspaceWidget::isMatrixWorkspace() const
{
  QString wsName = m_workspaceName->currentText();
  if ( wsName.isEmpty() ) return false;
  try
  {
    auto ws = Mantid::API::AnalysisDataService::Instance().retrieve(wsName.toStdString());
    if ( dynamic_cast<Mantid::API::MatrixWorkspace*>(ws.get()) ) return true;
    return false;
  }
  catch(...)
  {
    return false;
  }
}

/**
 * Is the workspace MD?
 */
bool InputWorkspaceWidget::isMDWorkspace() const
{
  QString wsName = m_workspaceName->currentText();
  if ( wsName.isEmpty() ) return false;
  try
  {
    auto ws = Mantid::API::AnalysisDataService::Instance().retrieve(wsName.toStdString());
    if ( dynamic_cast<Mantid::API::IMDWorkspace*>(ws.get()) ) return true;
    return false;
  }
  catch(...)
  {
    return false;
  }
}

/**
 * Is current workspace supported by Fit?
 */
bool InputWorkspaceWidget::isWorkspaceSupported() const
{
  return isMatrixWorkspace() || isMDWorkspace();
}


/**
 * Set the dynamic properties
 */
void InputWorkspaceWidget::setDynamicProperties()
{
  if ( !isWSNameSet() ) return;

  auto item = m_layout->takeAt(1);
  if ( item )
  {
    QWidget *w = item->widget();
    delete item;
    delete w;
  }
  
  m_dynamicProperties = NULL;
  
  if ( m_fitDialog->isMD() )
  {
    // empty space
    m_layout->insertWidget(1, new QLabel(" ") );
  }
  else if ( isMatrixWorkspace() )
  {
    m_dynamicProperties = new MWPropertiesWidget(this);
    m_layout->insertWidget( 1, m_dynamicProperties );
  }
  else if ( isMDWorkspace() )
  {
    // empty space
    m_layout->insertWidget(1, new QLabel(" ") );
  }
  else
  {
    m_layout->insertWidget(1, new QLabel("Workspace of this type is not supported") );
  }
}

/// Get workspace name
QString InputWorkspaceWidget::getWorkspaceName() const 
{
  return m_workspaceName->currentText();
}

/**
 * Set a property
 * @param propName :: Property name
 * @param propValue :: Property value
 */
void InputWorkspaceWidget::setPropertyValue(const QString& propName, const QString& propValue)
{
  if ( m_fitDialog->getAlgorithm()->existsProperty(propName.toStdString()) )
  {
    m_fitDialog->getAlgorithm()->setPropertyValue(propName.toStdString(),propValue.toStdString());
    m_fitDialog->storePropertyValue(propName,propValue);
  }
}

/**
 * Set all workspace properties
 */
void InputWorkspaceWidget::setProperties()
{
  if ( !isWorkspaceSupported() ) return;
  setPropertyValue(m_wsPropName,getWorkspaceName());
  if ( m_dynamicProperties )
  {
    m_dynamicProperties->setProperties();
  }
}



/**
 * Constructor.
 */
MWPropertiesWidget::MWPropertiesWidget(InputWorkspaceWidget* parent):DynamicPropertiesWidget(parent)
{
  m_workspaceIndex = new QSpinBox(this);
  m_startX = new QLineEdit(this);
  m_endX = new QLineEdit(this);

  auto layout = new QGridLayout(this);
  layout->addWidget(new QLabel("Workspace index"),0,0);
  layout->addWidget(m_workspaceIndex,0,1);
  layout->addWidget(new QLabel("StartX"),1,0);
  layout->addWidget(m_startX,1,1);
  layout->addWidget(new QLabel("EndX"),2,0);
  layout->addWidget(m_endX,2,1);

  QString wsName = parent->getWorkspaceName();
  if ( wsName.isEmpty() ) return;
  try
  {
    auto ws = dynamic_cast<Mantid::API::MatrixWorkspace*>(
      Mantid::API::AnalysisDataService::Instance().retrieve(wsName.toStdString()).get());
    if ( ws )
    {
      m_workspaceIndex->setRange(0, static_cast<int>(ws->getNumberHistograms()));
      if ( ws->blocksize() > 0 )
      {
        const Mantid::MantidVec& x = ws->readX(0);
        m_startX->setText( QString::number(x.front()) );
        m_endX->setText( QString::number(x.back()) );
      }
    }
  }
  catch(...)
  {
  }
}

/**
 * Initialize the child widgets with stored and allowed values
 */
void MWPropertiesWidget::init()
{
}

/**
 * Set all workspace properties
 */
void MWPropertiesWidget::setProperties()
{
  QString wsIndexName = "WorkspaceIndex";
  QString startXName = "StartX";
  QString endXName = "EndX";

  int domainIndex = m_wsWidget->getDomainIndex();
  if ( domainIndex > 0 )
  {
    QString suffix = "_" + QString::number(domainIndex);
    wsIndexName += suffix;
    startXName += suffix;
    endXName += suffix;
  }

  QString value = m_workspaceIndex->text();
  if ( !value.isEmpty() )
  {
    m_wsWidget->setPropertyValue(wsIndexName,value);
  }
  value = m_startX->text();
  if ( !value.isEmpty() )
  {
    m_wsWidget->setPropertyValue(startXName,value);
  }
  value = m_endX->text();
  if ( !value.isEmpty() )
  {
    m_wsWidget->setPropertyValue(endXName,value);
  }
}

//------------------------------------------------------
// FitDialog methods
//------------------------------------------------------

/// Default constructor
FitDialog:: FitDialog(QWidget *parent) 
  : API::AlgorithmDialog(parent), m_form()
{
}

/// Initialize the layout
void FitDialog::initLayout()
{
  m_form.setupUi(this);
  m_form.dialogLayout->addLayout(this->createDefaultButtonLayout());

  tieStaticWidgets(true);
}


/**
* Save the input after OK is clicked
*/
void FitDialog::saveInput()
{
  QString funStr = m_form.function->getFunctionString();
  if ( !funStr.isEmpty() )
  {
    storePropertyValue("Function",funStr);
  }
  AlgorithmDialog::saveInput();
}

/**
 * Parse input
 */
void FitDialog::parseInput()
{
  QString funStr = m_form.function->getFunctionString();
  if ( !funStr.isEmpty() )
  {
    storePropertyValue("Function",funStr);
    getAlgorithm()->setPropertyValue("Function",funStr.toStdString());
  }
  else
  {
    // Cannot set any other properties until Function is set
    return;
  }
  foreach(QWidget* t, m_tabs)
  {
    auto iww = dynamic_cast<InputWorkspaceWidget*>(t);
    if ( iww )
    {
      iww->setProperties();
    }
  }
}

/**
* Tie static widgets to their properties
* @param readHistory :: If true then the history will be re read.
*/
void FitDialog::tieStaticWidgets(const bool readHistory)
{

  QString funValue = getStoredPropertyValue("Function");
  if ( !funValue.isEmpty() )
  {
    m_form.function->setFunction(funValue);
  }

  //m_staticProperties << "Function" << "InputWorkspace" << "CreateOutput" << "Output"
  //  << "MaxIterations" << "Minimizer" << "CostFunction";

  tie(m_form.chbCreateOutput, "CreateOutput", m_form.staticLayout, readHistory);
  tie(m_form.leOutput, "Output", m_form.staticLayout, readHistory);
  tie(m_form.leMaxIterations, "MaxIterations", m_form.staticLayout, readHistory);

  m_form.cbMinimizer->addItems(getAllowedPropertyValues("Minimizer"));
  tie(m_form.cbMinimizer, "Minimizer", m_form.staticLayout, readHistory);

  m_form.cbCostFunction->addItems(getAllowedPropertyValues("CostFunction"));
  tie(m_form.cbCostFunction, "CostFunction", m_form.staticLayout, readHistory);

  createInputWorkspaceWidgets();
}

/**
 * Create InputWorkspaceWidgets and populate the tabs of the tab widget
 */
void FitDialog::createInputWorkspaceWidgets()
{
  m_form.tabWidget->clear();
  foreach(QWidget* t, m_tabs)
  {
    delete t;
  }
  m_tabs.clear();
  auto tab = new InputWorkspaceWidget(this,0);
  m_form.tabWidget->addTab(tab,"InputWorkspace");
  m_tabs << tab;

  auto fun = m_form.function->getFunction();
  if ( !fun ) return;
  auto multid = boost::dynamic_pointer_cast<Mantid::API::MultiDomainFunction>(fun);
  if ( multid )
  {
    // number of domains that the function expects
    size_t nd = multid->getMaxIndex();
    for(size_t i = 1; i < nd; ++i)
    {
      QString propName = "InputWorkspace_" + QString::number(i);
      auto t = new InputWorkspaceWidget( this, static_cast<int>(i) );
      m_form.tabWidget->addTab( t, propName );
      m_tabs << t;
    }
  }
}


/**
* Clear the old widgets for a new Loader type
* @param layout :: The layout containing the child layouts/widgets
*/
void FitDialog::removeOldInputWidgets()
{
  //auto layout = m_form.topLayout;
  //layout->setEnabled(false);
  //// Remove the old widgets if necessary
  //if( layout->count() > 4 )
  //{
  //  int count = layout->count();
  //  while( count > 4 )
  //  {
  //    QLayoutItem *child = layout->takeAt(count - 1);
  //    if( QWidget *w = child->widget() )
  //    {
  //      w->deleteLater();
  //    }
  //    else if( QLayout *l = child->layout() )
  //    {
  //      QLayoutItem *subChild(NULL);
  //      while( (subChild = l->takeAt(0)) != NULL )
  //      {
  //        subChild->widget()->deleteLater();
  //      }
  //    }
  //    count = layout->count();
  //  }
  //}
  //layout->setEnabled(true);
  //m_dynamicLabels.clear();
  //m_dynamicEditors.clear();
}

/**
* Create the dynamic widgets for the concrete loader
*/
void FitDialog::createDynamicLayout()
{
  //int index = m_form.topLayout->rowCount();
  //auto properties = getAlgorithm()->getProperties();
  //for(auto prop = properties.begin(); prop != properties.end(); ++prop)
  //{
  //  QString propName = QString::fromStdString((**prop).name());
  //  if ( !m_staticProperties.contains(propName) && 
  //    !m_dynamicLabels.contains(propName) && 
  //    (**prop).direction() == Mantid::Kernel::Direction::Input)
  //  {
  //    untie(propName);
  //    QLabel *label = new QLabel(propName,this);
  //    QLineEdit *edit = new QLineEdit(this);
  //    m_form.topLayout->addWidget(label,index,0);
  //    m_form.topLayout->addWidget(edit,index,1);
  //    tie(edit, propName, m_form.topLayout, true);
  //    m_dynamicLabels.insert(propName,label);
  //    m_dynamicEditors.insert(propName,edit);
  //    ++index;
  //  }
  //}
  //m_form.mainLayout->invalidate();
}

void FitDialog::workspaceChanged(const QString&)
{
  this->setPropertyValues();
  removeOldInputWidgets();
  createDynamicLayout();
}

void FitDialog::functionChanged()
{
  //this->setPropertyValues();
  //removeOldInputWidgets();
  //createDynamicLayout();
}

/**
  * Return property value stored in history
  * @param propName :: A property name
  */
QString FitDialog::getStoredPropertyValue(const QString& propName) const
{
  // Get the value from either the previous input store or from Python argument
  QString value("");
  Mantid::Kernel::Property *property = getAlgorithmProperty(propName);

  if( !isForScript() )
  {
    value = m_propertyValueMap.value(propName);
    if( value.isEmpty() )
    {
      value = AlgorithmInputHistory::Instance().previousInput(m_algName, propName);
    }
  }
  else
  {
    if( !property ) return "";
    value = m_propertyValueMap.value(propName);
  }
  return value;
}

/**
 * Get allowed values for a property
 * @param propName :: A property name
 */
QStringList FitDialog::getAllowedPropertyValues(const QString& propName) const
{
  QStringList out;
  std::set<std::string> workspaces = getAlgorithmProperty(propName)->allowedValues();
  for( std::set<std::string>::const_iterator itr = workspaces.begin(); itr != workspaces.end(); ++itr )
  {
    out << QString::fromStdString(*itr);
  }
  return out;
}

namespace
{
  /**
   * Helper function to check if a function is an MD one.
   * @param fun :: Function to check
   */
  bool isFunctionMD(Mantid::API::IFunction_sptr fun)
  {
    auto cf = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(fun);
    if ( !cf ) return static_cast<bool>( boost::dynamic_pointer_cast<Mantid::API::IFunctionMD>(fun) );
    for( size_t i = 0; i < cf->nFunctions(); ++i)
    {
      bool yes = isFunctionMD( cf->getFunction(i) );
      if ( yes ) return true;
    }
    return false;
  }
}

/**
 * Is the function MD?
 */
bool FitDialog::isMD() const
{
  auto fun = m_form.function->getFunction();
  return isFunctionMD(fun);
}


} // CustomDialogs
} // MantidQt
