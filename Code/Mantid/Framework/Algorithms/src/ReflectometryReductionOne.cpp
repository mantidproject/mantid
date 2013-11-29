/*WIKI*
Reduces a single TOF reflectometry run into a mod Q vs I/I0 workspace. Performs transmission corrections. Handles both point detector and multidetector cases.
*WIKI*/

#include "MantidAlgorithms/ReflectometryReductionOne.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>
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

  namespace
  {
    const std::string multiDetectorAnalysis = "MultiDetectorAnalysis";
    const std::string pointDetectorAnalysis = "PointDetectorAnalysis";
    typedef boost::optional< MatrixWorkspace_sptr > OptionalMatrixWorkspace_sptr;
    typedef boost::optional<std::vector<int> > OptionalWorkspaceIndexes;
    typedef boost::optional<double> OptionalDouble;
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void ReflectometryReductionOne::init()
  {
    boost::shared_ptr<CompositeValidator> inputValidator = boost::make_shared<CompositeValidator>();
    inputValidator->add(boost::make_shared<WorkspaceUnitValidator>("TOF"));

    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","", Direction::Input, inputValidator), "Run to reduce.");
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("FirstTransmissionRun","", Direction::Input, PropertyMode::Optional, inputValidator->clone() ), "First transmission run, or the low wavelength transmision run if SecondTransmissionRun is also provided.");
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("SecondTransmissionRun","", Direction::Input, PropertyMode::Optional, inputValidator->clone() ), "Second, high wavelength transmission run. Optional. Causes the FirstTransmissionRun to be treated as the low wavelength transmission run.");
    declareProperty(new PropertyWithValue<double>("Theta", -1, Direction::Input),  "Final theta value.");

    declareProperty(
                new ArrayProperty<double>("Params", boost::make_shared<RebinParamsValidator>(true)),
                "A comma separated list of first bin boundary, width, last bin boundary. "
                "These parameters are used for stitching together transmission runs. "
                "Values are in q. This input is only needed if a SecondTransmission run is provided."
                );

    std::vector<std::string> propOptions;
    propOptions.push_back( pointDetectorAnalysis );
    propOptions.push_back( multiDetectorAnalysis );

    declareProperty("AnalysisMode", "PointDetectorAnalysis", boost::make_shared<StringListValidator>(propOptions),
          "The type of analysis to perform. Point detector or multi detector.");

    declareProperty(new ArrayProperty<int>("RegionOfInterest"),"Indices of the spectra a pair (lower, upper) that mark the ranges that correspond to the region of interest (reflected beam) in multi-detector mode.");
    declareProperty(new ArrayProperty<int>("RegionOfDirectBeam"),"Indices of the spectra a pair (lower, upper) that mark the ranges that correspond to the direct beam in multi-detector mode.");

    declareProperty(new PropertyWithValue<double>("WavelengthMin", Mantid::EMPTY_DBL(), boost::make_shared<MandatoryValidator<double> >(), Direction::Input), "Wavelength minimum");
    declareProperty(new PropertyWithValue<double>("WavelengthMax", Mantid::EMPTY_DBL(), boost::make_shared<MandatoryValidator<double> >(), Direction::Input), "Wavelength maximum");

    boost::shared_ptr<CompositeValidator> mandatoryWorkspaceIndex = boost::make_shared<CompositeValidator>();
    mandatoryWorkspaceIndex->add(boost::make_shared<MandatoryValidator<int> >());
    auto boundedIndex = boost::make_shared<BoundedValidator<int> > ();
    boundedIndex->setLower(0);
    mandatoryWorkspaceIndex->add(boundedIndex);

    declareProperty(new ArrayProperty<int>("WorkspaceIndexList"),"Indices of the spectra in pairs (lower, upper) that mark the ranges that correspond to detectors of interest.");

    declareProperty(new PropertyWithValue<int>("I0MonitorIndex", Mantid::EMPTY_INT(), mandatoryWorkspaceIndex), "I0 monitor index");

    declareProperty(new PropertyWithValue<double>("MonitorBackgroundWavelengthMin", Mantid::EMPTY_DBL(), boost::make_shared<MandatoryValidator<double> >(), Direction::Input), "Wavelength minimum for monitor background. Taken to be WavelengthMin if not provided.");
    declareProperty(new PropertyWithValue<double>("MonitorBackgroundWavelengthMax", Mantid::EMPTY_DBL(), boost::make_shared<MandatoryValidator<double> >(), Direction::Input), "Wavelength maximum for monitor background. Taken to be WavelengthMax if not provided.");

    declareProperty(new PropertyWithValue<double>("MonitorIntegrationWavelengthMin", Mantid::EMPTY_DBL(), boost::make_shared<MandatoryValidator<double> >(), Direction::Input), "Wavelength minimum for integration. Taken to be WavelengthMin if not provided.");
    declareProperty(new PropertyWithValue<double>("MonitorIntegrationWavelengthMax", Mantid::EMPTY_DBL(), boost::make_shared<MandatoryValidator<double> >(), Direction::Input), "Wavelength maximum for integration. Taken to be WavelengthMax if not provided.");

    declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output));

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

  bool ReflectometryReductionOne::isPropertyDefault(const std::string& propertyName) const
  {
    Property* property = this->getProperty(propertyName);
    return property->isDefault();
  }

  bool checkNotPositive(const int value)
  {
    return value < 0;
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ReflectometryReductionOne::exec()
  {
    MatrixWorkspace_sptr runWS = getProperty("InputWorkspace");

    OptionalMatrixWorkspace_sptr firstTransmissionRun;
    if ( !isPropertyDefault("FirstTransmissionRun") )
    {
      MatrixWorkspace_sptr temp = this->getProperty("FirstTransmissionRun");
      firstTransmissionRun = temp;
    }

    OptionalMatrixWorkspace_sptr secondTransmissionRun;
    OptionalDouble stitchingStartQ;
    OptionalDouble stitchingDeltaQ;
    OptionalDouble stitchingEndQ;

    if ( !isPropertyDefault("SecondTransmissionRun") )
    {
      if( isPropertyDefault("FirstTransmissionRun") )
      {
        throw std::invalid_argument("A SecondTransmissionRun is only valid if a FirstTransmissionRun is provided.");
      }
      MatrixWorkspace_sptr temp = this->getProperty("SecondTransmissionRun");
      secondTransmissionRun = temp;
      if (isPropertyDefault("Params"))
      {
        throw std::invalid_argument("If a SecondTransmissionRun has been given, then stitching Params are also required.");
      }
      else
      {
        std::vector<double> params = getProperty("Params");
        stitchingStartQ = params[0];
        stitchingDeltaQ = params[1];
        stitchingEndQ = params[2];
      }
    }

    OptionalDouble theta;
    if ( !isPropertyDefault("Theta") )
    {
      double temp = this->getProperty("Theta");
      theta = temp;
    }

    const std::string strAnalysisMode = getProperty("AnalysisMode");
    const bool isPointDetector = (pointDetectorAnalysis.compare(strAnalysisMode) == 0);

    const double wavelengthMin = getProperty("WavelengthMin");
    const double wavelengthMax = getProperty("WavelengthMax");
    if (wavelengthMin > wavelengthMax)
    {
      throw std::invalid_argument("Cannot have WavelengthMin > WavelengthMax");
    }
    const int i0MonitorIndex = getProperty("I0MonitorIndex");
    const double monitorBackgroundWavelengthMin = getProperty("MonitorBackgroundWavelengthMin");
    const double monitorBackgroundWavelengthMax = getProperty("MonitorBackgroundWavelengthMax");
    if (monitorBackgroundWavelengthMin > monitorBackgroundWavelengthMax)
    {
      throw std::invalid_argument("Cannot have MonitorBackgroundWavelengthMin > MonitorBackgroundWavelengthMax");
    }
    const double monitorIntegrationMin = getProperty("MonitorIntegrationWavelengthMin");
    const double monitorIntegrationMax = getProperty("MonitorIntegrationWavelengthMax");
    if (monitorIntegrationMin > monitorIntegrationMax )
    {
      throw std::invalid_argument("Cannot have MonitorIntegrationWavelengthMin > MonitorIntegrationWavelengthMax");
    }

    const std::vector<int> indexList = getProperty("WorkspaceIndexList");
    if(indexList.size() % 2 != 0 || indexList.size() == 0)
    {
      throw std::invalid_argument("WorkspaceIndex list must be composed of pairs of min index, max index.");
    }

    if( std::find_if(indexList.begin(), indexList.end(), checkNotPositive) != indexList.end() )
    {
      throw std::invalid_argument("WorkspaceIndexList contains negative indexes");
    }

    for(size_t i = 0; (i+1) < indexList.size(); i+=2)
    {
      if (indexList[i] > indexList[i+1])
        throw std::invalid_argument("WorkspaceIndexList pairs must be in min, max order");
    }

    OptionalWorkspaceIndexes regionOfInterest;
    if( !isPropertyDefault("RegionOfInterest") )
    {
      if( isPointDetector )
      {
        throw std::invalid_argument("Cannot have a region of interest property in point detector mode.");
      }
      std::vector<int> temp = getProperty("RegionOfInterest");
      regionOfInterest = temp;
    }

    OptionalWorkspaceIndexes directBeam;
    if( !isPropertyDefault("RegionOfDirectBeam") )
    {
      if( isPointDetector )
      {
        throw std::invalid_argument("Cannot have a direct beam property in point detector mode.");
      }
      std::vector<int> temp = getProperty("RegionOfDirectBeam");
      directBeam = temp;
    }




    auto cloneAlg = this->createChildAlgorithm("CloneWorkspace");
    cloneAlg->initialize();
    cloneAlg->setProperty("InputWorkspace", runWS);
    cloneAlg->setPropertyValue("OutputWorkspace", "OutWS");
    cloneAlg->execute();
    Workspace_sptr cloned = cloneAlg->getProperty("OutputWorkspace");

    setProperty("OutputWorkspace", cloned );
    // Convert To Lambda.


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
