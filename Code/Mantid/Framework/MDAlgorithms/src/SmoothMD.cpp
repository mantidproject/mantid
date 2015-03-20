#include "MantidMDAlgorithms/SmoothMD.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidMDEvents/MDHistoWorkspaceIterator.h"
#include <boost/make_shared.hpp>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <utility>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/tuple/tuple.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::MDEvents;

typedef std::vector<int> WidthVector;
typedef boost::function<IMDHistoWorkspace_sptr(
    IMDHistoWorkspace_const_sptr, const WidthVector &)> SmoothFunction;
typedef std::map<std::string, SmoothFunction> SmoothFunctionMap;

namespace {

/**
 * @brief functions
 * @return Allowed smoothing functions
 */
std::vector<std::string> functions() {
  std::vector<std::string> propOptions;
  propOptions.push_back("Hat");
  //propOptions.push_back("Gaussian");
  return propOptions;
}

IMDHistoWorkspace_sptr hatSmooth(IMDHistoWorkspace_const_sptr toSmooth,
                                 const WidthVector &widthVector) {
  IMDHistoWorkspace_sptr outWS = toSmooth->clone();
  boost::scoped_ptr<MDHistoWorkspaceIterator> iterator(
      dynamic_cast<MDHistoWorkspaceIterator *>(
          toSmooth->createIterator(NULL))); // TODO should be multi-threaded
  do {
    // Gets all vertex-touching neighbours

    std::vector<size_t> neighbourIndexes =
        iterator->findNeighbourIndexesByWidth(
            widthVector.front()); // TODO we should use the whole width vector
                                  // not just the first element.
    const size_t nNeighbours = neighbourIndexes.size();
    double sumSignal = iterator->getSignal();
    double sumSqError = iterator->getError();
    for (size_t i = 0; i < neighbourIndexes.size(); ++i) {
      sumSignal += toSmooth->getSignalAt(neighbourIndexes[i]);
      double error = toSmooth->getErrorAt(neighbourIndexes[i]);
      sumSqError += (error * error);
    }

    // Calculate the mean
    outWS->setSignalAt(iterator->getLinearIndex(),
                       sumSignal / (nNeighbours + 1));
    // Calculate the sample variance
    outWS->setErrorSquaredAt(iterator->getLinearIndex(),
                             sumSqError / (nNeighbours + 1));
  } while (iterator->next());
  return outWS;
}

SmoothFunctionMap makeFunctionMap() {
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
  auto boundedValidator =
      boost::make_shared<ArrayBoundedValidator<int>>(1, 100);
  widthVectorValidator->add(boundedValidator);
  widthVectorValidator->add(
      boost::make_shared<MandatoryValidator<std::vector<int>>>());

  declareProperty(new ArrayProperty<int>("WidthVector", widthVectorValidator,
                                         Direction::Input),
                  "Width vector. Either specify the width in n-pixels for each "
                  "dimension, or provide a single entry (n-pixels) for all "
                  "dimensions.");

  const auto allFunctionTypes = functions();
  const std::string first = allFunctionTypes.front();

  std::stringstream docBuffer;
  docBuffer << "Smoothing function. Defaults to " << first;
  declareProperty(
      new PropertyWithValue<std::string>(
          "Function", first,
          boost::make_shared<ListValidator<std::string>>(allFunctionTypes),
          Direction::Input),
      docBuffer.str());

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

/**
 * validateInputs
 * @return map of property names to errors.
 */
std::map<std::string, std::string> SmoothMD::validateInputs() {

  std::map<std::string, std::string> product;

  const std::string widthVectorPropertyName = "WidthVector";
  std::vector<int> widthVector = this->getProperty(widthVectorPropertyName);
  for (auto it = widthVector.begin(); it != widthVector.end(); ++it) {
    const int widthEntry = *it;
    if (widthEntry % 2 == 0) {
      std::stringstream message;
      message << widthVectorPropertyName
              << " entries must be odd numbers. Bad entry is " << widthEntry;
      product.insert(std::make_pair(widthVectorPropertyName, message.str()));
    }
  }
  return product;
}

} // namespace MDAlgorithms
} // namespace Mantid
