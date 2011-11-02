#include "MantidMDEvents/BoxControllerSettingsAlgorithm.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ArrayProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  BoxControllerSettingsAlgorithm::BoxControllerSettingsAlgorithm()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  BoxControllerSettingsAlgorithm::~BoxControllerSettingsAlgorithm()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Box-controller-specific properties
  void BoxControllerSettingsAlgorithm::initBoxControllerProps()
  {

    declareProperty(
      new ArrayProperty<int>("SplitInto", "5"),
      "A comma separated list of into how many sub-grid elements each dimension should split; \n"
      "or just one to split into the same number for all dimensions. Default 5.");

    declareProperty(
      new PropertyWithValue<int>("SplitThreshold", 1000),
      "How many events in a box before it should be split. Default 1000.");

    declareProperty(
      new PropertyWithValue<int>("MaxRecursionDepth", 5),
      "How many levels of box splitting recursion are allowed. \n"
      "The smallest box will have each side length l = (extents) / (SplitInto ^ MaxRecursionDepth). Default 10.");

    std::string grp("Box Splitting Settings");
    setPropertyGroup("SplitInto", grp);
    setPropertyGroup("SplitThreshold", grp);
    //setPropertyGroup("MinRecursionDepth", grp);
    setPropertyGroup("MaxRecursionDepth", grp);

  }


  //----------------------------------------------------------------------------------------------
  /** Set the settings in the given box controller
   * This should only be called immediately after creating the workspace
   *
   * @param bc :: box controller to modify
   */
  void BoxControllerSettingsAlgorithm::setBoxController(BoxController_sptr bc)
  {
    size_t nd = bc->getNDims();

    int val;
    val = this->getProperty("SplitThreshold");
    bc->setSplitThreshold( val );
    val = this->getProperty("MaxRecursionDepth");
    bc->setMaxDepth( val );

    // Build MDGridBox
    std::vector<int> splits = getProperty("SplitInto");
    if (splits.size() == 1)
    {
      bc->setSplitInto(splits[0]);
    }
    else if (splits.size() == nd)
    {
      for (size_t d=0; d<nd; ++d)
        bc->setSplitInto(d, splits[0]);
    }
    else
      throw std::invalid_argument("SplitInto parameter must either have 1 argument, or the same number as the number of dimensions.");
    bc->resetNumBoxes();

  }




} // namespace Mantid
} // namespace MDEvents

