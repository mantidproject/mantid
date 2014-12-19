#include "MantidMDEvents/BoxControllerSettingsAlgorithm.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace MDEvents {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
BoxControllerSettingsAlgorithm::BoxControllerSettingsAlgorithm() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
BoxControllerSettingsAlgorithm::~BoxControllerSettingsAlgorithm() {}

//----------------------------------------------------------------------------------------------
/** Add Box-controller-specific properties to this algorithm
 *
 * @param SplitInto :: default parameter value
 * @param SplitThreshold :: default parameter value
 * @param MaxRecursionDepth :: default parameter value
 */
void BoxControllerSettingsAlgorithm::initBoxControllerProps(
    const std::string &SplitInto, int SplitThreshold, int MaxRecursionDepth) {
  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  auto mustBeMoreThen1 = boost::make_shared<BoundedValidator<int>>();
  mustBeMoreThen1->setLower(1);

  // Split up comma-separated properties
  std::vector<int> value;
  typedef Poco::StringTokenizer tokenizer;
  tokenizer values(SplitInto, ",",
                   tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
  value.clear();
  value.reserve(values.count());
  for (tokenizer::Iterator it = values.begin(); it != values.end(); ++it)
    value.push_back(boost::lexical_cast<int>(*it));

  declareProperty(
      new ArrayProperty<int>("SplitInto", value),
      "A comma separated list of into how many sub-grid elements each "
      "dimension should split; "
      "or just one to split into the same number for all dimensions. Default " +
          SplitInto + ".");

  declareProperty(
      new PropertyWithValue<int>("SplitThreshold", SplitThreshold,
                                 mustBePositive),
      "How many events in a box before it should be split. Default " +
          Strings::toString(SplitThreshold) + ".");

  declareProperty(new PropertyWithValue<int>(
                      "MaxRecursionDepth", MaxRecursionDepth, mustBeMoreThen1),
                  "How many levels of box splitting recursion are allowed. "
                  "The smallest box will have each side length :math:`l = "
                  "(extents) / (SplitInto^{MaxRecursionDepth}).` "
                  "Default " +
                      Strings::toString(MaxRecursionDepth) + ".");

  std::string grp = getBoxSettingsGroupName();
  setPropertyGroup("SplitInto", grp);
  setPropertyGroup("SplitThreshold", grp);
  setPropertyGroup("MaxRecursionDepth", grp);
}

/**
 * Looks at each of the parameters, to see if they are default, and if they are,
 * over-writes them
 * with the values set in the instrument parameters (if they exist).
 * @param instrument : Instrument on input workspace.
 * @param ndims : Number of dimensions in output workspace.
 */
void BoxControllerSettingsAlgorithm::takeDefaultsFromInstrument(
    Mantid::Geometry::Instrument_const_sptr instrument, const size_t ndims) {
  const std::string splitThresholdName = "SplitThreshold";
  const std::string splitIntoName = "SplitInto";
  const std::string maxRecursionDepthName = "MaxRecursionDepth";
  Property *p = getProperty(splitThresholdName);
  if (p->isDefault()) {
    std::vector<double> instrumentSplitThresholds =
        instrument->getNumberParameter(splitThresholdName, true);
    if (!instrumentSplitThresholds.empty()) {
      setProperty(splitThresholdName,
                  static_cast<int>(instrumentSplitThresholds.front()));
    }
  }
  p = getProperty(splitIntoName);
  if (p->isDefault()) {
    std::vector<double> instrumentSplitInto =
        instrument->getNumberParameter(splitIntoName, true);
    if (!instrumentSplitInto.empty()) {
      const int splitInto = static_cast<int>(instrumentSplitInto.front());
      std::vector<int> newSplitInto(ndims, splitInto);
      setProperty(splitIntoName, newSplitInto);
    }
  }
  p = getProperty(maxRecursionDepthName);
  if (p->isDefault()) {
    std::vector<double> instrumentMaxRecursionDepth =
        instrument->getNumberParameter(maxRecursionDepthName, true);
    if (!instrumentMaxRecursionDepth.empty()) {
      setProperty(maxRecursionDepthName,
                  static_cast<int>(instrumentMaxRecursionDepth.front()));
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Set the settings in the given box controller
 * This should only be called immediately after creating the workspace
 *
 * @param bc :: box controller to modify
 * @param instrument :: instrument to read parameters from.
 */
void BoxControllerSettingsAlgorithm::setBoxController(
    BoxController_sptr bc, Mantid::Geometry::Instrument_const_sptr instrument) {
  size_t nd = bc->getNDims();

  takeDefaultsFromInstrument(instrument, nd);

  setBoxController(bc);
}

//----------------------------------------------------------------------------------------------
/** Set the settings in the given box controller
 * This should only be called immediately after creating the workspace
 *
 * @param bc :: box controller to modify
 */
void BoxControllerSettingsAlgorithm::setBoxController(BoxController_sptr bc) {
  size_t nd = bc->getNDims();

  int val;
  val = this->getProperty("SplitThreshold");
  bc->setSplitThreshold(val);
  val = this->getProperty("MaxRecursionDepth");
  bc->setMaxDepth(val);

  // Build MDGridBox
  std::vector<int> splits = getProperty("SplitInto");
  if (splits.size() == 1) {
    bc->setSplitInto(splits[0]);
  } else if (splits.size() == nd) {
    for (size_t d = 0; d < nd; ++d)
      bc->setSplitInto(d, splits[d]);
  } else
    throw std::invalid_argument("SplitInto parameter has " +
                                Strings::toString(splits.size()) +
                                " arguments. It should have either 1, or the "
                                "same as the number of dimensions.");
  bc->resetNumBoxes();
}

} // namespace Mantid
} // namespace MDEvents
