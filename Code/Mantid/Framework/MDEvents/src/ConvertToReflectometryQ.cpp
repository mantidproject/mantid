/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidAPI/WorkspaceValidators.h"
#include "MantidMDEvents/ConvertToReflectometryQ.h"
#include "MantidKernel/System.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"

#include <boost/scoped_ptr.hpp>
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
    if(outputDimensions != qSpaceTransform())
    {
      throw std::runtime_error("Transforms other than to Q have not been implemented yet");
    }
  }

}

namespace Mantid
{
namespace MDEvents
{
  /*
  Interface for all reflectometry transformations.
  */
  class ReflectometryMDTransform 
  {
  public:
    virtual IMDEventWorkspace_sptr execute(IEventWorkspace_const_sptr eventWs) const = 0;
  };

  /*
  Tranforms to a 2D MDEventWorkspace with dimensions of Qx and Qz.
  */
  class TransformToQxQz : public ReflectometryMDTransform
  {
  private:
    const double m_qxMin;
    const double m_qxMax;
    const double m_qzMin;
    const double m_qzMax;
    const double m_incidentTheta;
  public:

    /*
    Constructor
    @param qxMin: min qx value (extent)
    @param qxMax: max qx value (extent)
    @param qzMin: min qz value (extent)
    @param qzMax; max qz value (extent)
    @param incidentTheta: Predetermined incident theta value
    */
    TransformToQxQz(double qxMin, double qxMax, double qzMin, double qzMax, double incidentTheta):
        m_qxMin(qxMin), m_qxMax(qxMax), m_qzMin(qzMin), m_qzMax(qzMax), m_incidentTheta(incidentTheta)
        {
        }

        /*
        Execute the transformtion. Generates an output IMDEventWorkspace.
        @return the constructed IMDEventWorkspace following the transformation.
        @param ws: Input EventWorkspace const shared pointer
        */
        virtual IMDEventWorkspace_sptr execute(IEventWorkspace_const_sptr eventWs) const
        {
          const size_t nbinsx = 10;
          const size_t nbinsz = 10;

          auto ws = boost::make_shared<MDEventWorkspace<MDLeanEvent<2>,2> >();
          MDHistoDimension_sptr qxDim = MDHistoDimension_sptr(new MDHistoDimension("Qx","qx","(Ang^-1)", static_cast<Mantid::coord_t>(m_qxMin), static_cast<Mantid::coord_t>(m_qxMax), nbinsx)); 
          MDHistoDimension_sptr qzDim = MDHistoDimension_sptr(new MDHistoDimension("Qz","qz","(Ang^-1)", static_cast<Mantid::coord_t>(m_qzMin), static_cast<Mantid::coord_t>(m_qzMax), nbinsz)); 

          ws->addDimension(qxDim);
          ws->addDimension(qzDim);

          // Set some reasonable values for the box controller
          BoxController_sptr bc = ws->getBoxController();
          bc->setSplitInto(2);
          bc->setSplitThreshold(10);

          // Initialize the workspace.
          ws->initialize();

          // Start with a MDGridBox.
          ws->splitBox();

          auto spectraAxis = eventWs->getAxis(1);
          const double two_pi = 6.28318531;
          const double c_cos_theta_i = cos(m_incidentTheta);
          const double  c_sin_theta_i = sin(m_incidentTheta);

          for(size_t index = 0; index < eventWs->getNumberHistograms(); ++index)
          {
            auto counts = eventWs->readY(index);
            auto wavelengths = eventWs->readX(index);
            auto errors = eventWs->readE(index);
            size_t nInputBins = eventWs->isHistogramData() ? wavelengths.size() -1 : wavelengths.size();
            const double theta_final = spectraAxis->getValue(index)/2;
            const double c_sin_theta_f = sin(theta_final);
            const double c_cos_theta_f = cos(theta_final);
            const double dirQx = (c_cos_theta_f - c_cos_theta_i);
            const double dirQz = (c_sin_theta_f + c_sin_theta_i);
            //Loop over all bins in spectra 
            for(size_t binIndex = 0; binIndex < nInputBins; ++binIndex)
            {
              double lambda = wavelengths[binIndex];
              double wavenumber = two_pi/lambda;
              double _qx = wavenumber * dirQx;
              double _qz = wavenumber * dirQz;

              double centers[2] = {_qx, _qz};

              ws->addEvent(MDLeanEvent<2>(float(counts[binIndex]), float(errors[binIndex]*errors[binIndex]), centers));
            }
            ws->splitAllIfNeeded(NULL);
          }
          return ws;
        }
  };


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
      "A comma separated list of min, max for each dimension. Takes four values in the form qx_min, qx_max, qz_min, qz_max,\n"
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
    const double qxmin = extents[0];
    const double qxmax = extents[1];
    const double qzmin = extents[2];
    const double qzmax = extents[3];
    
    /*
    Convert the input workspace to an eventworkspace if it is not already one. This allows us to dynamically rebin the results.
    */
    auto inputEventWs = boost::dynamic_pointer_cast<IEventWorkspace>(inputWs);
    if(!inputEventWs)
    {
      const std::string outputName = "ReflectometryEventWs";
      auto convertInput = this->createSubAlgorithm("ConvertToEventWorkspace");
      convertInput->setRethrows(true);
      convertInput->initialize();
      convertInput->setProperty("InputWorkspace", inputWs);
      convertInput->setPropertyValue("OutputWorkspace", outputName);
      convertInput->executeAsSubAlg();
      EventWorkspace_sptr result = convertInput->getProperty("OutputWorkspace");
      inputEventWs = result;
    }

    boost::scoped_ptr<ReflectometryMDTransform> transform(NULL);
    if(outputDimensions == qSpaceTransform())
    {
      transform.swap(boost::scoped_ptr<ReflectometryMDTransform>(new TransformToQxQz(qxmin, qxmax, qzmin, qzmax, incidentTheta)));
    }
    else if(outputDimensions == pSpaceTransform())
    {
      throw std::runtime_error("pSpaceTransform is not supported Yet.");
    }
    else
    {
      throw std::runtime_error("kSpaceTransform is not supported Yet");
    }

    setProperty("OutputWorkspace", transform->execute(inputEventWs));
  }

} // namespace Mantid
} // namespace MDAlgorithms