#include "MantidAPI/ImplicitFunction.h"
#include "MantidAPI/ImplicitFunctionFactory.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/BinToMDHistoWorkspace.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
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
    this->setWikiDescription(
        "This algorithm performs dense binning of the events in multiple dimensions of an input [[MDEventWorkspace]] "
        "and places them into a dense MDHistoWorkspace with 1-4 dimensions."
        "\n\n"
        "The input MDEventWorkspace may have more dimensions than the number of output dimensions. "
        "The names of the dimensions in the DimX, etc. parameters are used to find the corresponding dimensions that will "
        "be created in the output."
        "\n\n"
        "An ImplicitFunction can be defined using the ImplicitFunctionXML parameter; any points NOT belonging inside of the "
        "ImplicitFunction will be set as NaN (not-a-number). "
        "\n\n"
        "As of now, binning is only performed along axes perpendicular to the dimensions defined "
        "in the MDEventWorkspace."
        );
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void BinToMDHistoWorkspace::init()
  {
    std::string dimHelp = "Enter it as a comma-separated list of values with the format: 'name,minimum,maximum,number_of_bins'. Leave blank for NONE.";
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace","",Direction::InOut), "An input MDEventWorkspace.");
    declareProperty(new PropertyWithValue<std::string>("DimX","",Direction::Input), "Binning parameters for the X dimension.\n" + dimHelp);
    declareProperty(new PropertyWithValue<std::string>("DimY","",Direction::Input), "Binning parameters for the Y dimension.\n" + dimHelp);
    declareProperty(new PropertyWithValue<std::string>("DimZ","",Direction::Input), "Binning parameters for the Z dimension.\n" + dimHelp);
    declareProperty(new PropertyWithValue<std::string>("DimT","",Direction::Input), "Binning parameters for the T dimension.\n" + dimHelp);
    declareProperty(new PropertyWithValue<std::string>("ImplicitFunctionXML","",Direction::Input), "XML string describing the implicit function determining which bins to use.");
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
    if (str.empty())
    {
      // Make a blank dimension
      MDHistoDimension_sptr out;
      return out;
    }
    else
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
  }

  //----------------------------------------------------------------------------------------------
  /** Templated method to apply the binning operation to the particular
   * MDEventWorkspace passed in.
   *
   * @param ws :: MDEventWorkspace of the given type
   */
  template<typename MDE, size_t nd>
  void BinToMDHistoWorkspace::do_centerpointBin(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    out = ws->centerpointBinToMDHistoWorkspace(dimensions, implicitFunc, prog);
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void BinToMDHistoWorkspace::exec()
  {
    // Create the dimensions based on the strings from the user
    dimensions.push_back( makeMDHistoDimensionFromString( getPropertyValue("DimX")) );
    dimensions.push_back( makeMDHistoDimensionFromString( getPropertyValue("DimY")) );
    dimensions.push_back( makeMDHistoDimensionFromString( getPropertyValue("DimZ")) );
    dimensions.push_back( makeMDHistoDimensionFromString( getPropertyValue("DimT")) );

    IMDEventWorkspace_sptr in_ws = getProperty("InputWorkspace");

    // De serialize the implicit function
    std::string ImplicitFunctionXML = getPropertyValue("ImplicitFunctionXML");
    implicitFunc = NULL;
    if (!ImplicitFunctionXML.empty())
    {
      implicitFunc = Mantid::API::ImplicitFunctionFactory::Instance().createUnwrapped(ImplicitFunctionXML);
    }

    prog = new Progress(this, 0, 1.0, 1); // This gets deleted by the thread pool; don't delete it in here.

    // Wrapper to cast to MDEventWorkspace then call the function
    CALL_MDEVENT_FUNCTION(this->do_centerpointBin, in_ws);

    // Save the output
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(out));

  }



} // namespace Mantid
} // namespace MDEvents

