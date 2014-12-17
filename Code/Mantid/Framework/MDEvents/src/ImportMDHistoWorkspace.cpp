#include "MantidMDEvents/ImportMDHistoWorkspace.h"
#include "MantidAPI/FileProperty.h"

#include <deque>
#include <iostream>
#include <fstream>
#include <iterator>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid {
namespace MDEvents {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ImportMDHistoWorkspace)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ImportMDHistoWorkspace::ImportMDHistoWorkspace()
    : ImportMDHistoWorkspaceBase() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ImportMDHistoWorkspace::~ImportMDHistoWorkspace() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string ImportMDHistoWorkspace::name() const {
  return "ImportMDHistoWorkspace";
};

/// Algorithm's version for identification. @see Algorithm::version
int ImportMDHistoWorkspace::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string ImportMDHistoWorkspace::category() const {
  return "MDAlgorithms";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ImportMDHistoWorkspace::init() {
  std::vector<std::string> fileExtensions(1);
  fileExtensions[0] = ".txt";
  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load,
                                        fileExtensions),
                  "File of type txt");

  // Initialize generic dimension properties on the base class.
  this->initGenericImportProps();
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ImportMDHistoWorkspace::exec() {

  std::string filename = getProperty("Filename");

  /*
   Base class creates an empty output workspace, with the correct dimensionality
   according to the algorithm inputs (see base class).
  */
  MDHistoWorkspace_sptr ws = this->createEmptyOutputWorkspace();

  // Open the file
  std::ifstream file;
  try {
    file.open(filename.c_str(), std::ios::in);
  } catch (std::ifstream::failure &e) {
    g_log.error() << "Cannot open file: " << filename;
    throw(e);
  }

  // Copy each string present in the file stream into a deque.
  typedef std::deque<std::string> box_collection;
  box_collection box_elements;
  std::copy(std::istream_iterator<std::string>(file),
            std::istream_iterator<std::string>(),
            std::back_inserter(box_elements));

  //// Release the resource.
  file.close();

  const size_t nElements = this->getBinProduct() * 2;

  // Handle the case that the number of elements is wrong.
  if (box_elements.size() != nElements) {
    throw std::invalid_argument("The number of data entries in the file, does "
                                "not match up with the specified "
                                "dimensionality.");
  }

  // Fetch out raw pointers to workspace arrays.
  double *signals = ws->getSignalArray();
  double *errors = ws->getErrorSquaredArray();

  // Write to the signal and error array from the deque.
  size_t currentBox = 0;
  for (box_collection::iterator it = box_elements.begin();
       it != box_elements.end(); it += 2) {
    box_collection::iterator temp = it;
    double signal = atof((*(temp)).c_str());
    double error = atof((*(++temp)).c_str());
    signals[currentBox] = signal;
    errors[currentBox] = error * error;
    ++currentBox;
  }

  // Set the output.
  setProperty("OutputWorkspace", ws);
}

} // namespace Mantid
} // namespace MDEvents
