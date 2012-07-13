/*WIKI*

This algorithm is responsible for making the conversion from time-of-flight to
energy transfer for direct geometry spectrometers.

*WIKI*/

#include "MantidWorkflowAlgorithms/DgsConvertToEnergyTransfer.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace WorkflowAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(DgsConvertToEnergyTransfer)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  DgsConvertToEnergyTransfer::DgsConvertToEnergyTransfer()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  DgsConvertToEnergyTransfer::~DgsConvertToEnergyTransfer()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string DgsConvertToEnergyTransfer::name() const { return "DgsConvertToEnergyTransfer"; };
  
  /// Algorithm's version for identification. @see Algorithm::version
  int DgsConvertToEnergyTransfer::version() const { return 1; };
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string DgsConvertToEnergyTransfer::category() const { return "Workflow\\Inelastic"; }

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void DgsConvertToEnergyTransfer::initDocs()
  {
    this->setWikiSummary("Algorithm to convert from TOF to energy transfer.");
    this->setOptionalMessage("Algorithm to convert from TOF to energy transfer.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void DgsConvertToEnergyTransfer::init()
  {
    this->declareProperty(new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
        "An input workspace.");
    auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
    mustBePositive->setLower(0.0);
    this->declareProperty("IncidentEnergy", EMPTY_DBL(), mustBePositive,
      "Set the value of the incident energy in meV.");
    this->declareProperty("FixedIncidentEnergy", false,
        "Declare the value of the incident energy to be fixed (will not be calculated).");
    this->declareProperty(new ArrayProperty<double>("EnergyTransferRange",
        boost::make_shared<RebinParamsValidator>(true)),
      "A comma separated list of first bin boundary, width, last bin boundary.\n"
      "Negative width value indicates logarithmic binning.");
    this->declareProperty("SofPhiEIsDistribution", true,
        "The final S(Phi, E) data is made to be a distribution.");
    this->declareProperty(new WorkspaceProperty<>("OutputWorkspace", "",
        Direction::Output, PropertyMode::Optional));
    this->declareProperty("ReductionProperties", "__dgs_reduction_properties", Direction::Input);
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void DgsConvertToEnergyTransfer::exec()
  {
    g_log.notice() << "Starting DgsConvertToEnergyTransfer" << std::endl;
    // Get the reduction property manager
    const std::string reductionManagerName = this->getProperty("ReductionProperties");
    boost::shared_ptr<PropertyManager> reductionManager;
    if (PropertyManagerDataService::Instance().doesExist(reductionManagerName))
    {
      reductionManager = PropertyManagerDataService::Instance().retrieve(reductionManagerName);
    }
    else
    {
      reductionManager = boost::make_shared<PropertyManager>();
      PropertyManagerDataService::Instance().addOrReplace(reductionManagerName, reductionManager);
    }

    MatrixWorkspace_const_sptr inputWS = this->getProperty("InputWorkspace");
    const std::string inWsName = inputWS->getName();
    std::string outWsName = inWsName + "_et";

    bool preserveEvents = false;

    // Calculate the initial energy and time zero
    const std::string facility = ConfigService::Instance().getFacility().name();
    g_log.notice() << "Processing for " << facility << std::endl;
    const double ei_guess = this->getProperty("IncidentEnergy");
    double initial_energy = 0.0;
    if ("SNS" == facility)
      {
        // SNS wants to preserve events until the last
        preserveEvents = true;
        const std::string instName = inputWS->getInstrument()->getName();
        double t0 = 0.0;
        if ("CNCS" == instName || "HYSPEC" == instName)
          {
            initial_energy = ei_guess;
            if ("HYSPEC" == instName)
              {
                double pow_val = initial_energy / 27.0;
                t0 = 25.0 + 85.0 / (1.0 + std::pow(pow_val, 4));
              }
            // Do CNCS
            else
              {
                double pow_val = 1.0 + initial_energy;
                t0 = (0.1982 * std::pow(pow_val, -0.84098)) * 1000.0;
              }
          }
        // Do ARCS and SEQUOIA
        else
          {

          }

        g_log.notice() << "Adjusting for T0" << std::endl;
        IAlgorithm_sptr alg = this->createSubAlgorithm("ChangeBinOffset");
        alg->setAlwaysStoreInADS(true);
        alg->setProperty("InputWorkspace", inWsName);
        alg->setProperty("OutputWorkspace", outWsName);
        alg->setProperty("Offset", -t0);
        alg->execute();
      }
    // Do ISIS
    else
      {

      }

    // Convert to energy transfer
    g_log.notice() << "Converting to energy transfer." << std::endl;
    IAlgorithm_sptr cnvun = this->createSubAlgorithm("ConvertUnits");
    cnvun->setAlwaysStoreInADS(true);
    cnvun->setProperty("InputWorkspace", outWsName);
    cnvun->setProperty("OutputWorkspace", outWsName);
    cnvun->setProperty("Target", "DeltaE");
    cnvun->setProperty("EMode", "Direct");
    cnvun->setProperty("EFixed", initial_energy);
    cnvun->execute();

    // Rebin if necessary
    const std::vector<double> et_binning = this->getProperty("EnergyTransferRange");
    if (!et_binning.empty())
      {
        g_log.notice() << "Rebinning data" << std::endl;
        IAlgorithm_sptr rebin = this->createSubAlgorithm("Rebin");
        rebin->setAlwaysStoreInADS(true);
        rebin->setProperty("InputWorkspace", outWsName);
        rebin->setProperty("OutputWorkspace", outWsName);
        rebin->setProperty("Params", et_binning);
        rebin->setProperty("PreserveEvents", preserveEvents);
        rebin->execute();
      }

    // Correct for detector efficiency
    if ("SNS" == facility)
      {
        // He3TubeEfficiency requires the workspace to be in wavelength
        cnvun->setAlwaysStoreInADS(true);
        cnvun->setProperty("InputWorkspace", outWsName);
        cnvun->setProperty("OutputWorkspace", outWsName);
        cnvun->setProperty("Target", "Wavelength");
        cnvun->execute();

        // Do the correction
        IAlgorithm_sptr alg2 = this->createSubAlgorithm("He3TubeEfficiency");
        alg2->setAlwaysStoreInADS(true);
        alg2->setProperty("InputWorkspace", outWsName);
        alg2->setProperty("OutputWorkspace", outWsName);
        alg2->execute();

        // Convert back to energy transfer
        cnvun->setAlwaysStoreInADS(true);
        cnvun->setProperty("InputWorkspace", outWsName);
        cnvun->setProperty("OutputWorkspace", outWsName);
        cnvun->setProperty("Target", "DeltaE");
        cnvun->execute();
      }
    // Do ISIS
    else
      {
        IAlgorithm_sptr alg = this->createSubAlgorithm("DetectorEfficiencyCor");
        alg->setAlwaysStoreInADS(true);
        alg->setProperty("InputWorkspace", outWsName);
        alg->setProperty("OutputWorkspace", outWsName);
        alg->execute();
      }

    // Correct for Ki/Kf
    IAlgorithm_sptr kikf = this->createSubAlgorithm("CorrectKiKf");
    kikf->setAlwaysStoreInADS(true);
    kikf->setProperty("InputWorkspace", outWsName);
    kikf->setProperty("OutputWorkspace", outWsName);
    kikf->setProperty("EMode", "Direct");
    kikf->execute();

    // Rebin to ensure consistency
    if (!et_binning.empty())
      {
        const bool sofphie_is_distribution = this->getProperty("SofPhiEIsDistribution");

        g_log.notice() << "Rebinning data" << std::endl;
        IAlgorithm_sptr rebin = this->createSubAlgorithm("Rebin");
        rebin->setAlwaysStoreInADS(true);
        rebin->setProperty("InputWorkspace", outWsName);
        rebin->setProperty("OutputWorkspace", outWsName);
        rebin->setProperty("Params", et_binning);
        if (sofphie_is_distribution)
          {
            rebin->setProperty("PreserveEvents", false);
          }
        rebin->execute();

        if (sofphie_is_distribution)
          {
            g_log.notice() << "Making distribution" << std::endl;
            IAlgorithm_sptr distrib = this->createSubAlgorithm("ConvertToDistribution");
            distrib->setProperty("Workspace", outWsName);
            distrib->execute();
          }
      }

    MatrixWorkspace_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWsName);
    this->setProperty("OutputWorkspace", outputWS);
  }

} // namespace Mantid
} // namespace WorkflowAlgorithms
