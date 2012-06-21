/*WIKI*

This algorithm takes a text file (.txt extension) containing two columns and converts it into an MDHistoWorkspace.

=== Details ===
The columns are in the order '''signal''' then '''error'''. The file must only contain two columns, these may be separated by any whitespace character.

The algorithm expects there to be 2*product(nbins in each dimension) entries in this file. So if you have set the dimensionality to be ''4,4,4'' then you will need to provide 64 rows of data, in 2 columns or 124 floating point entries.

The Names, Units, Extents and NumberOfBins inputs are all linked by the order they are provided in. For example, if you provide Names ''A, B, C'' and Units ''U1, U2, U3'' then the dimension ''A'' will have units ''U1''.

Signal and Error inputs are read in such that, the first entries in the file will be entered across the first dimension specified, and the zeroth index in the other dimensions. The second set of entries will be entered across the first dimension and the 1st index in the second dimension, and the zeroth index in the others.

*WIKI*/

#include "MantidMDEvents/ImportMDHistoWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/System.h"
#include <vector>
#include <deque>
#include <iostream>
#include <fstream>
#include <iterator>
#include <algorithm>


using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ImportMDHistoWorkspace)
  
  /**
  Functor to compute the product of the set.
  */
  struct Product : public std::unary_function<size_t, void>
  {
    Product() : result(1) {}
    size_t result;
    void operator()(size_t x) { result *= x; }
  };


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ImportMDHistoWorkspace::ImportMDHistoWorkspace()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ImportMDHistoWorkspace::~ImportMDHistoWorkspace()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string ImportMDHistoWorkspace::name() const { return "ImportMDHistoWorkspace";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int ImportMDHistoWorkspace::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string ImportMDHistoWorkspace::category() const { return "General";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void ImportMDHistoWorkspace::initDocs()
  {
    this->setWikiSummary("Reads a text file and generates an MDHistoWorkspace from it.");
    this->setOptionalMessage("Reads a text file and generates an MDHistoWorkspace from it.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void ImportMDHistoWorkspace::init()
  {
    std::vector<std::string> fileExtensions(1);
    fileExtensions[0]=".txt";
    declareProperty(new API::FileProperty("Filename","", API::FileProperty::Load,fileExtensions), "File of type txt");
    declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("OutputWorkspace","",Direction::Output), "MDHistoWorkspace reflecting the input text file.");

    auto validator = boost::make_shared<CompositeValidator>();
    validator->add(boost::make_shared<BoundedValidator<int> >(1,9));
    validator->add(boost::make_shared<MandatoryValidator<int> >());

    declareProperty(new PropertyWithValue<int>("Dimensionality", -1, validator, Direction::Input), "Dimensionality of the data in the file.");

    declareProperty(
      new ArrayProperty<double>("Extents"),
      "A comma separated list of min, max for each dimension,\n"
      "specifying the extents of each dimension.");
    
    declareProperty(
      new ArrayProperty<int>("NumberOfBins"), 
      "Number of bin in each dimension.");

    declareProperty(
      new ArrayProperty<std::string>("Names"), 
      "A comma separated list of the name of each dimension.");

    declareProperty(
      new ArrayProperty<std::string>("Units"), 
      "A comma separated list of the units of each dimension.");

  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ImportMDHistoWorkspace::exec()
  {
    // Fetch input properties
    size_t ndims = getProperty("Dimensionality");
    std::vector<double> extents = getProperty("Extents");
    std::vector<int> nbins = getProperty("NumberOfBins");
    std::vector<std::string> names = getProperty("Names");
    std::vector<std::string> units = getProperty("Units");
    std::string filename = getProperty("Filename");
    
    // Perform all validation on inputs
    if (extents.size() != ndims*2)
      throw std::invalid_argument("You must specify twice as many extents (min,max) as there are dimensions.");    
    if (nbins.size() != ndims)
      throw std::invalid_argument("You must specify as number of bins as there are dimensions.");
    if (names.size() != ndims)
      throw std::invalid_argument("You must specify as many names as there are dimensions.");
    if (units.size() != ndims)
      throw std::invalid_argument("You must specify as many units as there are dimensions.");
    
    // Fabricate new dimensions from inputs
    std::vector<MDHistoDimension_sptr> dimensions;
    for(int k = 0; k < ndims; ++k)
    {
      dimensions.push_back(MDHistoDimension_sptr(new MDHistoDimension(names[k], names[k], units[k], static_cast<coord_t>(extents[k*2]), static_cast<coord_t>(extents[(k*2) + 1]), nbins[k])));
    }
   
    /*
    Create a new output workspace. Note that the MDHistoWorkspace will take care of memory allocation associated with
    the internal arrays for signal and error values. So no need to provide any clean-up in this algorithm if allocation is not possible.
    */
    MDHistoWorkspace_sptr ws (new MDHistoWorkspace(dimensions));

    // Open the file
    std::ifstream file;
    try
    {
      file.open(filename.c_str(), std::ios::in);
    }
    catch (std::ifstream::failure e) 
    {
      g_log.error() << "Cannot open file: " << filename;
      throw e;
    }

    // Copy each string present in the file stream into a deque.
    typedef std::deque<std::string> box_collection;
    box_collection box_elements;
    std::copy(
      std::istream_iterator <std::string> ( file ),
      std::istream_iterator <std::string> (),
      std::back_inserter( box_elements )
      );

    // Release the resource.
    file.close();

    // Calculated the expected number of elements.
    Product answer = std::for_each(nbins.begin(), nbins.end(), Product());
    const size_t nElements = answer.result * 2;

    // Handle the case that the number of elements is wrong.
    if(box_elements.size() % nElements != 0)
    {
      throw std::invalid_argument("The number of data entries in the file, does not match up with the specified dimensionality.");
    }
    
    // Fetch out raw pointers to workspace arrays.
    double* signals = ws->getSignalArray();
    double* errors = ws->getErrorSquaredArray();

    //Write to the signal and error array from the deque.
    size_t currentBox = 0;
    for(box_collection::iterator it = box_elements.begin(); it != box_elements.end(); it+=2, ++currentBox)
    {
      box_collection::iterator temp = it;
      double signal = atof((*(temp)).c_str());
      double error = atof((*(++temp)).c_str());
      signals[currentBox] = signal;
      errors[currentBox] = error*error;
    }

    //Set the output.
    setProperty("OutputWorkspace", ws);
  }



} // namespace Mantid
} // namespace MDEvents