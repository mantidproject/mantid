// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidQtWidgets/Plugins/AlgorithmDialogs/FitDialog.h"
#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"
// Qt
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopWidget>
#include <QFileInfo>
#include <QUrl>

// Mantid
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IFunctionMD.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidKernel/Property.h"

#include <limits>

using namespace MantidQt::API;

namespace MantidQt::CustomDialogs {

// Declare the dialog. Name must match the class name
DECLARE_DIALOG(FitDialog)

//------------------------------------------------------
// InputWorkspaceWidget methods
//------------------------------------------------------

/**
 * Constructor.
 * @param parent :: Parent dialog.
 * @param domainIndex: Number that allows to identify the InputWorkspace formed
 * with the followin rule InputWorkspace_[domainIndex]
 */
InputWorkspaceWidget::InputWorkspaceWidget(FitDialog *parent, int domainIndex)
    : QWidget(parent), m_fitDialog(parent), m_domainIndex(domainIndex), m_wsPropName("InputWorkspace"),
      m_workspaceName(new QComboBox(this)), m_dynamicProperties(nullptr), m_layout(new QVBoxLayout(this)) {
  if (domainIndex > 0) {
    m_wsPropName += "_" + QString::number(domainIndex);
  }
  m_layout->addWidget(m_workspaceName);

  QStringList allowedValues = getAllowedPropertyValues(m_wsPropName);
  m_workspaceName->clear();
  m_workspaceName->insertItems(0, allowedValues);
  connect(m_workspaceName, SIGNAL(currentIndexChanged(int)), this, SLOT(setDynamicProperties()));

  setDynamicProperties();
}

/**
 * Is ws name set?
 */
bool InputWorkspaceWidget::isWSNameSet() const {
  QString wsName = m_workspaceName->currentText();
  return !wsName.isEmpty();
}

/**
 * Is the workspace MW?
 */
bool InputWorkspaceWidget::isMatrixWorkspace() const {
  QString wsName = m_workspaceName->currentText();
  if (wsName.isEmpty())
    return false;
  try {
    auto ws = Mantid::API::AnalysisDataService::Instance().retrieve(wsName.toStdString());
    return dynamic_cast<Mantid::API::MatrixWorkspace *>(ws.get()) != nullptr;
  } catch (...) {
    return false;
  }
}

/**
 * Is the workspace MD?
 */
bool InputWorkspaceWidget::isMDWorkspace() const {
  QString wsName = m_workspaceName->currentText();
  if (wsName.isEmpty())
    return false;
  try {
    auto ws = Mantid::API::AnalysisDataService::Instance().retrieve(wsName.toStdString());
    return dynamic_cast<Mantid::API::IMDWorkspace *>(ws.get()) != nullptr;
  } catch (...) {
    return false;
  }
}

/**
 * Is current workspace supported by Fit?
 */
bool InputWorkspaceWidget::isWorkspaceSupported() const { return isMatrixWorkspace() || isMDWorkspace(); }

/**
 * Set the dynamic properties
 */
void InputWorkspaceWidget::setDynamicProperties() {
  if (!isWSNameSet())
    return;

  auto item = m_layout->takeAt(1);
  if (item) {
    QWidget *w = item->widget();
    delete item;
    delete w;
  }

  m_dynamicProperties = nullptr;

  if (m_fitDialog->isMD()) {
    if (isMDWorkspace()) {
      m_dynamicProperties = new MDPropertiesWidget(this);
      m_layout->insertWidget(1, m_dynamicProperties);
    } else {
      m_layout->insertWidget(1, new QLabel("MD Workspace is expected"));
    }
  } else if (isMatrixWorkspace()) {
    m_dynamicProperties = new MWPropertiesWidget(this);
    m_layout->insertWidget(1, m_dynamicProperties);
  } else {
    m_layout->insertWidget(1, new QLabel("Workspace of this type is not supported"));
  }
}

/// Get workspace name
QString InputWorkspaceWidget::getWorkspaceName() const { return m_workspaceName->currentText(); }

/// Set workspace name
void InputWorkspaceWidget::setWorkspaceName(const QString &wsName) {
  int i = m_workspaceName->findText(wsName);
  if (i >= 0) {
    m_workspaceName->setCurrentIndex(i);
  }
}

/**
 * Set a property
 * @param propName :: Property name
 * @param propValue :: Property value
 */
void InputWorkspaceWidget::setPropertyValue(const QString &propName, const QString &propValue) {
  if (m_fitDialog->getAlgorithm()->existsProperty(propName.toStdString())) {
    m_fitDialog->getAlgorithm()->setPropertyValue(propName.toStdString(), propValue.toStdString());
    m_fitDialog->storePropertyValue(propName, propValue);
  }
}

/**
 * Set all workspace properties
 */
void InputWorkspaceWidget::setProperties() {
  if (!isWorkspaceSupported())
    return;
  setPropertyValue(m_wsPropName, getWorkspaceName());
  if (m_dynamicProperties) {
    m_dynamicProperties->setProperties();
  }
}

/**
 * Constructor.
 */
MWPropertiesWidget::MWPropertiesWidget(InputWorkspaceWidget *parent) : DynamicPropertiesWidget(parent) {
  m_workspaceIndex = new QSpinBox(this);
  m_startX = new QLineEdit(this);
  m_endX = new QLineEdit(this);

  auto layout = new QGridLayout(this);
  layout->addWidget(new QLabel("Workspace index"), 0, 0);
  layout->addWidget(m_workspaceIndex, 0, 1);
  layout->addWidget(new QLabel("StartX"), 1, 0);
  layout->addWidget(m_startX, 1, 1);
  layout->addWidget(new QLabel("EndX"), 2, 0);
  layout->addWidget(m_endX, 2, 1);

  if (parent->getDomainType() > 0) {
    m_maxSize = new QSpinBox(this);
    m_maxSize->setMinimum(1);
    m_maxSize->setMaximum(std::numeric_limits<int>::max());
    layout->addWidget(new QLabel("Maximum size"), 3, 0);
    layout->addWidget(m_maxSize, 3, 1);
  } else {
    m_maxSize = nullptr;
  }

  QString wsName = parent->getWorkspaceName();
  if (wsName.isEmpty())
    return;
  try {
    auto ws = dynamic_cast<Mantid::API::MatrixWorkspace *>(
        Mantid::API::AnalysisDataService::Instance().retrieve(wsName.toStdString()).get());
    if (ws) {
      m_workspaceIndex->setRange(0, static_cast<int>(ws->getNumberHistograms()));
      const Mantid::MantidVec &x = ws->readX(0);
      if (!x.empty()) {
        m_startX->setText(QString::number(x.front()));
        m_endX->setText(QString::number(x.back()));
      }
    }
  } catch (...) {
  }
}

/**
 * Initialize the child widgets with stored and allowed values
 */
void MWPropertiesWidget::init() {}

/**
 * Set all workspace properties
 */
void MWPropertiesWidget::setProperties() {
  QString wsIndexName = "WorkspaceIndex";
  QString startXName = "StartX";
  QString endXName = "EndX";
  QString maxSizeName = "MaxSize";

  int domainIndex = m_wsWidget->getDomainIndex();
  if (domainIndex > 0) {
    QString suffix = "_" + QString::number(domainIndex);
    wsIndexName += suffix;
    startXName += suffix;
    endXName += suffix;
    maxSizeName += suffix;
  }

  QString value = m_workspaceIndex->text();
  if (!value.isEmpty()) {
    m_wsWidget->setPropertyValue(wsIndexName, value);
  }
  value = m_startX->text();
  if (!value.isEmpty()) {
    m_wsWidget->setPropertyValue(startXName, value);
  }
  value = m_endX->text();
  if (!value.isEmpty()) {
    m_wsWidget->setPropertyValue(endXName, value);
  }

  if (m_wsWidget->getDomainType() > 0) {
    value = m_maxSize->text();
    m_wsWidget->setPropertyValue(maxSizeName, value);
  }
}

//------------------------------------------------------
// MDPropertiesWidget methods
//------------------------------------------------------
/**
 * Constructor.
 * @param parent :: Parent InputWorkspaceWidget tab.
 */
MDPropertiesWidget::MDPropertiesWidget(InputWorkspaceWidget *parent) : DynamicPropertiesWidget(parent) {
  if (parent->getDomainType() > 0) {
    auto layout = new QGridLayout(this);
    m_maxSize = new QSpinBox(this);
    m_maxSize->setMinimum(1);
    m_maxSize->setMaximum(std::numeric_limits<int>::max());
    layout->addWidget(new QLabel("Maximum size"), 3, 0);
    layout->addWidget(m_maxSize, 3, 1);
  } else {
    m_maxSize = nullptr;
  }
}

/**
 * Set all workspace properties
 */
void MDPropertiesWidget::setProperties() {
  QString maxSizeName = "MaxSize";

  int domainIndex = m_wsWidget->getDomainIndex();
  if (domainIndex > 0) {
    QString suffix = "_" + QString::number(domainIndex);
    maxSizeName += suffix;
  }

  if (m_wsWidget->getDomainType() > 0) {
    QString value = m_maxSize->text();
    m_wsWidget->setPropertyValue(maxSizeName, value);
  }
}

//------------------------------------------------------
// FitDialog methods
//------------------------------------------------------

/// Default constructor
FitDialog::FitDialog(QWidget *parent) : API::AlgorithmDialog(parent), m_form() {}

/// Initialize the layout
void FitDialog::initLayout() {
  m_form.setupUi(this);
  m_form.dialogLayout->addLayout(this->createDefaultButtonLayout());

  tieStaticWidgets(true);
}

/**
 * Save the input after OK is clicked
 */
void FitDialog::saveInput() {
  storePropertyValue("DomainType", getDomainTypeString());
  QString funStr = QString::fromStdString(m_form.function->getFunctionString());
  if (!funStr.isEmpty()) {
    storePropertyValue("Function", funStr);
  }
  AlgorithmDialog::saveInput();
}

/**
 * Parse input
 */
void FitDialog::parseInput() {
  // int domainType = getDomainType();
  storePropertyValue("DomainType", getDomainTypeString());
  getAlgorithm()->setPropertyValue("DomainType", getDomainTypeString().toStdString());
  auto funStr = m_form.function->getFunctionString();
  if (!funStr.empty()) {
    storePropertyValue("Function", QString::fromStdString(funStr));
    getAlgorithm()->setPropertyValue("Function", funStr);
  } else {
    // Cannot set any other properties until Function is set
    return;
  }
  foreach (QWidget *t, m_tabs) {
    auto iww = dynamic_cast<InputWorkspaceWidget *>(t);
    if (iww) {
      iww->setProperties();
    }
  }
}

/**
 * Tie static widgets to their properties
 * @param readHistory :: If true then the history will be re read.
 */
void FitDialog::tieStaticWidgets(const bool readHistory) {
  QString funValue = getPreviousValue("Function");
  if (!funValue.isEmpty()) {
    m_form.function->setFunction(funValue.toStdString());
  }

  tie(m_form.chbCreateOutput, "CreateOutput", m_form.staticLayout, readHistory);
  tie(m_form.leOutput, "Output", m_form.staticLayout, readHistory);
  tie(m_form.leMaxIterations, "MaxIterations", m_form.staticLayout, readHistory);

  m_form.cbCostFunction->addItems(getAllowedPropertyValues("CostFunction"));
  tie(m_form.cbCostFunction, "CostFunction", m_form.staticLayout, readHistory);

  QStringList allowedDomainTypes = getAllowedPropertyValues("DomainType");
  // Disable some domain types in the GUI until their imlpementations have been
  // finished
  allowedDomainTypes.removeAll("Sequential");
  allowedDomainTypes.removeAll("Parallel");
  m_form.cbDomainType->addItems(allowedDomainTypes);
  // tie(m_form.cbDomainType, "DomainType", m_form.staticLayout, readHistory);
  connect(m_form.cbDomainType, SIGNAL(currentIndexChanged(int)), this, SLOT(domainTypeChanged()));
  QString domainTypeValue = getPreviousValue("DomainType");
  if (!domainTypeValue.isEmpty()) {
    m_form.cbDomainType->setItemText(-1, domainTypeValue);
  }

  // this creates input workspace widgets and adjusts minimizers list
  // according to the domain type
  domainTypeChanged();

  // read value from history
  tie(m_form.cbMinimizer, "Minimizer", m_form.staticLayout, readHistory);

  auto value = getPreviousValue("InputWorkspace");
  setWorkspaceName(0, value);
}

/**
 * Update user interface when domain type changes.
 */
void FitDialog::domainTypeChanged() {
  getAlgorithm()->setPropertyValue("DomainType", getDomainTypeString().toStdString());
  auto minimizerList = getAllowedPropertyValues("Minimizer");
  if (getDomainType() != 0) {
    minimizerList.removeAll("Levenberg-Marquardt");
  }
  QString currentMinimizer = m_form.cbMinimizer->currentText();
  m_form.cbMinimizer->clear();
  m_form.cbMinimizer->addItems(minimizerList);
  int index = m_form.cbMinimizer->findText(currentMinimizer);
  if (index >= 0) {
    m_form.cbMinimizer->setItemText(-1, currentMinimizer);
  }
  createInputWorkspaceWidgets();
}

/**
 * Create InputWorkspaceWidgets and populate the tabs of the tab widget
 */
void FitDialog::createInputWorkspaceWidgets() {
  m_form.tabWidget->clear();
  QStringList wsNames;
  foreach (QWidget *t, m_tabs) {
    auto tab = dynamic_cast<InputWorkspaceWidget *>(t);
    if (tab) {
      wsNames << tab->getWorkspaceName();
    } else {
      wsNames << "";
    }
    delete t;
  }
  m_tabs.clear();
  auto tab = new InputWorkspaceWidget(this, 0);
  if (!wsNames.isEmpty()) {
    tab->setWorkspaceName(wsNames[0]);
  }
  m_form.tabWidget->addTab(tab, "InputWorkspace");
  m_tabs << tab;

  auto fun = m_form.function->getFunction();
  if (!fun)
    return;
  auto multid = std::dynamic_pointer_cast<Mantid::API::MultiDomainFunction>(fun);
  if (multid) {
    // number of domains that the function expects
    size_t nd = multid->getMaxIndex();
    for (int i = 1; i < static_cast<int>(nd); ++i) {
      QString propName = "InputWorkspace_" + QString::number(i);
      auto t = new InputWorkspaceWidget(this, i);
      if (wsNames.size() > (i)) {
        tab->setWorkspaceName(wsNames[i]);
      }
      m_form.tabWidget->addTab(t, propName);
      m_tabs << t;
    }
  }
}

/**
 * Set i-th workspace name
 * @param i :: Tab index
 * @param wsName :: A workspace name to try to set.
 */
void FitDialog::setWorkspaceName(int i, const QString &wsName) {
  if (i < 0 || i >= m_form.tabWidget->count())
    return;
  auto tab = dynamic_cast<InputWorkspaceWidget *>(m_form.tabWidget->widget(i));
  if (tab)
    tab->setWorkspaceName(wsName);
}

void FitDialog::workspaceChanged(const QString & /*unused*/) { this->setPropertyValues(); }

void FitDialog::functionChanged() {
  // this->setPropertyValues();
  // removeOldInputWidgets();
  // createDynamicLayout();
}

/**
 * Get allowed values for a property
 * @param propName :: A property name
 */
QStringList FitDialog::getAllowedPropertyValues(const QString &propName) const {
  QStringList out;
  std::vector<std::string> workspaces = getAlgorithmProperty(propName)->allowedValues();
  for (std::vector<std::string>::const_iterator itr = workspaces.begin(); itr != workspaces.end(); ++itr) {
    out << QString::fromStdString(*itr);
  }
  return out;
}

namespace {
/**
 * Helper function to check if a function is an MD one.
 * @param fun :: Function to check
 */
bool isFunctionMD(const Mantid::API::IFunction_sptr &fun) {
  auto cf = std::dynamic_pointer_cast<Mantid::API::CompositeFunction>(fun);
  if (!cf)
    return static_cast<bool>(std::dynamic_pointer_cast<Mantid::API::IFunctionMD>(fun));
  for (size_t i = 0; i < cf->nFunctions(); ++i) {
    bool yes = isFunctionMD(cf->getFunction(i));
    if (yes)
      return true;
  }
  return false;
}
} // namespace

/**
 * Is the function MD?
 */
bool FitDialog::isMD() const {
  auto fun = m_form.function->getFunction();
  return isFunctionMD(fun);
}

/**
 * Get the domain type: Simple, Sequential, or Parallel
 */
int FitDialog::getDomainType() const {
  QString type = m_form.cbDomainType->currentText();
  if (type == "Simple") {
    return 0;
  } else if (type == "Sequential") {
    return 1;
  } else if (type == "Parallel") {
    return 2;
  } else {
    return 0;
  }
}

/// Get the domain type: Simple, Sequential, or Parallel
QString FitDialog::getDomainTypeString() const { return m_form.cbDomainType->currentText(); }

} // namespace MantidQt::CustomDialogs
