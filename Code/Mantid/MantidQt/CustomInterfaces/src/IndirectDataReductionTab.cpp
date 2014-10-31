#include "MantidQtCustomInterfaces/IndirectDataReductionTab.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Logger.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;

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
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  IndirectDataReductionTab::~IndirectDataReductionTab()
  {
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
    std::string instWorkspaceName = "__empty_" + instrumentName;
    std::string idfDirectory = Mantid::Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");

    // If the workspace does not exist in ADS then load an ampty instrument
    if(AnalysisDataService::Instance().doesExist(instWorkspaceName))
    {
      std::string parameterFilename = idfDirectory + instrumentName + "_Definition.xml";
      IAlgorithm_sptr loadAlg = AlgorithmManager::Instance().create("LoadEmptyInstrument");
      loadAlg->initialize();
      loadAlg->setProperty("Filename", parameterFilename);
      loadAlg->setProperty("OutputWorkspace", instWorkspaceName);
      loadAlg->execute();
    }

    // Load the IPF if given an analyser and reflection
    if(!analyser.empty() && !reflection.empty())
    {
      std::string ipfFilename = idfDirectory + instrumentName + "_" + analyser + "_" + reflection + "_Parameters.xml";
      IAlgorithm_sptr loadParamAlg = AlgorithmManager::Instance().create("LoadParameterFile");
      loadParamAlg->initialize();
      loadParamAlg->setProperty("Filename", ipfFilename);
      loadParamAlg->setProperty("Workspace", instWorkspaceName);
      loadParamAlg->execute();
    }

    // Get the workspace, which should exist now
    MatrixWorkspace_sptr instWorkspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(instWorkspaceName);

    return instWorkspace;
  }

  /**
   * Gets the operation modes for a given instrument as defined in it's parameter file.
   *
   * @param instrumentName The name of an indirect instrument (IRIS, OSIRIS, TOSCA, VESUVIO)
   * @returns A list of analysers and a vector of reflections that can be used with each
   */
  std::vector<std::pair<std::string, std::vector<std::string> > > IndirectDataReductionTab::getInstrumentModes(std::string instrumentName)
  {
    std::vector<std::pair<std::string, std::vector<std::string> > > modes;
    MatrixWorkspace_sptr instWorkspace = loadInstrumentIfNotExist(instrumentName);
    Instrument_const_sptr instrument = instWorkspace->getInstrument();

    std::vector<std::string> analysers;
    boost::split(analysers, instrument->getStringParameter("analysers")[0], boost::is_any_of(","));

    for(auto it = analysers.begin(); it != analysers.end(); ++it)
    {
      std::string analyser = *it;
      std::string ipfReflections = instrument->getStringParameter("refl-" + analyser)[0];

      std::vector<std::string> reflections;
      boost::split(reflections, ipfReflections, boost::is_any_of(","), boost::token_compress_on);

      std::pair<std::string, std::vector<std::string> > data(analyser, reflections);
      modes.push_back(data);
    }

    return modes;
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

    // Get the instrument workspace
    MatrixWorkspace_sptr instWorkspace = loadInstrumentIfNotExist(instrumentName, analyser, reflection);

    // In the IRIS IPF there is no fmica component
    if(instrumentName == "IRIS" && analyser == "fmica")
      analyser = "mica";

    // Get the instrument
    auto instrument = instWorkspace->getInstrument()->getComponentByName(analyser);
    if(instrument == NULL)
      return instDetails;

    // For each parameter we want to get
    for(auto it = ipfElements.begin(); it != ipfElements.end(); ++it)
    {
      try
      {
        std::string key = *it;
        QString value;

        // Determint it's type and call the corresponding get function
        std::string paramType = instrument->getParameterType(key);

        if(paramType == "string")
          value = QString::fromStdString(instrument->getStringParameter(key)[0]);

        if(paramType == "double")
          value = QString::number(instrument->getNumberParameter(key)[0]);

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

} // namespace CustomInterfaces
} // namespace Mantid
