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
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/FrameworkManager.h"

#include <boost/shared_ptr.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

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
        "An input workspace in time-of-flight");

    std::vector<std::string> propOptions;
    propOptions.push_back("Q (lab frame)");
    propOptions.push_back("P (lab frame)");
    propOptions.push_back("K (initial, final)");
    
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

    setPropertySettings("IncidentTheta", new Kernel::EnabledWhenProperty("OverrideIncidentTheta", IS_EQUAL_TO, "1") );

    declareProperty(new WorkspaceProperty<IMDWorkspace>("OutputWorkspace","",Direction::Output), "Output 2D Workspace.");
  }

  bool incidentThetaInRange(const double& theta)
  {
    if(theta < 0 || theta >  90)
    {
      return false;
    }
    return true;
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ConvertToReflectometryQ::exec()
  {
    Mantid::API::MatrixWorkspace_const_sptr inputWs = getProperty("InputWorkspace");
    bool bUseOwnIncidentTheta = getProperty("OverrideIncidentTheta"); 
    double incidentTheta = getProperty("IncidentTheta");
    std::string outputDimensions = getPropertyValue("OutputDimensions");

    if(bUseOwnIncidentTheta)
    {
      if(!incidentThetaInRange(incidentTheta))
      {
        throw std::logic_error("Overriding incident theta is out of range");
      }
    }
    else
    {
      const Mantid::API::Run& run = inputWs->run();
      try
      {
        Property* p = run.getLogData("stheta");
        auto incidentThetas = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double>*>(p);
        incidentTheta = incidentThetas->valuesAsVector().back(); //Not quite sure what to do with the time series for stheta
      }
      catch(Exception::NotFoundError& ex)
      {
        throw std::runtime_error("The workspace does not have a stheta log value.");
      }
    }

    if(outputDimensions != "Q (lab frame)")
    {
      throw std::runtime_error("Transforms other than to Q have not been implemented yet");
    }

    auto spectraAxis = inputWs->getAxis(1);
    auto wavelengthAxis = inputWs->getAxis(0);
    const std::string caption = spectraAxis->unit()->caption();
    const std::string label = spectraAxis->unit()->label();
    const std::string expectedCaption = "Scattering angle";
    const std::string expectedLabel = "degrees";

    const double qzmin = -1;
    const double qxmin = -1;
    const double qxmax = 1;
    const double qzmax = 1;
    const size_t nbinsx = 100;
    const size_t nbinsz = 100;

    MDHistoDimension_sptr qxDim = MDHistoDimension_sptr(new MDHistoDimension("Qx","qx","(Ang^-1)", qxmin, qxmax, nbinsx)); 
    MDHistoDimension_sptr qzDim = MDHistoDimension_sptr(new MDHistoDimension("Qz","qz","(Ang^-1)", qzmin, qzmax, nbinsz)); 
    MDHistoWorkspace_sptr ws = MDHistoWorkspace_sptr(new MDHistoWorkspace(qxDim, qzDim));
  
    const double two_pi = 6.28318531;
    const double c_cos_theta_i = cos(incidentTheta);
    const double  c_sin_theta_i = sin(incidentTheta);
    for(size_t index = 0; index < inputWs->getNumberHistograms(); ++index)
    {

      auto counts = inputWs->readY(index);
      auto wavelengths = inputWs->readX(index);
      auto errors = inputWs->readE(index);
      size_t nInputBins = inputWs->isHistogramData() ? wavelengths.size() -1 : wavelengths.size();
      const double theta_final = spectraAxis->getValue(index)/2;
      const double c_sin_theta_f = sin(theta_final);
      const double c_cos_theta_f = cos(theta_final);
      for(size_t j = 0; j < nInputBins; ++j)
      {
        double lambda = wavelengths[j];
        double wavenumber = two_pi/lambda;
        double _qx = wavenumber * (c_cos_theta_f - c_cos_theta_i);
        double _qz = wavenumber * (c_sin_theta_f + c_sin_theta_i);
       
        /// If q-max and min are known a-prori, these boundrary case truncations are not required. See top of method for comment.
       _qx = _qx < qxmin ? qxmin : _qx;
       _qz = _qz < qzmin ? qzmin : _qz;
       _qx = _qx > qxmax ? qxmax : _qx;
       _qz = _qz > qzmax ? qzmax : _qz;

       //Set up for linear transformation qi -> dimension index.
 
       double mx = (nbinsx / (qxmax - qxmin));
 	     double mz = (nbinsz / (qzmax - qzmin));
       double cx = (nbinsx - mx * (qxmin + qxmax))/2;
       double cz = (nbinsz - mz * (qzmin + qzmax))/2;

       size_t posIndexX = mx*_qx + cx;
       size_t posIndexZ = mz*_qz + cz;
       size_t linearindex = (posIndexX * nbinsx) + posIndexZ;

       double error = errors[j];

       ws->setSignalAt(linearindex, ws->getSignalAt(linearindex) + counts[j]);
       ws->setErrorSquaredAt(linearindex, error*error);

      }
    }
    
    setProperty("OutputWorkspace", ws);
  }



} // namespace Mantid
} // namespace MDAlgorithms