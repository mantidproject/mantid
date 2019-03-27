// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Mantid Headers from the same project
#include "DPDFFitControl.h"
#include "DPDFDisplayControl.h"
#include "DPDFFitOptionsBrowser.h"
#include "DPDFInputDataControl.h"
// Mantid headers from other projects
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/FitOptionsBrowser.h"
#include "MantidQtWidgets/Common/FunctionBrowser.h"
// 3rd party library headers
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSignalMapper>

namespace {
Mantid::Kernel::Logger g_log("DynamicPDF");
}

namespace MantidQt {
namespace CustomInterfaces {
namespace DynamicPDF {

/*               **********************
 *               **  Public Members  **
 *               **********************/

/**
 * @brief Constructor
 */
FitControl::FitControl(QWidget *parent)
    : QWidget(parent), m_functionBrowser{nullptr}, m_fitOptionsBrowser{nullptr},
      m_inputDataControl{nullptr}, m_displayControl{nullptr},
      m_fitRunner(), m_individualFitName{"DPDFIndivFit"}, m_modelEvaluationName{
                                                              "DPDFModelEval"} {
  this->initLayout();
}

/**
 * @brief Destructor
 */
FitControl::~FitControl() {
  // nothing in the body
}

/*               ********************
 *               **  Public Slots  **
 *               ********************/

/**
 * @brief Number of selected slices for fitting. At the moment
 * only single fitting is implemented.
 * @return 1
 */
int FitControl::getNumberOfSpectra() const { return 1; }

/*                *********************
 *                **  Private Slots  **
 *                *********************/

/**
 * @brief update the fit range when user manipulated the rangeSelectorFit
 */
void FitControl::updateFitRangeFromDisplayControl() {
  auto startX = (m_fitOptionsBrowser->getProperty("StartX")).toDouble();
  auto endX = (m_fitOptionsBrowser->getProperty("EndX")).toDouble();
  auto fitRange = m_displayControl->getFitMinMax();
  // prevent "echo" situations. For instance, user changed
  // the fit-range in the property browser, wwhich in turn changed
  // the fit-range in the DisplayCurveFit, which in turn sent a
  //  signal that is being received now by this slot.
  if ((startX != fitRange.first) || (endX != fitRange.second)) {
    m_fitOptionsBrowser->setProperty(QString::fromStdString("StartX"),
                                     QString::number(fitRange.first));
    m_fitOptionsBrowser->setProperty(QString::fromStdString("EndX"),
                                     QString::number(fitRange.second));
  }
}

/**
 * @brief update the boundaries of the fit RangeSelector in the
 * DisplayCurveFit when user changes the corresponding values in the
 * DPDFFitOptionsBrowser
 */
void FitControl::updateFitRangeSelector(const QString &propertyName) {
  auto name = QString::fromStdString("StartX");
  if (propertyName == name) {
    auto startX = (m_fitOptionsBrowser->getProperty(propertyName)).toDouble();
    m_displayControl->setFitMin(startX);
    return;
  }
  name = QString::fromStdString("EndX");
  if (propertyName == name) {
    auto endX = (m_fitOptionsBrowser->getProperty(propertyName)).toDouble();
    m_displayControl->setFitMax(endX);
    return;
  }
}

void FitControl::fit() {
  if (!isSliceSelectedForFitting()) {
    QMessageBox::warning(this, "MantidPlot - Warning", "Select a slice first.");
    return;
  }
  if (!m_functionBrowser->hasFunction()) {
    QMessageBox::warning(this, "MantidPlot - Warning", "Function wasn't set.");
    return;
  }
  auto fittingType = m_fitOptionsBrowser->getCurrentFittingType();
  if (fittingType == MantidWidgets::FitOptionsBrowser::Simultaneous) {
    fitSimultaneous();
  } else if (fittingType == MantidWidgets::FitOptionsBrowser::Sequential) {
    fitSequential();
  } else {
    throw std::logic_error("Unrecognised fitting type");
  }
}

/*
 * @brief Update function browser  with the optimized parameters
 * @param error do nothing if fitting did not complete
 */
void FitControl::finishIndividualFit(bool error) {
  if (error) {
    return;
  }
  g_log.debug() << "FitControl::finishIndividualFit\n";
  Mantid::API::IFunction_sptr fun;
  fun = m_fitRunner->getAlgorithm()->getProperty("Function");
  // prevent the function browser to emit signal after update
  disconnect(m_functionBrowser,
             SIGNAL(parameterChanged(const QString &, const QString &)), this,
             SLOT(slotEvaluateModel(const QString &, const QString &)));
  this->updateFunctionBrowser(fun);
  connect(m_functionBrowser,
          SIGNAL(parameterChanged(const QString &, const QString &)), this,
          SLOT(slotEvaluateModel(const QString &, const QString &)));
  const bool evaluateModel{true};
  this->fitIndividual(evaluateModel);
}

/*
 * @brief Evaluate the model after changes to the function browser.
 * This is just a slot matching the signal from the function browser
 * that calls evaluateModel()
 */
void FitControl::slotEvaluateModel(const QString &, const QString &) {
  const bool evaluateModel{true};
  this->fitIndividual(evaluateModel);
}

/*
 * @brief Emit signal after model evaluation
 */
void FitControl::finishModelEvaluation(bool error) {
  if (error) {
    return;
  }
  emit signalModelEvaluationFinished(
      QString::fromStdString(m_modelEvaluationName + "_Workspace"));
}

/*
 * @brief Load a model from the built-in models in the Mantid settings
 * @param modelName name of the model function
 */
void FitControl::updateFunctionBrowserWithBuiltInModel(
    const QString &modelName) {
  this->updateFunctionBrowser("BuiltInModels", modelName);
}

/*                ***********************
 *                **  Private Members  **
 *                ***********************/

/**
 * @brief Initialize UI form
 */
void FitControl::initLayout() {
  m_uiForm.setupUi(this);
  m_functionBrowser = m_uiForm.functionBrowser;
  m_fitOptionsBrowser = m_uiForm.fitOptionsBrowser;
  this->initBuiltInModels();
  this->initCustomModels();

  // set SIGNAL/SLOTS connections between "internal" objects
  // update the range selector in StartX or EndX has changed in the browser
  connect(m_fitOptionsBrowser, SIGNAL(doublePropertyChanged(QString)), this,
          SLOT(updateFitRangeSelector(QString)));
  // user clicks the Fit push buttom to carry out the fit
  connect(m_uiForm.pushButtonFit, SIGNAL(clicked()), this, SLOT(fit()));
  // update the model evaluation after changes in the function browser
  connect(m_functionBrowser,
          SIGNAL(parameterChanged(const QString &, const QString &)), this,
          SLOT(slotEvaluateModel(const QString &, const QString &)));
}

/**
 * @brief inquire if the slice has already been selected for fitting
 */
bool FitControl::isSliceSelectedForFitting() {
  return m_inputDataControl && m_inputDataControl->isSliceSelectedForFitting();
}

/**
 * @brief Establish connections between objects instantiated in
 * DPDFBackgroundRemover. Thus, this method is called there.
 */
void FitControl::setConnections() {
  // rangeSelectorFit has been changed in the DisplayControl
  connect(m_displayControl, SIGNAL(signalRangeSelectorFitUpdated()), this,
          SLOT(updateFitRangeFromDisplayControl()));
  connect(this, SIGNAL(signalModelEvaluationFinished(const QString &)),
          m_displayControl,
          SLOT(updateModelEvaluationDisplay(const QString &)));
}

/**
 * @brief pass the InputDataControl object for initialization
 */
void FitControl::setInputDataControl(InputDataControl *inputDataControl) {
  m_inputDataControl = inputDataControl;
}

/**
 * @brief pass the DisplayControl object for initialization
 */
void FitControl::setDisplayControl(DisplayControl *displayControl) {
  m_displayControl = displayControl;
}

/**
 * @brief Sequential fitting in the absence of global parameters
 */
void FitControl::fitSequential() {
  int n = this->getNumberOfSpectra();
  if (n == 1) {
    this->fitIndividual();
    return;
  }
  throw std::logic_error("Sequential fitting not yet implemented.");
} // FitControl::fitSequential

/**
 * @brief Simultaneeous fitting
 */
void FitControl::fitSimultaneous() {
  int n = this->getNumberOfSpectra();
  if (n == 1) {
    this->fitIndividual();
    return;
  }
  throw std::logic_error("Simultaneous fitting not yet implemented.");
} // FitControl::fitSimultaneous

/**
 * @brief fitting of a single slice
 * @param isEvaluation carry out model evaluation instead of Fit
 */
void FitControl::fitIndividual(const bool &isEvaluation) {
  try {
    g_log.debug() << "FitControl::fitIndividual\n";
    auto fun = m_functionBrowser->getFunction();
    auto fit = Mantid::API::AlgorithmManager::Instance().create("Fit");
    fit->initialize();
    fit->setProperty("Function", fun);
    fit->setPropertyValue("InputWorkspace",
                          m_inputDataControl->getWorkspaceName());
    auto index = static_cast<int>(m_inputDataControl->getWorkspaceIndex());
    fit->setProperty("WorkspaceIndex", index);
    m_fitOptionsBrowser->copyPropertiesToAlgorithm(*fit);
    m_fitRunner.reset(new API::AlgorithmRunner());
    if (isEvaluation) {
      fit->setPropertyValue("Output", m_modelEvaluationName);
      fit->setProperty("MaxIterations", 0);
      auto range = m_inputDataControl->getCurrentRange();
      fit->setProperty("StartX", range.first);
      fit->setProperty("EndX", range.second);
      connect(m_fitRunner.get(), SIGNAL(algorithmComplete(bool)), this,
              SLOT(finishModelEvaluation(bool)), Qt::QueuedConnection);
    } else {
      fit->setPropertyValue("Output", m_individualFitName);
      connect(m_fitRunner.get(), SIGNAL(algorithmComplete(bool)), this,
              SLOT(finishIndividualFit(bool)), Qt::QueuedConnection);
    }
    m_fitRunner->startAlgorithm(fit);
  } catch (std::exception &e) {
    QString mess(e.what());
    const int maxSize = 500;
    if (mess.size() > maxSize) {
      mess = mess.mid(0, maxSize);
      mess += "...";
    }
    QMessageBox::critical(this, "DynamicPDF - Error",
                          QString("fitIndividual failed:\n\n  %1").arg(mess));
  }
} // FitControl::fitIndividual

/*
 * @brief Update the parameters fo the function browser
 * @param fun A function from which to retrieve the parameters
 */
void FitControl::updateFunctionBrowser(Mantid::API::IFunction_sptr fun) {
  m_functionBrowser->setFunction(fun);
}

/*
 * @brief Load a model from the Mantid settings
 * @param directory either "BuiltInModels" or "CustomModels"
 * @param modelName name of the model function
 */
void FitControl::updateFunctionBrowser(const QString &directory,
                                       const QString &modelName) {
  QSettings settings;
  settings.beginGroup("Mantid/DynamicPDF/" + directory);
  QString function = settings.value(modelName).toString();
  m_functionBrowser->setFunction(function);
}

/**
 * @brief Create menu for the builtInModels. Load the built-in models
 * from the Mantid settings but save them first to the settings if
 * if not found
 */
void FitControl::initBuiltInModels() {
  auto menuBuiltIn = new QMenu(this);
  m_uiForm.pbBuiltIn->setMenu(menuBuiltIn);
  QSettings settings;
  settings.beginGroup("Mantid/DynamicPDF/BuiltInModels");
  QStringList names = settings.childKeys();
  if (names.size() == 0) {
    this->saveBuiltInModels();
  }
  this->loadBuiltInModels(menuBuiltIn);
}

/**
 * @brief Save a few model functions in the Mantid settings.
 * Current models are "Quadratic", "Gaussian plus linear background"
 * and "Quadratic times gaussian, plus linear background".
 */
void FitControl::saveBuiltInModels() {
  QSettings settings;
  settings.beginGroup("Mantid/DynamicPDF/BuiltInModels");
  QMap<QString, QString> models;
  // Quadratic
  models["Quadratic"] = "name=Quadratic,A0=0,A1=0,A2=0";
  // Gaussian plus a linear background
  models["Gaussian+LB"] = "name=Gaussian,Height=0,PeakCentre=0,Sigma=0;name="
                          "LinearBackground,A0=0,A1=0";
  // (Quadratic times Gaussian ) plus linear background
  models["QuadXGauss+LB"] = "(composite=ProductFunction,NumDeriv=false;name="
                            "Quadratic,A0=0,A1=0,A2=0;name=Gaussian,Height=0,"
                            "PeakCentre=0,Sigma=0);name=LinearBackground,A0=0,"
                            "A1=0";
  for (const auto &modelName : models.keys()) {
    settings.setValue(modelName, models[modelName]);
  }
}

/**
 * @brief Load the models from the Mantid settings
 */
void FitControl::loadBuiltInModels(QMenu *menuModels) {
  QSettings settings;
  settings.beginGroup("Mantid/DynamicPDF/BuiltInModels");
  QSignalMapper *mapperModel = new QSignalMapper(this);
  QStringList modelNames = settings.childKeys();
  for (int i = 0; i < modelNames.size(); i++) {
    QAction *actionModel = new QAction(modelNames.at(i), this);
    mapperModel->setMapping(actionModel, modelNames.at(i));
    connect(actionModel, SIGNAL(triggered()), mapperModel, SLOT(map()));
    menuModels->addAction(actionModel);
  }
  connect(mapperModel, SIGNAL(mapped(const QString &)), this,
          SLOT(updateFunctionBrowserWithBuiltInModel(const QString &)));
}
/**
 * @brief Load the models from the Mantid settings
 */
void FitControl::initCustomModels() {
  auto menuCustom = new QMenu(this);
  m_uiForm.pbCustom->setMenu(menuCustom);
  // initialize the action that manage the list of custom models
  QAction *actionSave = new QAction("Save", this);
  QAction *actionCopy = new QAction("Copy", this);
  QAction *actionLoad = new QAction("Load", this);
  QAction *actionDelete = new QAction("Delete", this);
  menuCustom->addAction(actionSave);
  menuCustom->addAction(actionCopy);
  menuCustom->addAction(actionLoad);
  menuCustom->addAction(actionDelete);
}
} // namespace DynamicPDF
} // namespace CustomInterfaces
} // namespace MantidQt
