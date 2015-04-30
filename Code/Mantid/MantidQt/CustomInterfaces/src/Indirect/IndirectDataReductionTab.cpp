#include "MantidQtCustomInterfaces/Indirect/IndirectDataReductionTab.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Logger.h"
#include "MantidQtCustomInterfaces/Indirect/IndirectDataReduction.h"

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
  IndirectDataReductionTab::IndirectDataReductionTab(IndirectDataReduction *idrUI, QObject *parent) :
    IndirectTab(parent), m_idrUI(idrUI)
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
    return m_idrUI->loadInstrumentIfNotExist(instrumentName, analyser, reflection);
  }

  /**
   * Gets details for the current instrument configuration defined in Convert To Energy tab.
   *
   * @return Map of information ID to value
   */
  std::map<QString, QString> IndirectDataReductionTab::getInstrumentDetails()
  {
    return m_idrUI->getInstrumentDetails();
  }

  /**
   * Returns a pointer to the instrument configuration widget common to all tabs.
   *
   * @return Instrument config widget
   */
  MantidWidgets::IndirectInstrumentConfig *IndirectDataReductionTab::getInstrumentConfiguration()
  {
    return m_idrUI->m_uiForm.iicInstrumentConfiguration;
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
      instName = getInstrumentConfiguration()->getInstrumentName();
    if(analyser.isEmpty())
      analyser = getInstrumentConfiguration()->getAnalyserName();
    if(reflection.isEmpty())
      reflection = getInstrumentConfiguration()->getReflectionName();

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
    createWsAlg->setChild(true);
    createWsAlg->initialize();
    createWsAlg->setProperty("OutputWorkspace", "__energy");
    createWsAlg->setProperty("DataX", x);
    createWsAlg->setProperty("DataY", y);
    createWsAlg->setProperty("DataE", e);
    createWsAlg->setProperty("Nspec", 1);
    createWsAlg->setProperty("UnitX", "DeltaE");
    createWsAlg->execute();
    MatrixWorkspace_sptr energyWs = createWsAlg->getProperty("OutputWorkspace");

    IAlgorithm_sptr convertHistAlg = AlgorithmManager::Instance().create("ConvertToHistogram");
    convertHistAlg->setChild(true);
    convertHistAlg->initialize();
    convertHistAlg->setProperty("InputWorkspace", energyWs);
    convertHistAlg->setProperty("OutputWorkspace", "__energy");
    convertHistAlg->execute();
    energyWs = convertHistAlg->getProperty("OutputWorkspace");

    IAlgorithm_sptr loadInstAlg = AlgorithmManager::Instance().create("LoadInstrument");
    loadInstAlg->setChild(true);
    loadInstAlg->initialize();
    loadInstAlg->setProperty("Workspace", energyWs);
    loadInstAlg->setProperty("InstrumentName", instName.toStdString());
    loadInstAlg->execute();
    energyWs = loadInstAlg->getProperty("Workspace");

    std::string idfDirectory = Mantid::Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");
    QString ipfFilename = QString::fromStdString(idfDirectory) + instName + "_" + analyser + "_" + reflection + "_Parameters.xml";

    IAlgorithm_sptr loadParamAlg = AlgorithmManager::Instance().create("LoadParameterFile");
    loadParamAlg->setChild(true);
    loadParamAlg->initialize();
    loadParamAlg->setProperty("Workspace", energyWs);
    loadParamAlg->setProperty("Filename", ipfFilename.toStdString());
    loadParamAlg->execute();
    energyWs = loadParamAlg->getProperty("Workspace");

    double efixed = getEFixed(energyWs);

    auto spectrum = energyWs->getSpectrum(0);
    spectrum->setSpectrumNo(3);
    spectrum->clearDetectorIDs();
    spectrum->addDetectorID(3);

    IAlgorithm_sptr convUnitsAlg = AlgorithmManager::Instance().create("ConvertUnits");
    convUnitsAlg->setChild(true);
    convUnitsAlg->initialize();
    convUnitsAlg->setProperty("InputWorkspace", energyWs);
    convUnitsAlg->setProperty("OutputWorkspace", "__tof");
    convUnitsAlg->setProperty("Target", "TOF");
    convUnitsAlg->setProperty("EMode", "Indirect");
    convUnitsAlg->setProperty("EFixed", efixed);
    convUnitsAlg->execute();
    MatrixWorkspace_sptr tofWs = convUnitsAlg->getProperty("OutputWorkspace");

    std::vector<double> tofData = tofWs->readX(0);
    ranges["peak-start-tof"] = tofData[0];
    ranges["peak-end-tof"] = tofData[2];
    ranges["back-start-tof"] = tofData[3];
    ranges["back-end-tof"] = tofData[4];

    return ranges;
  }

} // namespace CustomInterfaces
} // namespace Mantid
