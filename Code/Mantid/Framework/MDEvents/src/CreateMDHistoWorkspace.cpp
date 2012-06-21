/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidMDEvents/CreateMDHistoWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include <algorithm>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{
  /**
  Helper type to compute the square in-place.
  */
  struct Square : public std::unary_function<double, void>
  {
    void operator()(double& i)
    {
      i*=i;
    }
  };



  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(CreateMDHistoWorkspace)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  CreateMDHistoWorkspace::CreateMDHistoWorkspace()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CreateMDHistoWorkspace::~CreateMDHistoWorkspace()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string CreateMDHistoWorkspace::name() const { return "CreateMDHistoWorkspace";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int CreateMDHistoWorkspace::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string CreateMDHistoWorkspace::category() const { return "General";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void CreateMDHistoWorkspace::initDocs()
  {
    this->setWikiSummary("Creates an MDHistoWorkspace from supplied lists of signal and error values.");
    this->setOptionalMessage("Creates an MDHistoWorkspace from supplied lists of signal and error values.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void CreateMDHistoWorkspace::init()
  {
    declareProperty(
      new ArrayProperty<double>("SignalInput"),
      "A comma separated list of all the signal values required for the workspace");

    declareProperty(
      new ArrayProperty<double>("ErrorInput"),
      "A comma separated list of all the error values required for the workspace");
   
    // Declare all the generic properties required.
    this->initGenericImportProps();
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void CreateMDHistoWorkspace::exec()
  {
     MDHistoWorkspace_sptr ws = this->createEmptyOutputWorkspace();
     double* signals = ws->getSignalArray();
     double* errors = ws->getErrorSquaredArray();

     std::vector<double> signalValues = getProperty("SignalInput");
     std::vector<double> errorValues = getProperty("ErrorInput");

     size_t binProduct = this->getBinProduct();
     std::stringstream stream;
     stream << binProduct;
     if(binProduct != signalValues.size())
     {
       throw std::invalid_argument("Expected size of the SignalInput is: " + stream.str() );
     }
     if(binProduct != errorValues.size())
     {
       throw std::invalid_argument("Expected size of the ErrorInput is: " + stream.str() );
     }

     //Copy from property
     std::copy(signalValues.begin(), signalValues.end(), signals);
     //Clean up.
     signalValues.swap(std::vector<double>());
     //Copy from property
     std::for_each(errorValues.begin(), errorValues.end(), Square());
     std::copy(errorValues.begin(), errorValues.end(), errors);
     //Clean up
     errorValues.swap(std::vector<double>());

     setProperty("OutputWorkspace", ws);

  }



} // namespace Mantid
} // namespace MDEvents