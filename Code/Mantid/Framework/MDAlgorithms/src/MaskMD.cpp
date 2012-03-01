/*WIKI*

This algorithm masks a [[MDWorkspace]] in-situ.

This algorithm will first clear-out any existing masking and then apply the new masking.

== Simple Example ==
Mask as single box region in a 3D workspace with Dimension ids X, Y, Z. Suppose that the dimensions exented from -2 to 2 in each dimension and you want to mask the central region.

 MaskMD("Workspace"=workspace,Dimensions="X,Y,Z",Exents="-1,1,-1,1,-1,1")

== Complex Example ==

Mask two box regions in a 3D workspace, where the input workspace is the same as above. Here we attempt to mask two opposite corners of the 3D workspace.

 MaskMD("Workspace"=workspace,Dimensions="X,Y,Z,X,Y,Z",Extents="-2,-1,-2,-1,-2,-1,+1,+2,+1,+2,+1,+2")

In this example, because the dimensionality is 3 and because 6 dimension ids have been provided, the algorithm treats {X,Y,Z} as one masking region and the
following {X,Y,Z} as another. Likewise of the 12, Extents inputs provided, the first 6 entries {-2,-1,-2,-1,-2,-1} are min, max values for the first {X,Y,Z} and the
latter 6 {+1,+2,+1,+2,+1,+2} relate to the last {X,Y,Z}. Applying this maksing will result in two completely separate areas masked in a single call to the algorithm.

*WIKI*/

#include "MantidMDAlgorithms/MaskMD.h"
#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ArrayProperty.h"
#include <algorithm>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace MDAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(MaskMD)

  /// Local type to group min, max extents with a dimension index.
  struct InputArgument
  {
    double min, max;
    size_t index;
  };

  /// Comparitor to allow sorting by dimension index.
  struct LessThanIndex : std::binary_function<InputArgument&, InputArgument&, bool>
  {
    bool operator() (InputArgument& a , InputArgument& b) const
    {
      return a.index < b.index;
    }
  };


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MaskMD::MaskMD()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MaskMD::~MaskMD()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string MaskMD::name() const { return "MaskMD";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int MaskMD::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string MaskMD::category() const { return "MDAlgorithms";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void MaskMD::initDocs()
  {
    this->setWikiSummary("Mask an [[MDWorkspace]] in-situ");
    this->setOptionalMessage("Mask an MDWorkspace in-situ marking specified boxes as masked");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void MaskMD::init()
  {
    declareProperty(new PropertyWithValue<bool>("ClearExistingMasks", "1", Direction::Input));
    declareProperty(new WorkspaceProperty<IMDWorkspace>("Workspace","",Direction::InOut), "An input workspace.");
    declareProperty(new ArrayProperty<std::string>("Dimensions",new MandatoryValidator<std::vector<std::string> >,Direction::Input),
          "Dimension ids/names all comma separated.\n"
          "According to the dimensionality of the workspace, these names will be grouped,\n"
          "so the number of entries must be n*(number of dimensions in the workspace)."
          );

    declareProperty(new ArrayProperty<double>("Extents",new MandatoryValidator<std::vector<double> >,Direction::Input),
      "Extents {min, max} corresponding to each of the dimensions specified, according to the order those identifies have been specified." );
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void MaskMD::exec()
  {
    IMDWorkspace_sptr ws = getProperty("Workspace");
    std::vector<std::string> dimensions = getProperty("Dimensions");
    std::vector<double> extents = getProperty("Extents");

    size_t nDims = ws->getNumDims();
    size_t nDimensionIds = dimensions.size();
    
    std::stringstream messageStream;

    // Check cardinality on names/ids
    if(nDimensionIds%nDims != 0)
    {
      messageStream << "Number of dimension ids/names must be n * " << nDims;
      this->g_log.error(messageStream.str());
      throw std::invalid_argument(messageStream.str());
    }

    // Check cardinality on extents
    if(extents.size() != (2*dimensions.size()))
    {
      messageStream << "Number of extents must be " << 2 * dimensions.size();
      this->g_log.error(messageStream.str());
      throw std::invalid_argument(messageStream.str());
    }

    // Check extent value provided.
    for(size_t i = 0; i < nDimensionIds; ++i)
    {
      double min = extents[i*2];
      double max = extents[(i*2)+1];
      if(min > max)
      {
        messageStream << "Cannot have minimum extents " << min << " larger than maximum extents " << max;
        this->g_log.error(messageStream.str());
        throw std::invalid_argument(messageStream.str());
      }
    }

    size_t nGroups = nDimensionIds/nDims;

    bool bClearExistingMasks = getProperty("ClearExistingMasks");
    if(bClearExistingMasks)
    {
      ws->clearMDMasking();
    }

    //Loop over all groups
    for(size_t group = 0; group < nGroups; ++group)
    {
      std::vector<InputArgument> arguments(nDims);

      //Loop over all arguments within the group. and construct InputArguments for sorting.
      for(size_t i = 0; i < nDims; ++i)
      {
        size_t index = i + group;
        InputArgument& arg = arguments[i];
        try
        {
          arg.index = ws->getDimensionIndexById(dimensions[index]);
        }
        catch(std::runtime_error)
        {
          arg.index = ws->getDimensionIndexByName(dimensions[index]);
        }
        arg.min = extents[index*2];
        arg.max = extents[(index*2)+1];
      }

      // Sort all the inputs by the dimension index. Without this it will not be possible to construct the MDImplicit function propertly.
      LessThanIndex comparitor;
      std::sort(arguments.begin(), arguments.end(), comparitor);

      //Create inputs for a box implicit function
      VMD mins(nDims);
      VMD maxs(nDims);
      for(size_t i = 0; i < nDims; ++i)
      {
        mins[i] = arguments[i].min;
        maxs[i] = arguments[i].max;
      }

      //Add new masking.
      ws->setMDMasking(new MDBoxImplicitFunction(mins, maxs));
    }
  }



} // namespace Mantid
} // namespace MDAlgorithms