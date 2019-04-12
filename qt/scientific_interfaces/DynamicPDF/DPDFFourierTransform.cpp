// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Mantid Headers from the same project
#include "DPDFFourierTransform.h"
#include "DPDFFitControl.h"
#include "DPDFInputDataControl.h"
// Mantid headers from other projects
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"
#include "MantidQtWidgets/LegacyQwt/RangeSelector.h"
// 3rd party library headers
#include <QMessageBox>

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
FourierTransform::FourierTransform(QWidget *parent)
    : QWidget(parent), m_inputDataControl{nullptr},
      m_propertyTree(new QtTreePropertyBrowser()), m_properties(),
      m_decimals(6), m_residualsName{"DPDFResiduals"},
      m_fourierName{"DPDFFourierTransform"}, m_colors(),
      m_doubleManager(new QtDoublePropertyManager(this)),
      m_boolManager(new QtBoolPropertyManager(this)),
      m_enumManager(new QtEnumPropertyManager(this)),
      m_groupManager(new QtGroupPropertyManager(this)), m_algorithmRunner() {
  this->initLayout();
}

/**
 * @brief Destructor
 */
FourierTransform::~FourierTransform() { delete m_propertyTree; }

/*                *********************
 *                **  Private Slots  **
 *                *********************/

/**
 * @brief reset actions after user selects a new slice for fitting
 */
void FourierTransform::resetAfterSliceSelected() {
  // clear previewplot
  m_uiForm.previewPlotFourier->clear();
  // remove residuals and fourier workspaces
  if (Mantid::API::AnalysisDataService::Instance().doesExist(m_residualsName)) {
    Mantid::API::AnalysisDataService::Instance().remove(m_residualsName);
  }
  if (Mantid::API::AnalysisDataService::Instance().doesExist(m_fourierName)) {
    Mantid::API::AnalysisDataService::Instance().remove(m_fourierName);
  }
  // update [Qmin, Qmax] range but do not emit any signal
  disconnect(m_doubleManager, SIGNAL(propertyChanged(QtProperty *)), this,
             SLOT(transformAfterPropertyChanged(QtProperty *)));
  auto range = m_inputDataControl->getCurrentRange();
  m_doubleManager->setValue(m_properties["Qmin"], range.first);
  m_doubleManager->setValue(m_properties["Qmax"], range.second);
  connect(m_doubleManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(transformAfterPropertyChanged(QtProperty *)));
}

/**
 * @brief Extract the histogram corresponding to the residuals
 * of the model evaluation.
 * @param modelWorkspaceName name of the workspace containing
 *  data, model evaluation, and residuals
 */
void FourierTransform::extractResidualsHistogram(
    const QString &modelWorkspaceName) {
  try {

    auto modelWorkspace = Mantid::API::AnalysisDataService::Instance()
                              .retrieveWS<Mantid::API::MatrixWorkspace>(
                                  modelWorkspaceName.toStdString());
    if (!modelWorkspace) {
      g_log.debug() << "Empty modelWorkspace\n";
    }
    // use modelWorkspace as template for the residuals workspace
    auto residualsWorkspace =
        Mantid::API::WorkspaceFactory::Instance().create(modelWorkspace, 1);
    residualsWorkspace->setSharedX(0, modelWorkspace->sharedX(0));
    // residuals is the third spectrum
    residualsWorkspace->setSharedY(0, modelWorkspace->sharedY(2));
    // errors are coming from experiment
    residualsWorkspace->setSharedE(0, modelWorkspace->sharedE(0));
    Mantid::API::AnalysisDataService::Instance().addOrReplace(
        m_residualsName, residualsWorkspace);
  } catch (std::exception &e) {
    QString mess(e.what());
    const int maxSize = 500;
    if (mess.size() > maxSize) {
      mess = mess.mid(0, maxSize);
      mess += "...";
    }
    QMessageBox::critical(
        this, "DynamicPDF - Error",
        QString("extractModelHistogram failed:\n\n  %1").arg(mess));
  }
  emit signalExtractResidualsHistogramFinished();
}

/**
 * @brief carry out the fourier transform with the
 * PDFFourierTransform algorithm
 */
void FourierTransform::transform() {
  try {
    if (!Mantid::API::AnalysisDataService::Instance().doesExist(
            m_residualsName)) {
      throw std::runtime_error("No residuals found from any model evaluation");
    }
    // set up the PDFFourierTransform algorithm
    auto fourier =
        Mantid::API::AlgorithmManager::Instance().create("PDFFourierTransform");
    fourier->initialize();
    fourier->setPropertyValue("InputWorkspace", m_residualsName);
    fourier->setPropertyValue("OutputWorkspace", m_fourierName);
    // pass reciprocal space properties
    auto names = m_enumManager->enumNames(m_properties["InputSofQType"]);
    auto index = m_enumManager->value(m_properties["InputSofQType"]);
    fourier->setProperty("InputSofQType", names[index].toStdString());
    auto qmin = m_doubleManager->value(m_properties["Qmin"]);
    fourier->setProperty("Qmin", qmin);
    auto qmax = m_doubleManager->value(m_properties["Qmax"]);
    fourier->setProperty("Qmax", qmax);
    // pass real space properties
    names = m_enumManager->enumNames(m_properties["PDFType"]);
    index = m_enumManager->value(m_properties["PDFType"]);
    fourier->setProperty("PDFType", names[index].toStdString());
    auto deltaR = m_doubleManager->value(m_properties["DeltaR"]);
    fourier->setProperty("DeltaR", deltaR);
    auto rmax = m_doubleManager->value(m_properties["Rmax"]);
    fourier->setProperty("Rmax", rmax);
    auto rho0 = m_doubleManager->value(m_properties["rho0"]);
    fourier->setProperty("rho0", rho0);
    // Asynchronous execution of the algorithm, so that we can keep on
    // working with the interface
    m_algorithmRunner.reset(new API::AlgorithmRunner());
    connect(m_algorithmRunner.get(), SIGNAL(algorithmComplete(bool)), this,
            SLOT(finishTransform(bool)), Qt::QueuedConnection);
    m_algorithmRunner->startAlgorithm(fourier);
  } catch (std::exception &e) {
    QString mess(e.what());
    const int maxSize = 500;
    if (mess.size() > maxSize) {
      mess = mess.mid(0, maxSize);
      mess += "...";
    }
    QMessageBox::critical(
        this, "DynamicPDF - Error",
        QString("FourierTransform::transform failed:\n\n  %1").arg(mess));
  }
}

/**
 * @brief Just signal that the residuals extraction finished
 */
void FourierTransform::finishTransform(bool error) {
  if (error) {
    return;
  }
  this->updatePlot();
}

/**
 * @brief rely to transform()
 */
void FourierTransform::transformAfterPropertyChanged(QtProperty *property) {
  Q_UNUSED(property);
  this->transform();
}

/**
 * @brief remove all plots from the Fourier transform display
 */
void FourierTransform::clearFourierPlot() {
  m_uiForm.previewPlotFourier->clear();
}

/*                ***********************
 *                **  Private Members  **
 *                ***********************/

/*
 * @brief Instantiate all widgets components
 */
void FourierTransform::initLayout() {
  m_uiForm.setupUi(this);
  this->setupPlotDisplay();
  this->createPropertyTree();
  this->setDefaultPropertyValues();
}

/*
 * @brief Instantiate the property tree with the properties
 * of algorithm PDFFourierTransform
 */
void FourierTransform::createPropertyTree() {

  // create editor factories
  DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory(this);
  QtCheckBoxFactory *checkBoxFactory = new QtCheckBoxFactory(this);
  QtEnumEditorFactory *comboBoxFactory = new QtEnumEditorFactory(this);

  // assign factories to property managers
  m_propertyTree->setFactoryForManager(m_doubleManager, doubleEditorFactory);
  m_propertyTree->setFactoryForManager(m_boolManager, checkBoxFactory);
  m_propertyTree->setFactoryForManager(m_enumManager, comboBoxFactory);

  // properties for "Reciprocal Space" group of algorithm PDFFourierTransform
  m_properties["Reciprocal Space"] =
      m_groupManager->addProperty("Reciprocal Space");
  // insert type of structure factor
  QStringList sOfQTypes;
  sOfQTypes << "S(Q)-1"
            << "S(Q)"
            << "Q[S(Q)-1]";
  m_properties["InputSofQType"] = m_enumManager->addProperty("InputSofQType");
  m_enumManager->setEnumNames(m_properties["InputSofQType"], sOfQTypes);
  m_properties["Reciprocal Space"]->addSubProperty(
      m_properties["InputSofQType"]);
  // insert Qmin property
  m_properties["Qmin"] = m_doubleManager->addProperty("Qmin");
  m_doubleManager->setDecimals(m_properties["Qmin"], m_decimals);
  m_doubleManager->setMinimum(m_properties["Qmin"], 0.0);
  m_properties["Reciprocal Space"]->addSubProperty(m_properties["Qmin"]);
  // insert Qmax property
  m_properties["Qmax"] = m_doubleManager->addProperty("Qmax");
  m_doubleManager->setDecimals(m_properties["Qmax"], m_decimals);
  m_doubleManager->setMinimum(m_properties["Qmax"], 0.0);
  m_properties["Reciprocal Space"]->addSubProperty(m_properties["Qmax"]);
  // insert Filter property
  m_properties["Filter"] = m_boolManager->addProperty("Filter");
  m_properties["Reciprocal Space"]->addSubProperty(m_properties["Filter"]);

  // properties for "Real Space" group of algorithm PDFFourierTransform
  m_properties["Real Space"] = m_groupManager->addProperty("Real Space");
  // insert type of PDF
  QStringList pdfTypes;
  pdfTypes << "G(r)"
           << "g(r)"
           << "RDF(r)";
  m_properties["PDFType"] = m_enumManager->addProperty("PDFType");
  m_enumManager->setEnumNames(m_properties["PDFType"], pdfTypes);
  m_properties["Real Space"]->addSubProperty(m_properties["PDFType"]);
  // insert DeltaR property
  m_properties["DeltaR"] = m_doubleManager->addProperty("DeltaR");
  m_doubleManager->setDecimals(m_properties["DeltaR"], m_decimals);
  m_properties["Real Space"]->addSubProperty(m_properties["DeltaR"]);
  // insert Rmax property
  m_properties["Rmax"] = m_doubleManager->addProperty("Rmax");
  m_doubleManager->setDecimals(m_properties["Rmax"], m_decimals);
  m_properties["Real Space"]->addSubProperty(m_properties["Rmax"]);
  // insert rho0 property
  m_properties["rho0"] = m_doubleManager->addProperty("rho0");
  m_doubleManager->setDecimals(m_properties["rho0"], m_decimals);
  m_properties["Real Space"]->addSubProperty(m_properties["rho0"]);

  // insert properties in the tree
  m_propertyTree->addProperty(m_properties["Reciprocal Space"]);
  m_propertyTree->addProperty(m_properties["Real Space"]);

  // inform of the meaning of certain properties
  m_properties["Qmin"]->setToolTip("must be positive");
  m_properties["Qmax"]->setToolTip("must be positive");
  m_properties["Filter"]->setToolTip("apply Lorch function filter");
  m_properties["DeltaR"]->setToolTip("binning in real space");
  m_properties["Rmax"]->setToolTip("must be positive");
  m_properties["rho0"]->setToolTip("average number density");

  // insert the widget tree in the UI form
  m_uiForm.verticalLayoutProperties->addWidget(m_propertyTree);
  m_uiForm.verticalLayoutProperties->setContentsMargins(0, 0, 0, 0);
}

/*
 * @brief Establish connections between objects instantiated in
 * DPDFBackgroundRemover. Thus, this method is called there.
 * Instantiate the property tree with the properties of
 * algorithm PDFFourierTransform
 */
void FourierTransform::setConnections() {
  connect(m_inputDataControl, SIGNAL(signalSliceForFittingUpdated()), this,
          SLOT(resetAfterSliceSelected()));
  connect(m_fitControl, SIGNAL(signalModelEvaluationFinished(const QString &)),
          this, SLOT(extractResidualsHistogram(const QString &)));
  connect(this, SIGNAL(signalExtractResidualsHistogramFinished()), this,
          SLOT(transform()));
  connect(m_doubleManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(transformAfterPropertyChanged(QtProperty *)));
}

/*
 * @brief Insert a dashed line at Y=0 and show legends
 */
void FourierTransform::setupPlotDisplay() {
  m_uiForm.previewPlotFourier->showLegend(true);
  if (m_uiForm.previewPlotFourier->hasRangeSelector(QString("zeroLine"))) {
    return; // do nothing
  }
  auto zeroLine = m_uiForm.previewPlotFourier->addRangeSelector(
      QString("zeroLine"), MantidQt::MantidWidgets::RangeSelector::YSINGLE);
  zeroLine->setColour(QColor(Qt::darkGreen));
  zeroLine->setMinimum(0.0);
}

/**
 * @brief pass the InputDataControl object for initialization
 */
void FourierTransform::setInputDataControl(InputDataControl *inputDataControl) {
  m_inputDataControl = inputDataControl;
}

/**
 * @brief pass the FitControl object for initialization
 */
void FourierTransform::setFitControl(FitControl *fitControl) {
  m_fitControl = fitControl;
}

/**
 * @brief Set some sensible values for certain properties
 */
void FourierTransform::setDefaultPropertyValues() {
  m_doubleManager->setValue(m_properties["DeltaR"], 0.01);
  m_doubleManager->setValue(m_properties["Rmax"], 10.0);
  // initialize the colors for plotting each transform
  m_colors["G(r)"] = QColor(Qt::black);
  m_colors["g(r)"] = QColor(Qt::blue);
  m_colors["RDF(r)"] = QColor(Qt::red);
}

/**
 * @brief Plot the new Fourier transform
 */
void FourierTransform::updatePlot() {
  auto plotter = m_uiForm.previewPlotFourier;
  auto names = m_enumManager->enumNames(m_properties["PDFType"]);
  auto i = m_enumManager->value(m_properties["PDFType"]);
  auto name = names[i];
  if (plotter->hasCurve(name)) {
    plotter->removeSpectrum(name);
  }
  plotter->addSpectrum(name, QString::fromStdString(m_fourierName), 0,
                       m_colors[name]);
}
} // namespace DynamicPDF
} // namespace CustomInterfaces
} // namespace MantidQt
