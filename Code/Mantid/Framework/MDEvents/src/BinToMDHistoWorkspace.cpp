#include "MantidMDEvents/BinToMDHistoWorkspace.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Strings.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include <boost/algorithm/string.hpp>

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(BinToMDHistoWorkspace)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::Geometry;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  BinToMDHistoWorkspace::BinToMDHistoWorkspace()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  BinToMDHistoWorkspace::~BinToMDHistoWorkspace()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void BinToMDHistoWorkspace::initDocs()
  {
    this->setWikiSummary("Take a [[MDEventWorkspace]] and bin into into a dense, multi-dimensional histogram workspace ([[MDHistoWorkspace]]).");
    this->setOptionalMessage("Take a MDEventWorkspace and bin into into a dense, multi-dimensional histogram workspace (MDHistoWorkspace).");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void BinToMDHistoWorkspace::init()
  {
    std::string dimHelp = "Enter it as a comma-separated list of values with the format: 'name,minimum,maximum,number_of_bins'.";
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace","",Direction::InOut), "An input MDEventWorkspace.");
    declareProperty(new PropertyWithValue<std::string>("DimX","",Direction::Input), "Binning parameters for the X dimension.\n" + dimHelp);
    declareProperty(new PropertyWithValue<std::string>("DimY","",Direction::Input), "Binning parameters for the Y dimension.\n" + dimHelp);
    declareProperty(new PropertyWithValue<std::string>("DimZ","",Direction::Input), "Binning parameters for the Z dimension.\n" + dimHelp);
    declareProperty(new PropertyWithValue<std::string>("DimT","",Direction::Input), "Binning parameters for the T dimension.\n" + dimHelp);
    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output), "A name for the output MDHistoWorkspace.");
  }


  //----------------------------------------------------------------------------------------------
  /** Generate a MDHistoDimension_sptr from a comma-sep string.
   *
   * @param str :: name,minimum,maximum,number_of_bins
   * @return
   */
  MDHistoDimension_sptr makeMDHistoDimensionFromString(const std::string & str)
  {
    std::string name, id;
    double min, max;
    int numBins = 0;
    std::vector<std::string> strs;
    boost::split(strs, str, boost::is_any_of(","));
    if (strs.size() != 4)
      throw std::invalid_argument("Wrong number of values (4 are expected) in the dimensions string: " + str);
    // Extract the arguments
    name = Strings::strip(strs[0]);
    id = name;
    Strings::convert(strs[1], min);
    Strings::convert(strs[2], max);
    Strings::convert(strs[3], numBins);
    if (name.size() == 0)
      throw std::invalid_argument("Name should not be blank.");
    if (min >= max)
      throw std::invalid_argument("Min should be > max.");
    if (numBins < 1)
      throw std::invalid_argument("Number of bins should be >= 1.");

    MDHistoDimension_sptr out(new MDHistoDimension(name, id, "", min, max, numBins));
    return out;
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void BinToMDHistoWorkspace::exec()
  {
    // Create the dimensions based on the strings from the user
    MDHistoDimension_sptr dimX,dimY,dimZ,dimT;
    dimX = makeMDHistoDimensionFromString( getPropertyValue("DimX"));
    dimY = makeMDHistoDimensionFromString( getPropertyValue("DimY"));
    dimZ = makeMDHistoDimensionFromString( getPropertyValue("DimZ"));
    dimT = makeMDHistoDimensionFromString( getPropertyValue("DimT"));

    IMDEventWorkspace_sptr in_ws = getProperty("InputWorkspace");

    Progress * prog = new Progress(this, 0, 1.0, 1); // This gets deleted by the thread pool!
    IMDWorkspace_sptr out = in_ws->centerpointBinToMDHistoWorkspace(dimX,dimY,dimZ,dimT, prog);

    // Save the output
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(out));

  }



} // namespace Mantid
} // namespace MDEvents

