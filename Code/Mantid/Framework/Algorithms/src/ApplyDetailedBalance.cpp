#include "MantidAlgorithms/ApplyDetailedBalance.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/PropertyWithValue.h"
#include "boost/lexical_cast.hpp"
#include <iostream>
#include <cmath>
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/PhysicalConstants.h"

using std::string;
using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ApplyDetailedBalance)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ApplyDetailedBalance::ApplyDetailedBalance()
  {
    // TODO Auto-generated constructor stub
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ApplyDetailedBalance::~ApplyDetailedBalance()
  {
    // TODO Auto-generated destructor stub
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void ApplyDetailedBalance::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void ApplyDetailedBalance::init()
  {
    CompositeWorkspaceValidator<> *wsValidator = new CompositeWorkspaceValidator<>;
    wsValidator->add(new WorkspaceUnitValidator<>("DeltaE"));
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
    declareProperty(new PropertyWithValue<string>("Temperature","",Direction::Input),"SampleLog variable name that contains the temperature or a number");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ApplyDetailedBalance::exec()
  {
     MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
     MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
     // If input and output workspaces are not the same, create a new workspace for the output
     if (outputWS != inputWS)
     {
       outputWS = API::WorkspaceFactory::Instance().create(inputWS);
     }

     std::string Tstring=getProperty("Temperature");
     double Temp;
     if (inputWS->run().hasProperty(Tstring))
       Temp=(dynamic_cast<Kernel::TimeSeriesProperty<double> *>(inputWS->run().getProperty(Tstring)))->getStatistics().mean;
     else
       Temp=boost::lexical_cast<double>(Tstring);
     double oneOverT=1.0/(Temp*PhysicalConstants::meVtoKelvin);
     // Run the exponential correction algorithm explicitly to enable progress reporting
     IAlgorithm_sptr expcor = createSubAlgorithm("OneMinusExponentialCor",0.0,1.0);
     expcor->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWS);
     expcor->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", outputWS);
     expcor->setProperty<double>("C1", M_PI);
     expcor->setProperty<double>("C", oneOverT);
     expcor->setPropertyValue("Operation","Multiply");
     expcor->executeAsSubAlg();
     // Get back the result
     outputWS = expcor->getProperty("OutputWorkspace");

     setProperty("OutputWorkspace",outputWS);
  }



} // namespace Mantid
} // namespace Algorithms

