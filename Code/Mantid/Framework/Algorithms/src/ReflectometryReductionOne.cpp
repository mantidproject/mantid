/*WIKI*
Reduces a single TOF reflectometry run into a mod Q vs I/I0 workspace. Performs transmission corrections. Handles both point detector and multidetector cases.
*WIKI*/

#include "MantidAlgorithms/ReflectometryReductionOne.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ListValidator.h"
#include <boost/make_shared.hpp>
#include <vector>

using namespace Mantid::Kernel;
using namespace Mantid::API;


namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ReflectometryReductionOne)
  
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ReflectometryReductionOne::ReflectometryReductionOne()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ReflectometryReductionOne::~ReflectometryReductionOne()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string ReflectometryReductionOne::name() const { return "ReflectometryReductionOne";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int ReflectometryReductionOne::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string ReflectometryReductionOne::category() const { return "Reflectometry\\ISIS";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void ReflectometryReductionOne::initDocs()
  {
    this->setWikiSummary("Reduces a single TOF reflectometry run into a mod Q vs I/I0 workspace. Performs transmission corrections.");
    this->setOptionalMessage(this->getWikiSummary());
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void ReflectometryReductionOne::init()
  {
    boost::shared_ptr<CompositeValidator> inputValidator = boost::make_shared<CompositeValidator>();
    inputValidator->add(boost::make_shared<WorkspaceUnitValidator>("TOF"));

    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","", Direction::Input, inputValidator), "Run to reduce.");
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("FirstTransmissionRun","", Direction::Input, inputValidator->clone() ), "First transmission run, or the low wavelength transmision run if SecondTransmissionRun is also provided.");
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("SecondTransmissionRun","", Direction::Input, inputValidator->clone() ), "Second, high wavelength transmission run. Optional. Causes the FirstTransmissionRun to be treated as the low wavelength transmission run.");
    declareProperty(new PropertyWithValue<double>("Theta", -1, Direction::Input),  "Final theta value.");
    declareProperty(new PropertyWithValue<bool>("PointDetectorAnalysis", true, Direction::Input ), "Set the Mode for the analysis");


    std::vector<std::string> propOptions;
    propOptions.push_back("PointDetectorAnalysis");
    propOptions.push_back("MultiDetectorAnalysis");

    declareProperty("AnalysisMode", "PointDetectorAnalysis", boost::make_shared<StringListValidator>(propOptions),
          "The type of analysis to perform. Point detector or multi detector.");

    // Declare property for region of interest (workspace index ranges min, max)
    // Declare property for direct beam (workspace index ranges min, max)
    // Declare property for workspace index ranges
    // Declare property for lam min
    // Declare property for lam max
    // Declare property for integration min
    // Declare property for integration max
    // Declare property for workspace index I0
    // Declare property for monitor integral min
    // Declare property for monitor integral max
    // Declare property for monitor background min
    // Declare property for monitor background max.



    /*
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("LHSWorkspace","", Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("RHSWorkspace","", Direction::Input), "An output workspace.");
    declareProperty(new PropertyWithValue<double>("StartOverlap", 0.0, Direction::Input));
    declareProperty(new PropertyWithValue<double>("EndOverlap", 0.0, Direction::Input));
    declareProperty(new PropertyWithValue<std::string>("Params", "", Direction::Input));
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "", Direction::Output));
    declareProperty(new PropertyWithValue<double>("OutScaleFactor", 0.0, Direction::Output));
    */
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ReflectometryReductionOne::exec()
  {
    /*
    MatrixWorkspace_sptr lhsWS = this->getProperty("LHSWorkspace");
    MatrixWorkspace_sptr rhsWS = this->getProperty("RHSWorkspace");
    const double startOverlap = this->getProperty("StartOverlap");
    const double endOverlap = this->getProperty("EndOverlap");
    const std::string params = this->getProperty("Params");
    const std::string outputWSName = this->getPropertyValue("OutputWorkspace");

    IAlgorithm_sptr stitch = this->createChildAlgorithm("Stitch1D");
    stitch->setRethrows(true);
    stitch->setProperty("LHSWorkspace", lhsWS);
    stitch->setProperty("RHSWorkspace", rhsWS);
    stitch->setProperty("StartOverlap", startOverlap);
    stitch->setProperty("EndOverlap", endOverlap);
    stitch->setProperty("Params", params);
    stitch->setPropertyValue("OutputWorkspace", outputWSName);
    stitch->execute();
    MatrixWorkspace_sptr out = stitch->getProperty("OutputWorkspace");
    double scaleFactor = stitch->getProperty("OutScaleFactor");


    this->setProperty("OutScaleFactor", scaleFactor);
    this->setProperty("OutputWorkspace", out);
    */
  }



} // namespace Algorithms
} // namespace Mantid
