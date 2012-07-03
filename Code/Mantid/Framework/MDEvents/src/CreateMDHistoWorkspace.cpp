/*WIKI*
Takes two arrays of signal and error values, as well as information describing the dimensionality and extents, and creates a MDHistoWorkspace (histogrammed multi-dimensional workspace). The ''SignalInput'' and ''ErrorInput'' arrays must be of equal length and have a length that is equal to the product of all the comma separated arguments provided to '''NumberOfBins'''. The order in which the arguments are specified to each of the properties (for those taking multiple arguments) is important, as they are assumed to match by the order they are declared. For example, specifying '''Names'''='A,B' and '''Units'''='U1,U2' will generate two dimensions, the first with a name of ''A'' and units of ''U1'' and the second with a name of ''B'' and units of ''U2''. The same logic applies to the '''Extents''' inputs. Signal and Error inputs are read in such that, the first entries in the file will be entered across the first dimension specified, and the zeroth index in the other dimensions. The second set of entries will be entered across the first dimension and the 1st index in the second dimension, and the zeroth index in the others.

== Usage ==

The following example creates a 2D MDHistoWorkspace called ''demo'' with 3 bins in each dimension and and extents spanning from -1 to 1 in each dimension. The first dimension is called A, and has units of U, the second is called B and has units of T.

 CreateMDHistoWorkspace(SignalInput='1,2,3,4,5,6,7,8,9',ErrorInput='1,1,1,1,1,1,1,1,1',Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins='3,3',Names='A,B',Units='U,T',OutputWorkspace='demo')

The following example creates a 1D sine function

  import math
  
  signals=[]
  errors=[]
  pi = 3.14159
  extents = [-2*pi,2*pi]
  nbins = [100]
  dimensionality = 1
  step = float((extents[1] - extents[0])/nbins[0])
  for i in range(0, nbins[0]):
      x = i*step;
      signals.append(math.sin(x))
      errors.append(math.cos(x))
 CreateMDHistoWorkspace(SignalInput=signals,ErrorInput=errors,Dimensionality=dimensionality,Extents=extents,NumberOfBins=nbins,Names='x',Units='dimensionless',OutputWorkspace='demo')

== Alternatives ==
A very similar algorithm to this is [[ImportMDHistoWorkspace]], which takes it's input signal and error values from a text file rather than from arrays. Another alternative is to use [[CnvrtToMD]] which works on MatrixWorkspaces, and allows log values to be included in the dimensionality.

[[Category:MDAlgorithms]]

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
     std::vector<double> empty;
     //Clean up.
     signalValues.swap(empty);
     //Copy from property
     std::for_each(errorValues.begin(), errorValues.end(), Square());
     std::copy(errorValues.begin(), errorValues.end(), errors);
     //Clean up
     errorValues.swap(empty);

     setProperty("OutputWorkspace", ws);

  }



} // namespace Mantid
} // namespace MDEvents