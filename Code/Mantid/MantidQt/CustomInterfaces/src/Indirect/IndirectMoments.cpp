#include "MantidQtCustomInterfaces/Indirect/IndirectMoments.h"

#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include <QDoubleValidator>
#include <QFileInfo>

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  IndirectMoments::IndirectMoments(IndirectDataReduction * idrUI, QWidget * parent) :
    IndirectDataReductionTab(idrUI, parent)
  {
    m_uiForm.setupUi(parent);

    const unsigned int NUM_DECIMALS = 6;

    // RAW PLOT
    m_plots["MomentsPlot"] = new QwtPlot(m_parentWidget);
    /* m_curves["MomentsPlotCurve"] = new QwtPlotCurve(); */
    m_rangeSelectors["MomentsRangeSelector"] = new MantidWidgets::RangeSelector(m_plots["MomentsPlot"]);
    m_rangeSelectors["MomentsRangeSelector"]->setInfoOnly(false);

    // Initilise plot
    m_plots["MomentsPlot"]->setCanvasBackground(Qt::white);
    m_plots["MomentsPlot"]->setAxisFont(QwtPlot::xBottom, parent->font());
    m_plots["MomentsPlot"]->setAxisFont(QwtPlot::yLeft, parent->font());

    // Add plot to UI
    m_uiForm.plotRaw->addWidget(m_plots["MomentsPlot"]);

    // PROPERTY TREE
    m_propTrees["MomentsPropTree"] = new QtTreePropertyBrowser();
    m_propTrees["MomentsPropTree"]->setFactoryForManager(m_dblManager, m_dblEdFac);
    m_uiForm.properties->addWidget(m_propTrees["MomentsPropTree"]);
    m_properties["EMin"] = m_dblManager->addProperty("EMin");
    m_properties["EMax"] = m_dblManager->addProperty("EMax");

    m_propTrees["MomentsPropTree"]->addProperty(m_properties["EMin"]);
    m_propTrees["MomentsPropTree"]->addProperty(m_properties["EMax"]);

    m_dblManager->setDecimals(m_properties["EMin"], NUM_DECIMALS);
    m_dblManager->setDecimals(m_properties["EMax"], NUM_DECIMALS);

    connect(m_uiForm.dsInput, SIGNAL(dataReady(const QString&)), this, SLOT(handleSampleInputReady(const QString&)));

    connect(m_rangeSelectors["MomentsRangeSelector"], SIGNAL(selectionChangedLazy(double, double)), this, SLOT(rangeChanged(double, double)));
    connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updateProperties(QtProperty*, double)));

    // Update the preview plot when the algorithm completes
    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(momentsAlgComplete(bool)));
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  IndirectMoments::~IndirectMoments()
  {
  }

  void IndirectMoments::setup()
  {
  }

  void IndirectMoments::run()
  {
    QString workspaceName = m_uiForm.dsInput->getCurrentDataName();
    QString outputName = workspaceName.left(workspaceName.length() - 4);
    double scale = m_uiForm.spScale->value();
    double eMin = m_dblManager->value(m_properties["EMin"]);
    double eMax = m_dblManager->value(m_properties["EMax"]);

    bool plot = m_uiForm.ckPlot->isChecked();
    bool verbose = m_uiForm.ckVerbose->isChecked();
    bool save = m_uiForm.ckSave->isChecked();

    std::string outputWorkspaceName = outputName.toStdString() + "_Moments";

    IAlgorithm_sptr momentsAlg = AlgorithmManager::Instance().create("SofQWMoments", -1);
    momentsAlg->initialize();
    momentsAlg->setProperty("Sample", workspaceName.toStdString());
    momentsAlg->setProperty("EnergyMin", eMin);
    momentsAlg->setProperty("EnergyMax", eMax);
    momentsAlg->setProperty("Plot", plot);
    momentsAlg->setProperty("Verbose", verbose);
    momentsAlg->setProperty("Save", save);
    momentsAlg->setProperty("OutputWorkspace", outputWorkspaceName);

    if(m_uiForm.ckScale->isChecked())
      momentsAlg->setProperty("Scale", scale);

    // Set the workspace name for Python script export
    m_pythonExportWsName = outputWorkspaceName + "_M0";

    // Execute algorithm on seperate thread
    runAlgorithm(momentsAlg);
  }

  bool IndirectMoments::validate()
  {
    UserInputValidator uiv;

    uiv.checkDataSelectorIsValid("Sample input", m_uiForm.dsInput);

    QString msg = uiv.generateErrorMessage();
    if (!msg.isEmpty())
    {
      emit showMessageBox(msg);
      return false;
    }

    return true;
  }

  void IndirectMoments::handleSampleInputReady(const QString& filename)
  {
    disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updateProperties(QtProperty*, double)));

    plotMiniPlot(filename, 0, "MomentsPlot", "MomentsPlotCurve");
    std::pair<double,double> range = getCurveRange("MomentsPlotCurve");
    setMiniPlotGuides("MomentsRangeSelector", m_properties["EMin"], m_properties["EMax"], range);
    setPlotRange("MomentsRangeSelector", m_properties["EMin"], m_properties["EMax"], range);

    connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updateProperties(QtProperty*, double)));

    // Update the results preview plot
    updatePreviewPlot();
  }

  /**
   * Updates the property manager when the range selector is moved.
   *
   * @param min :: The new value of the lower guide
   * @param max :: The new value of the upper guide
   */
  void IndirectMoments::rangeChanged(double min, double max)
  {
    m_dblManager->setValue(m_properties["EMin"], min);
    m_dblManager->setValue(m_properties["EMax"], max);
  }

  /**
   * Handles when properties in the property manager are updated.
   *
   * Performs validation and uodated preview plot.
   *
   * @param prop :: The property being updated
   * @param val :: The new value for the property
   */
  void IndirectMoments::updateProperties(QtProperty* prop, double val)
  {
    if(prop == m_properties["EMin"])
    {
      double emax = m_dblManager->value(m_properties["EMax"]);
      if(val >  emax)
      {
        m_dblManager->setValue(prop, emax);
      }
      else
      {
        m_rangeSelectors["MomentsRangeSelector"]->setMinimum(val);
      }
    }
    else if (prop == m_properties["EMax"])
    {
      double emin = m_dblManager->value(m_properties["EMin"]);
      if(emin > val)
      {
        m_dblManager->setValue(prop, emin);
      }
      else
      {
        m_rangeSelectors["MomentsRangeSelector"]->setMaximum(val);
      }
    }

    updatePreviewPlot();
  }

  /**
   * Runs the moments algorithm with preview properties.
   */
  void IndirectMoments::updatePreviewPlot(QString workspaceName)
  {
    if(workspaceName.isEmpty())
      workspaceName = m_uiForm.dsInput->getCurrentDataName();

    QString outputName = workspaceName.left(workspaceName.length() - 4);
    double scale = m_uiForm.spScale->value();
    double eMin = m_dblManager->value(m_properties["EMin"]);
    double eMax = m_dblManager->value(m_properties["EMax"]);

    bool verbose = m_uiForm.ckVerbose->isChecked();

    std::string outputWorkspaceName = outputName.toStdString() + "_Moments";

    IAlgorithm_sptr momentsAlg = AlgorithmManager::Instance().create("SofQWMoments");
    momentsAlg->initialize();
    momentsAlg->setProperty("Sample", workspaceName.toStdString());
    momentsAlg->setProperty("EnergyMin", eMin);
    momentsAlg->setProperty("EnergyMax", eMax);
    momentsAlg->setProperty("Plot", false);
    momentsAlg->setProperty("Verbose", verbose);
    momentsAlg->setProperty("Save", false);
    momentsAlg->setProperty("OutputWorkspace", outputWorkspaceName);

    if(m_uiForm.ckScale->isChecked())
      momentsAlg->setProperty("Scale", scale);

    // Make sure there are no other algorithms in the queue.
    // It seems to be possible to have the selctionChangedLazy signal fire multiple times
    // if the renage selector is moved in a certain way.
    if(m_batchAlgoRunner->queueLength() == 0)
      runAlgorithm(momentsAlg);
  }

  /**
   * Handles plotting the preview plot when the algorithm finishes.
   *
   * @param error True if the algorithm exited due to error, false otherwise
   */
  void IndirectMoments::momentsAlgComplete(bool error)
  {
    if(error)
      return;

    QString workspaceName = m_uiForm.dsInput->getCurrentDataName();
    QString outputName = workspaceName.left(workspaceName.length() - 4);
    std::string outputWorkspaceName = outputName.toStdString() + "_Moments";

    WorkspaceGroup_sptr resultWsGroup = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputWorkspaceName);
    std::vector<std::string> resultWsNames = resultWsGroup->getNames();

    if(resultWsNames.size() < 4)
      return;

    // Plot each spectrum
    m_uiForm.ppMomentsPreview->clear();
    m_uiForm.ppMomentsPreview->addSpectrum(QString::fromStdString(resultWsNames[0]), 0, Qt::green);
    m_uiForm.ppMomentsPreview->addSpectrum(QString::fromStdString(resultWsNames[2]), 0, Qt::black);
    m_uiForm.ppMomentsPreview->addSpectrum(QString::fromStdString(resultWsNames[3]), 0, Qt::red);
    m_uiForm.ppMomentsPreview->resizeX();
  }

} // namespace CustomInterfaces
} // namespace Mantid
