/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidAPI/WorkspaceValidators.h"
#include "MantidMDEvents/ConvertToReflectometryQ.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/ReflectometryTransformQxQz.h"
#include "MantidMDEvents/ReflectometryTransformKiKf.h"
#include "MantidMDEvents/ReflectometryTransformP.h"
#include <boost/shared_ptr.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

/*Non member helpers*/
namespace
{
  /*
  Transform to q-space label:
  @return: associated id/label
  */
  std::string qSpaceTransform()
  {
    return "Q (lab frame)";
  }

  /*
  Transform to p-space label:
  @return: associated id/label
  */
  std::string pSpaceTransform()
  {
    return "P (lab frame)";
  }

    /*
  Transform to k-space label:
  @return: associated id/label
  */
  std::string kSpaceTransform()
  {
    return "K (incident, final)";
  }

  /*
  Check that the input workspace is of the correct type.
  @param: inputWS: The input workspace.
  @throw: runtime_error if the units do not appear to be correct/compatible with the algorithm.
  */
  void checkInputWorkspace(Mantid::API::MatrixWorkspace_const_sptr inputWs)
  {
    auto spectraAxis = inputWs->getAxis(1);
    const std::string label = spectraAxis->unit()->label();
    const std::string expectedLabel = "degrees";
    if(expectedLabel != label)
    {
      std::string message = "Spectra axis should have units of degrees. Instead found: " + label;
      throw std::runtime_error(message);
    }
  }

  /*
  Check the extents. 
  @param extents: A vector containing all the extents.
  @throw: runtime_error if the extents appear to be incorrect.
  */
  void checkExtents(const std::vector<double>& extents)
  {
    if(extents.size() != 4)
    {
      throw std::runtime_error("Four comma separated extents inputs should be provided");
    }
    if((extents[0] >= extents[1]) || (extents[2] >= extents[3]))
    {
      throw std::runtime_error("Extents must be provided min, max with min less than max!");
    }
  }

  /*
  Check the incident theta inputs.
  @param bUseOwnIncidentTheta: True if the user has requested to provide their own incident theta value.
  @param theta: The proposed incident theta value provided by the user.
  @throw: logic_error if the theta value is out of range.
  */
  void checkCustomThetaInputs(const bool bUseOwnIncidentTheta, const double& theta)
  { 
    if(bUseOwnIncidentTheta)
    {
      if(theta < 0 || theta >  90)
      {
        throw std::logic_error("Overriding incident theta is out of range");
      }
    }
  }

  /*
  General check for the indient theta.
  @param theta: The proposed incident theta value.
  @throw: logic_error if the theta value is out of range.
  */
  void checkIncidentTheta(const double& theta)
  {
   if(theta < 0 || theta >  90)
    {
      throw std::logic_error("Incident theta is out of range");
    }
  }

  /*
  Check for the output dimensionality.
  @param outputDimensions : requested output dimensionality
  @throw: runtime_errror if the dimensionality is not supported.
  */
  void checkOutputDimensionalityChoice(const std::string & outputDimensions )
  {
    if(outputDimensions != qSpaceTransform() && outputDimensions != kSpaceTransform() && outputDimensions != pSpaceTransform())
    {
      throw std::runtime_error("Unknown transformation");
    }
  }

}

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ConvertToReflectometryQ)
  
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ConvertToReflectometryQ::ConvertToReflectometryQ()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ConvertToReflectometryQ::~ConvertToReflectometryQ()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string ConvertToReflectometryQ::name() const { return "ConvertToReflectometryQ";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int ConvertToReflectometryQ::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string ConvertToReflectometryQ::category() const { return "General";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void ConvertToReflectometryQ::initDocs()
  {
    this->setWikiSummary("Transforms from real-space to Q or momentum space for reflectometry workspaces");
    this->setOptionalMessage("Transforms from real-space to Q or momentum space for reflectometry workspaces");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void ConvertToReflectometryQ::init()
  {
    auto unitValidator = boost::make_shared<API::WorkspaceUnitValidator>("Wavelength");
    auto compositeValidator = boost::make_shared<CompositeValidator>();
    compositeValidator->add(unitValidator);

    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input, compositeValidator),
        "An input workspace in wavelength");

    std::vector<std::string> propOptions;
    propOptions.push_back(qSpaceTransform());
    propOptions.push_back(pSpaceTransform());
    propOptions.push_back(kSpaceTransform());
    
    declareProperty("OutputDimensions", "Q (lab frame)" ,boost::make_shared<StringListValidator>(propOptions),
      "What will be the dimensions of the output workspace?\n"
      "  Q (lab frame): Wave-vector change of the lattice in the lab frame.\n"
      "  P (lab frame): Momentum in the sample frame.\n"
      "  K initial and final vectors in the z plane."
       );

    declareProperty(new Kernel::PropertyWithValue<bool>("OverrideIncidentTheta", false),
        "Use the provided incident theta value.");

    declareProperty(new PropertyWithValue<double>("IncidentTheta", -1),
        "An input incident theta value specified in degrees."
        "Optional input value for the incident theta specified in degrees.");

    std::vector<double> extents(4,0);
    extents[0]=-50;extents[1]=+50;extents[2]=-50;extents[3]=+50;
    declareProperty(
      new ArrayProperty<double>("Extents", extents),
      "A comma separated list of min, max for each dimension. Takes four values in the form dim_0_min, dim_0_max, dim_1_min, dim_1_max,\n"
      "specifying the extents of each dimension. Optional, default +-50 in each dimension.");

    setPropertySettings("IncidentTheta", new Kernel::EnabledWhenProperty("OverrideIncidentTheta", IS_EQUAL_TO, "1") );

    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output), "Output 2D Workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ConvertToReflectometryQ::exec()
  {
    Mantid::API::MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");
    bool bUseOwnIncidentTheta = getProperty("OverrideIncidentTheta"); 
    std::vector<double> extents = getProperty("Extents");
    double incidentTheta = getProperty("IncidentTheta");
    std::string outputDimensions = getPropertyValue("OutputDimensions");

    //Validation of input parameters
    checkInputWorkspace(inputWs);
    checkExtents(extents);
    checkCustomThetaInputs(bUseOwnIncidentTheta, incidentTheta);
    checkOutputDimensionalityChoice(outputDimensions); //TODO: This check can be retired as soon as all transforms have been implemented.
    
    // Extract the incient theta angle from the logs if a user provided one is not given.
    if(!bUseOwnIncidentTheta)
    {
      const Mantid::API::Run& run = inputWs->run();
      try
      {
        Property* p = run.getLogData("stheta");
        auto incidentThetas = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double>*>(p);
        incidentTheta = incidentThetas->valuesAsVector().back(); //Not quite sure what to do with the time series for stheta
        checkIncidentTheta(incidentTheta);
        std::stringstream stream;
        stream << "Extracted initial theta value of: " << incidentTheta;
        g_log.information(stream.str());
      }
      catch(Exception::NotFoundError&)
      {
        throw std::runtime_error("The input workspace does not have a stheta log value.");
      }
    }

    // Min max extent values.
    const double dim0min = extents[0];
    const double dim0max = extents[1];
    const double dim1min = extents[2];
    const double dim1max = extents[3];
    
    typedef boost::shared_ptr<ReflectometryMDTransform> ReflectometryMDTransform_sptr;

    //Select the transform strategy.
    ReflectometryMDTransform_sptr transform;
    if(outputDimensions == qSpaceTransform())
    {
      transform = ReflectometryMDTransform_sptr(new ReflectometryTransformQxQz(dim0min, dim0max, dim1min, dim1max, incidentTheta));
    }
    else if(outputDimensions == pSpaceTransform())
    {
      transform = ReflectometryMDTransform_sptr(new ReflectometryTransformP(dim0min, dim0max, dim1min, dim1max, incidentTheta));
    }
    else
    {
      transform = ReflectometryMDTransform_sptr(new ReflectometryTransformKiKf(dim0min, dim0max, dim1min, dim1max, incidentTheta));
    }

    //Execute the transform and bind to the output.
    setProperty("OutputWorkspace", transform->execute(inputWs));
  }

} // namespace Mantid
} // namespace MDAlgorithms