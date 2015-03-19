#include "MantidMDAlgorithms/SmoothMD.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/PropertyWithValue.h"
#include <boost/make_shared.hpp>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <utility>
#include <boost/function.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;

typedef std::vector<int> WidthVector;
typedef boost::function<IMDHistoWorkspace_sptr(IMDHistoWorkspace_sptr, WidthVector)> SmoothFunction;
typedef std::map<std::string, SmoothFunction> SmoothFunctionMap;

namespace {

/**
 * @brief functions
 * @return Allowed smoothing functions
 */
std::vector<std::string> functions() {
  std::vector<std::string> propOptions;
  propOptions.push_back("Hat");
  propOptions.push_back("Gaussian");
  return propOptions;
}

IMDHistoWorkspace_sptr hatSmooth(IMDHistoWorkspace_sptr toSmooth, WidthVector widthVector)
{
    return toSmooth->clone(); // TODO
}

SmoothFunctionMap makeFunctionMap()
{
    SmoothFunctionMap map;
    map.insert(std::make_pair("Hat", hatSmooth));
    return map;
}
}


namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SmoothMD)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SmoothMD::SmoothMD() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SmoothMD::~SmoothMD() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string SmoothMD::name() const { return "SmoothMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int SmoothMD::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string SmoothMD::category() const { return "MDAlgorithms"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SmoothMD::summary() const {
  return "Smooth an MDHistoWorkspace according to a weight function";
};

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SmoothMD::init() {
  declareProperty(new WorkspaceProperty<API::IMDHistoWorkspace>(
                      "InputWorkspace", "", Direction::Input),
                  "An input MDHistoWorkspace to smooth.");

  auto widthVectorValidator = boost::make_shared<CompositeValidator>();
  auto boundedValidator = boost::make_shared<ArrayBoundedValidator<int> >(1, 100);
  widthVectorValidator->add(boundedValidator);
  widthVectorValidator->add(boost::make_shared<MandatoryValidator<std::vector<int> > >());

  declareProperty(new ArrayProperty<int>(
      "WidthVector", widthVectorValidator
      , Direction::Input), "Width vector. Either specify the width in n-pixels for each dimension, or provide a single entry (n-pixels) for all dimensions." );

  const auto allFunctionTypes = functions();
  const std::string first = allFunctionTypes.front();

  std::stringstream docBuffer;
  docBuffer << "Smoothing function. Defaults to " << first;
  declareProperty(
      new PropertyWithValue<std::string>("Function", first, boost::make_shared<ListValidator<std::string> >(allFunctionTypes), Direction::Input), docBuffer.str());

      declareProperty(new WorkspaceProperty<API::IMDHistoWorkspace>(
                          "OutputWorkspace", "", Direction::Output),
                      "An output smoothed MDHistoWorkspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SmoothMD::exec() {

  // Get the input workspace to smooth
  IMDHistoWorkspace_sptr toSmooth = this->getProperty("InputWorkspace");
  // Get the width vector
  const std::vector<int> widthVector = this->getProperty("WidthVector");

  // Find the choosen smooth operation
  const std::string smoothFunctionName = this->getProperty("Function");
  SmoothFunctionMap functionMap = makeFunctionMap();
  SmoothFunction smoothFunction = functionMap[smoothFunctionName];
  // invoke the smoothing operation
  auto smoothed = smoothFunction(toSmooth, widthVector);

  setProperty("OutputWorkspace", smoothed);
}

} // namespace MDAlgorithms
} // namespace Mantid
