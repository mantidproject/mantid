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
    declareProperty(new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
        "An input workspace.");
    auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
    mustBePositive->setLower(0.0);
    declareProperty("IncidentEnergy", EMPTY_DBL(), mustBePositive,
      "Set the value of the incident energy in meV.");
    declareProperty("FixedIncidentEnergy", false,
        "Declare the value of the incident energy to be fixed (will not be calculated).");
    declareProperty(new ArrayProperty<double>("EnergyTransferRange",
        boost::make_shared<RebinParamsValidator>()),
      "A comma separated list of first bin boundary, width, last bin boundary.\n"
      "Negative width value indicates logarithmic binning.");
    declareProperty("ReductionProperties", "__dgs_reduction_properties", Direction::Input);
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void DgsConvertToEnergyTransfer::exec()
  {
    // Get the reduction property manager
    const std::string reductionManagerName = getProperty("ReductionProperties");
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

    MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
    const std::string inWsName = inputWS->getName();

    // Calculate the initial energy and time zero
    const std::string facility = ConfigService::Instance().getFacility().name();
    auto ei_guess = getProperty("IncidentEnergy");
    double initial_energy = 0.0;
    if ("SNS" == facility)
      {
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

        IAlgorithm_sptr alg = createSubAlgorithm("ChangeBinOffset");
        alg->setPropertyValue("InputWorkspace", inWsName);
        alg->setPropertyValue("OutputWorkspace", inWsName);
        alg->setProperty("Offset", -t0);
        alg->execute();
      }
    // Do ISIS
    else
      {

      }

    // Convert to energy transfer
    IAlgorithm_sptr cnvun = createSubAlgorithm("ConvertUnits");
    cnvun->setPropertyValue("InputWorkspace", inWsName);
    cnvun->setPropertyValue("OutputWorkspace", inWsName);
    cnvun->setProperty("Target", "Wavelength");
    cnvun->setProperty("EMode", "Direct");
    cnvun->setProperty("EFixed", initial_energy);
    cnvun->execute();

    // Correct for detector efficiency
    if ("SNS" == facility)
      {
        // He3TubeEfficiency requires the workspace to be in wavelength
        cnvun->setProperty("Target", "Wavelength");
        cnvun->execute();

        // Do the correction
        IAlgorithm_sptr alg2 = createSubAlgorithm("He3TubeEfficiency");
        alg2->setPropertyValue("InputWorkspace", inWsName);
        alg2->setPropertyValue("OutputWorkspace", inWsName);
        alg2->execute();

        // Convert back to energy transfer
        cnvun->setProperty("Target", "DeltaE");
        cnvun->execute();
      }
    // Do ISIS
    else
      {
        IAlgorithm_sptr alg = createSubAlgorithm("DetectorEfficiencyCor");
        alg->setPropertyValue("InputWorkspace", inWsName);
        alg->setPropertyValue("OutputWorkspace", inWsName);
        alg->execute();
      }

    // Correct for Ki/Kf
    IAlgorithm_sptr kikf = createSubAlgorithm("CorrectKiKf");
    kikf->setPropertyValue("InputWorkspace", inWsName);
    kikf->setPropertyValue("OutputWorkspace", inWsName);
    kikf->setProperty("EMode", "Direct");
    kikf->execute();

    // Should the workspace be cloned at any point?
    std::string outWsName = inWsName + "_et";
    IAlgorithm_sptr rename = createSubAlgorithm("RenameWorkspace");
    rename->setPropertyValue("InputWorkspace", inWsName);
    rename->setPropertyValue("OutputWorkspace", outWsName);
    rename->execute();

    setPropertyValue("OutputWorkspace", outWsName);
  }

} // namespace Mantid
} // namespace WorkflowAlgorithms
