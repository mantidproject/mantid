#include "MantidQtCustomInterfaces/IndirectDataReductionTab.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Logger.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

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
  IndirectDataReductionTab::IndirectDataReductionTab(Ui::IndirectDataReduction& uiForm, QObject* parent) : IndirectTab(parent),
      m_uiForm(uiForm)
  {
    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(tabExecutionComplete(bool)));
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
      m_tabStartTime = DateAndTime::getCurrentTime();
      m_tabRunning = true;
      emit updateRunButton(false, "Running...", "Running data reduction...");
      run();
    }
    else
    {
      g_log.warning("Failed to validate indirect tab input!");
    }
  }

  /**
   * Slot used to update the run button when an algorithm that was strted by the Run button complete.
   *
   * @param error Unused
   */
  void IndirectDataReductionTab::tabExecutionComplete(bool error)
  {
    UNUSED_ARG(error);
    if(m_tabRunning)
    {
      m_tabRunning = false;
      emit updateRunButton();
    }
  }

  /**
   * Loads an empty instrument into a workspace (__empty_INST) unless the workspace already exists.
   *
   * If an analyser and reflection are supplied then the corresponding IPF is also loaded.
   *
   * @param instrumentName Name of the instrument to load
   * @param analyser Analyser being used (optional)
   * @param reflection Relection being used (optional)
   * @returns Pointer to instrument workspace
   */
  Mantid::API::MatrixWorkspace_sptr IndirectDataReductionTab::loadInstrumentIfNotExist(std::string instrumentName,
      std::string analyser, std::string reflection)
  {
    IndirectDataReduction* parentIDR = dynamic_cast<IndirectDataReduction*>(m_parentWidget);

    if(parentIDR == NULL)
      throw std::runtime_error("IndirectDataReductionTab must be a child of IndirectDataReduction");

    return parentIDR->loadInstrumentIfNotExist(instrumentName, analyser, reflection);
  }

  /**
   * Gets details for the current instrument configuration defined in Convert To Energy tab.
   *
   * @return Map of information ID to value
   */
  std::map<QString, QString> IndirectDataReductionTab::getInstrumentDetails()
  {
    std::map<QString, QString> instDetails;

    // Get instrument configuration
    std::string instrumentName = m_uiForm.cbInst->currentText().toStdString();
    std::string analyser = m_uiForm.cbAnalyser->currentText().toStdString();
    std::string reflection = m_uiForm.cbReflection->currentText().toStdString();

    instDetails["instrument"] = QString::fromStdString(instrumentName);
    instDetails["analyser"] = QString::fromStdString(analyser);
    instDetails["reflection"] = QString::fromStdString(reflection);

    // List of values to get from IPF
    std::vector<std::string> ipfElements;
    ipfElements.push_back("analysis-type");
    ipfElements.push_back("spectra-min");
    ipfElements.push_back("spectra-max");
    ipfElements.push_back("efixed-val");
    ipfElements.push_back("peak-start");
    ipfElements.push_back("peak-end");
    ipfElements.push_back("back-start");
    ipfElements.push_back("back-end");
    ipfElements.push_back("rebin-default");
    ipfElements.push_back("cm-1-convert-choice");
    ipfElements.push_back("save-ascii-choice");

    // Get the instrument workspace
    MatrixWorkspace_sptr instWorkspace = loadInstrumentIfNotExist(instrumentName, analyser, reflection);

    // In the IRIS IPF there is no fmica component
    if(instrumentName == "IRIS" && analyser == "fmica")
      analyser = "mica";

    // Get the instrument
    auto instrument = instWorkspace->getInstrument();
    if(instrument == NULL)
      return instDetails;

    // Get the analyser component
    auto component = instrument->getComponentByName(analyser);

    // For each parameter we want to get
    for(auto it = ipfElements.begin(); it != ipfElements.end(); ++it)
    {
      try
      {
        std::string key = *it;

        QString value = getInstrumentParameterFrom(instrument, key);

        if(value.isEmpty() && component != NULL)
          QString value = getInstrumentParameterFrom(component, key);

        instDetails[QString::fromStdString(key)] = value;
      }
      // In the case that the parameter does not exist
      catch(Mantid::Kernel::Exception::NotFoundError &nfe)
      {
        UNUSED_ARG(nfe);
        g_log.warning() << "Could not find parameter " << *it << " in instrument " << instrumentName << std::endl;
      }
    }

    return instDetails;
  }

  QString IndirectDataReductionTab::getInstrumentParameterFrom(Mantid::Geometry::IComponent_const_sptr comp, std::string param)
  {
    QString value;

    if(!comp->hasParameter(param))
      return "";

    // Determine it's type and call the corresponding get function
    std::string paramType = comp->getParameterType(param);

    if(paramType == "string")
      value = QString::fromStdString(comp->getStringParameter(param)[0]);

    if(paramType == "double")
      value = QString::number(comp->getNumberParameter(param)[0]);

    return value;
  }

  /**
   * Gets default peak and background ranges for an instrument in time of flight.
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
    // Get any unset parameters
    if(instName.isEmpty())
      instName = m_uiForm.cbInst->currentText();
    if(analyser.isEmpty())
      analyser = m_uiForm.cbAnalyser->currentText();
    if(reflection.isEmpty())
      reflection = m_uiForm.cbReflection->currentText();

    std::map<std::string, double> ranges;

    // Get the instrument
    auto instWs = loadInstrumentIfNotExist(instName.toStdString(), analyser.toStdString(), reflection.toStdString());
    auto inst = instWs->getInstrument();

    // Get the analyser component
    auto comp = inst->getComponentByName(analyser.toStdString());
    if(!comp)
      return ranges;

    // Get the resolution of the analyser
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
