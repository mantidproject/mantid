#include "MantidQtCustomInterfaces/DynamicPDF/DPDFFitPropertyBrowser.h"

#include <QSettings>

namespace MantidQt {
namespace CustomInterfaces {
namespace DynamicPDF {

/**
 * @brief Constructor
 * @param parent :: The parent widget - must be an ApplicationWindow
 * @param mantidui :: The UI form for MantidPlot
*/
DPDFFitPropertyBrowser::DPDFFitPropertyBrowser(QWidget *parent,
                                               QObject *mantidui)
    : FitPropertyBrowser(parent, mantidui) {}

/**
 * @brief initializes all components, including the fitting properties,
 * the widgets to edit the properties, the action menus, the layout of
 * the property-tree-browser, the root CompositeFunction, and the list
 * of available functions.
 */
void DPDFFitPropertyBrowser::init() {
  QWidget *w = new QWidget(this);
  this->initProperties();
  this->createEditors(w);
  this->updateDecimals();
  this->createModelPropertyGroup(w);
  this->initializeFunctionObserver();
  this->initLayout(w);
}

/**            *******************
 *               Private members
 *             *******************/

/**
 * @brief initializes properties, managers, and QSettings
 * This is the property tree:
 *  Model
 *  |
 *  Data
 *  |__Workspace
 *  |__Workspace Index
 *  |__Fit-Range StartX
 *  |__Fit-Range EndX
 *  |__Eval-Range StartX
 *  |__Eval-Range EndX
 *  |__Output
 *  |
 *  Engine (groupProperty)
 *  |__Minimizer
 *  |__Max Iterations
 *  |__Plot Difference
 */
void DPDFFitPropertyBrowser::initProperties() {

  QSettings settings;
  settings.beginGroup("Mantid/DPDFFitBrowser");

  // Create Model group
  // Model group is created after DPDFFitPropertyBrowser::createEditors()

  // Create Data group
  auto dataGroup = m_groupManager->addProperty("Data");
  // Populate the group with its member properties
  // (Property "Workspace" is initialized in the constructor of the parent
  // class)
  m_workspaceIndex = m_intManager->addProperty("Workspace Index");
  m_startX = addDoubleProperty(QString("Fit-Range StartX"));
  m_endX = addDoubleProperty(QString("Fit-Range EndX"));
  m_eStartX = addDoubleProperty(QString("Eval-Range StartX"));
  m_eEndX = addDoubleProperty(QString("Eval-Range EndX"));
  m_output = m_stringManager->addProperty("Output");
  for (auto prop :
       {m_workspaceIndex, m_startX, m_endX, m_eStartX, m_eEndX, m_output}) {
    dataGroup->addSubProperty(prop);
  }

  // Create Engine group
  auto engineGroup = m_groupManager->addProperty("Engine");
  // Populate the group
  m_minimizer = m_enumManager->addProperty("Minimizer");
  m_minimizers << "Levenberg-Marquardt"
               << "Simplex"
               << "Conjugate gradient (Fletcher-Reeves imp.)"
               << "Conjugate gradient (Polak-Ribiere imp.)"
               << "BFGS";
  m_enumManager->setEnumNames(m_minimizer, m_minimizers);
  m_maxIterations = m_intManager->addProperty("Max Iterations");
  m_intManager->setValue(m_maxIterations,
                         settings.value("Max Iterations", 500).toInt());
  m_plotDiff = m_boolManager->addProperty("Plot Difference");
  bool plotDiff = settings.value("Plot Difference", QVariant(true)).toBool();
  m_boolManager->setValue(m_plotDiff, plotDiff);
  for (auto prop : {m_minimizer, m_maxIterations, m_plotDiff}) {
    engineGroup->addSubProperty(prop);
  }
}

/**
 * @brief Initializes the Model
 * @param w widget parenting the property browser and editor widgets
 */
void DPDFFitPropertyBrowser::createModelPropertyGroup(QWidget *w) {
  auto modelGroup = m_groupManager->addProperty("Model");
  // m_browser was initialized in DPDFFitPropertyBrowser::createEditors()
  m_functionsGroup = m_browser->addProperty(modelGroup);
}

/**
 * @brief Notifies when a new function is registered, e.g.a user-defined
 * function
 */
void DPDFFitPropertyBrowser::initializeFunctionObserver() {
  Mantid::API::FunctionFactory::Instance().notificationCenter.addObserver(
      m_updateObserver);
  connect(this, SIGNAL(functionFactoryUpdateReceived()), this,
          SLOT(populateFunctionNames()));
  Mantid::API::FunctionFactory::Instance().enableNotifications();
}

} // namespace DynamicPDF
} // namespace CustomInterfaces
} // namespace MantidQt
