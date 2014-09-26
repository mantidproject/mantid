#include "MantidQtCustomInterfaces/IndirectDataReductionTab.h"

#include "MantidKernel/Logger.h"

using namespace Mantid::API;

namespace
{
  Mantid::Kernel::Logger g_log("IndirectDataReductionTab");
}

namespace MantidQt
{
namespace CustomInterfaces
{
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  IndirectDataReductionTab::IndirectDataReductionTab(Ui::IndirectDataReduction& uiForm, QObject* parent) : QObject(parent),
      m_plots(), m_curves(), m_rangeSelectors(),
      m_properties(),
      m_dblManager(new QtDoublePropertyManager()), m_blnManager(new QtBoolPropertyManager()), m_grpManager(new QtGroupPropertyManager()),
      m_dblEdFac(new DoubleEditorFactory()),
      m_uiForm(uiForm)
  {
    m_parentWidget = dynamic_cast<QWidget *>(parent);

    m_batchAlgoRunner = new MantidQt::API::BatchAlgorithmRunner(m_parentWidget);
    m_valInt = new QIntValidator(m_parentWidget);
    m_valDbl = new QDoubleValidator(m_parentWidget);
    m_valPosDbl = new QDoubleValidator(m_parentWidget);

    const double tolerance = 0.00001;
    m_valPosDbl->setBottom(tolerance);

    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmFinished(bool)));
    connect(&m_pythonRunner, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  IndirectDataReductionTab::~IndirectDataReductionTab()
  {
  }
  
  void IndirectDataReductionTab::runTab()
  {
    if(validate())
    {
      run();
    }
    else
    {
      g_log.warning("Failed to validate indirect tab input!");
    }
  }

  void IndirectDataReductionTab::setupTab()
  {
    setup();
  }

  void IndirectDataReductionTab::validateTab()
  {
    validate();
  }

  /**
   * Run the load algorithm with the supplied filename and spectrum range
   * 
   * @param filename :: The name of the file to load
   * @param outputName :: The name of the output workspace
   * @param specMin :: Lower spectra bound
   * @param specMax :: Upper spectra bound
   * @return If the algorithm was successful
   */
  bool IndirectDataReductionTab::loadFile(const QString& filename, const QString& outputName,
      const int specMin, const int specMax)
  {
    Algorithm_sptr load = AlgorithmManager::Instance().createUnmanaged("Load", -1);
    load->initialize();

    load->setProperty("Filename", filename.toStdString());
    load->setProperty("OutputWorkspace", outputName.toStdString());

    if(specMin != -1)
      load->setProperty("SpectrumMin", specMin);

    if(specMax != -1)
      load->setProperty("SpectrumMax", specMax);

    load->execute();
    
    //If reloading fails we're out of options
    return load->isExecuted();
  }

  /**
   * Gets details for the current indtrument configuration defined in Convert To Energy tab
   *
   * @return :: Map of information ID to value
   */
  std::map<QString, QString> IndirectDataReductionTab::getInstrumentDetails()
  {
    std::map<QString, QString> instDetails;
    
    QString pyInput =
      "from IndirectEnergyConversion import getReflectionDetails\n"
      "instrument = '" + m_uiForm.cbInst->currentText() + "'\n"
      "analyser = '" + m_uiForm.cbAnalyser->currentText() + "'\n"
      "reflection = '" + m_uiForm.cbReflection->currentText() + "'\n"
      "print getReflectionDetails(instrument, analyser, reflection)\n";

    QString pyOutput = m_pythonRunner.runPythonCode(pyInput).trimmed();

    QStringList values = pyOutput.split("\n", QString::SkipEmptyParts);

    if(values.count() > 3)
    {
      instDetails["AnalysisType"] = values[0];
      instDetails["SpectraMin"] = values[1];
      instDetails["SpectraMax"] = values[2];

      if(values.count() >= 8)
      {
        instDetails["EFixed"] = values[3];
        instDetails["PeakMin"] = values[4];
        instDetails["PeakMax"] = values[5];
        instDetails["BackMin"] = values[6];
        instDetails["BackMax"] = values[7];
      }

      if(values.count() >= 9)
      {
        instDetails["RebinString"] = values[8];
      }
    }

    return instDetails;
  }

  /**
   * Gets the range of the curve plotted in the mini plot
   *
   * @param curveID :: The string index of the curve in the m_curves map
   * @return A pair containing the maximum and minimum points of the curve
   */
  std::pair<double,double> IndirectDataReductionTab::getCurveRange(const QString& curveID)
  {
    size_t npts = m_curves[curveID]->data().size();

    if( npts < 2 )
      throw std::invalid_argument("Too few points on data curve to determine range.");

    return std::make_pair(m_curves[curveID]->data().x(0), m_curves[curveID]->data().x(npts-1));
  }

  /**
   * Set the range of an axis on a miniplot
   *
   * @param plotID :: Index of plot in m_plots map
   * @param axis :: ID of axis to set range of
   * @param range :: Pair of double values specifying range
   */
  void IndirectDataReductionTab::setAxisRange(const QString& plotID, QwtPlot::Axis axis,
      std::pair<double, double> range)
  {
    m_plots[plotID]->setAxisScale(axis, range.first, range.second);
  }

  /**
   * Sets the X axis of a plot to match the range of x values on a curve
   *
   * @param plotID :: Index of plot in m_plots map
   * @param curveID :: Index of curve in m_curves map
   */
  void IndirectDataReductionTab::setXAxisToCurve(const QString& plotID, const QString& curveID)
  {
    auto range = getCurveRange(curveID);
    setAxisRange(plotID, QwtPlot::xBottom, range);
  }

  /**
   * Plot a workspace to the miniplot given a workspace name and
   * a specturm index.
   *
   * This method uses the analysis data service to retrieve the workspace.
   * 
   * @param workspace :: The name of the workspace
   * @param index :: The spectrum index of the workspace
   * @param plotID :: String index of the plot in the m_plots map
   * @param curveID :: String index of the curve in the m_curves map, defaults to plot ID
   */
  void IndirectDataReductionTab::plotMiniPlot(const QString& workspace, size_t index,
      const QString& plotID, const QString& curveID)
  {
    using namespace Mantid::API;
    auto ws = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(workspace.toStdString());
    plotMiniPlot(ws, index, plotID, curveID);
  }

  /**
   * Replot a given mini plot
   *
   * @param plotID :: ID of plot in m_plots map
   */
  void IndirectDataReductionTab::replot(const QString& plotID)
  {
    m_plots[plotID]->replot();
  }

  /**
   * Plot a workspace to the miniplot given a workspace pointer and
   * a specturm index.
   * 
   * @param workspace :: Pointer to the workspace
   * @param wsIndex :: The spectrum index of the workspace
   * @param plotID :: String index of the plot in the m_plots map
   * @param curveID :: String index of the curve in the m_curves map, defaults to plot ID
   */
  void IndirectDataReductionTab::plotMiniPlot(const Mantid::API::MatrixWorkspace_const_sptr & workspace, size_t wsIndex,
      const QString& plotID, const QString& curveID)
  {
    using Mantid::MantidVec;

    QString cID = curveID;
    if(cID == "")
      cID = plotID;

    //check if we can plot
    if( wsIndex >= workspace->getNumberHistograms() || workspace->readX(0).size() < 2 )
      return;

    QwtWorkspaceSpectrumData wsData(*workspace, static_cast<int>(wsIndex), false);

    if ( m_curves[cID] != NULL )
    {
      m_curves[cID]->attach(0);
      delete m_curves[cID];
      m_curves[cID] = NULL;
    }

    size_t nhist = workspace->getNumberHistograms();
    if ( wsIndex >= nhist )
    {
      emit showMessageBox("Error: Workspace index out of range.");
    }
    else
    {
      m_curves[cID] = new QwtPlotCurve();
      m_curves[cID]->setData(wsData);
      m_curves[cID]->attach(m_plots[plotID]);

      m_plots[plotID]->replot();
    }
  }

  /**
   * Sets the edge bounds of plot to prevent the user inputting invalid values
   * Also sets limits for range selector movement
   * 
   * @param rsID :: The string index of the range selector in the map m_rangeSelectors
   * @param min :: The lower bound property in the property browser
   * @param max :: The upper bound property in the property browser
   * @param bounds :: The upper and lower bounds to be set
   */
  void IndirectDataReductionTab::setPlotRange(const QString& rsID, QtProperty* min, QtProperty* max,
      const std::pair<double, double>& bounds)
  {
    m_dblManager->setMinimum(min, bounds.first);
    m_dblManager->setMaximum(min, bounds.second);
    m_dblManager->setMinimum(max, bounds.first);
    m_dblManager->setMaximum(max, bounds.second);
    m_rangeSelectors[rsID]->setRange(bounds.first, bounds.second);
  }

  /**
   * Set the position of the guides on the mini plot
   * 
   * @param rsID :: The string index of the range selector in the map m_rangeSelectors
   * @param lower :: The lower bound property in the property browser
   * @param upper :: The upper bound property in the property browser
   * @param bounds :: The upper and lower bounds to be set
   */
  void IndirectDataReductionTab::setMiniPlotGuides(const QString& rsID, QtProperty* lower, QtProperty* upper,
      const std::pair<double, double>& bounds)
  {
    m_dblManager->setValue(lower, bounds.first);
    m_dblManager->setValue(upper, bounds.second);
    m_rangeSelectors[rsID]->setMinimum(bounds.first);
    m_rangeSelectors[rsID]->setMaximum(bounds.second);
  }

  /**
   * Runs an algorithm async
   *
   * @param algorithm :: The algorithm to be run
   */
  void IndirectDataReductionTab::runAlgorithm(const Mantid::API::IAlgorithm_sptr algorithm)
  {
    algorithm->setRethrows(true);

    // There should never really be unexecuted algorithms in the queue, but it is worth warning in case of possible weirdness
    size_t batchQueueLength = m_batchAlgoRunner->queueLength();
    if(batchQueueLength > 0)
      g_log.warning() << "Batch queue already contains " << batchQueueLength << " algorithms!" << std::endl;

    m_batchAlgoRunner->addAlgorithm(algorithm);
    m_batchAlgoRunner->executeBatchAsync();
  }

  /**
   * Handles getting the results of an algorithm running async
   *
   * @param error :: True if execution failed, false otherwise
   */
  void IndirectDataReductionTab::algorithmFinished(bool error)
  {
    if(error)
    {
      emit showMessageBox("Error running algorithm. \nSee results log for details.");
    }
  }

  /**
   * Gets default peak and background ranges for an instrument in both time of flight and energy.
   *
   * @param instName Name of instrument
   * @param analyser Analyser component
   * @param reflection Reflection used
   *
   * @returns A map of range ID to value
   */
  std::map<std::string, double> IndirectDataReductionTab::getRangesFromInstrument(
      QString instName, QString analyser, QString reflection)
  {
    if(instName.isEmpty())
      instName = m_uiForm.cbInst->currentText();
    if(analyser.isEmpty())
      analyser = m_uiForm.cbAnalyser->currentText();
    if(reflection.isEmpty())
      reflection = m_uiForm.cbReflection->currentText();

    std::map<std::string, double> ranges;

    //TODO
    auto inst = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("__empty_" + instName.toStdString())->getInstrument();

    auto comp = inst->getComponentByName(analyser.toStdString());
    if(!comp)
      return ranges;

    auto resParams = comp->getNumberParameter("resolution", true);
    if(resParams.size() < 1)
      return ranges;

    double resolution = resParams[0];

    std::vector<double> x;
    x.push_back(-6 * resolution);
    x.push_back(-5 * resolution);
    x.push_back(-2 * resolution);
    x.push_back(0);
    x.push_back(2 * resolution);
    std::vector<double> y;
    y.push_back(1);
    y.push_back(2);
    y.push_back(3);
    y.push_back(4);
    std::vector<double> e(4, 0);

    IAlgorithm_sptr createWsAlg = AlgorithmManager::Instance().create("CreateWorkspace");
    createWsAlg->initialize();
    createWsAlg->setProperty("OutputWorkspace", "__energy");
    createWsAlg->setProperty("DataX", x);
    createWsAlg->setProperty("DataY", y);
    createWsAlg->setProperty("DataE", e);
    createWsAlg->setProperty("Nspec", 1);
    createWsAlg->setProperty("UnitX", "DeltaE");
    createWsAlg->execute();

    IAlgorithm_sptr convertHistAlg = AlgorithmManager::Instance().create("ConvertToHistogram");
    convertHistAlg->initialize();
    convertHistAlg->setProperty("InputWorkspace", "__energy");
    convertHistAlg->setProperty("OutputWorkspace", "__energy");
    convertHistAlg->execute();

    IAlgorithm_sptr loadInstAlg = AlgorithmManager::Instance().create("LoadInstrument");
    loadInstAlg->initialize();
    loadInstAlg->setProperty("Workspace", "__energy");
    loadInstAlg->setProperty("InstrumentName", instName.toStdString());
    loadInstAlg->execute();

    QString ipfFilename = instName + "_" + analyser + "_" + reflection + "_Parameters.xml";

    IAlgorithm_sptr loadParamAlg = AlgorithmManager::Instance().create("LoadParameterFile");
    loadParamAlg->initialize();
    loadParamAlg->setProperty("Workspace", "__energy");
    loadParamAlg->setProperty("Filename", ipfFilename.toStdString());
    loadParamAlg->execute();

    auto energyWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("__energy");

    std::vector<double> energyData = energyWs->readX(0);
    ranges["peak-start-energy"] = energyData[2];
    ranges["peak-end-energy"] = energyData[4];
    ranges["back-start-energy"] = energyData[0];
    ranges["back-end-energy"] = energyData[1];

    double efixed = energyWs->getInstrument()->getNumberParameter("efixed-val")[0];

    auto spectrum = energyWs->getSpectrum(0);
    spectrum->setSpectrumNo(3);
    spectrum->clearDetectorIDs();
    spectrum->addDetectorID(3);

    IAlgorithm_sptr convUnitsAlg = AlgorithmManager::Instance().create("ConvertUnits");
    convUnitsAlg->initialize();
    convUnitsAlg->setProperty("InputWorkspace", "__energy");
    convUnitsAlg->setProperty("OutputWorkspace", "__tof");
    convUnitsAlg->setProperty("Target", "TOF");
    convUnitsAlg->setProperty("EMode", "Indirect");
    convUnitsAlg->setProperty("EFixed", efixed);
    convUnitsAlg->execute();

    auto tofWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("__tof");

    std::vector<double> tofData = tofWs->readX(0);
    ranges["peak-start-tof"] = tofData[0];
    ranges["peak-end-tof"] = tofData[2];
    ranges["back-start-tof"] = tofData[3];
    ranges["back-end-tof"] = tofData[4];

    return ranges;
  }

} // namespace CustomInterfaces
} // namespace Mantid
