// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/ImportMDHistoWorkspace.h"
#include "MantidAPI/FileProperty.h"

#include <deque>
#include <fstream>
#include <iterator>

namespace Mantid {
namespace MDAlgorithms {

using namespace API;
using namespace DataObjects;
using namespace Kernel;
using namespace Geometry;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ImportMDHistoWorkspace)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string ImportMDHistoWorkspace::name() const {
  return "ImportMDHistoWorkspace";
}

/// Algorithm's version for identification. @see Algorithm::version
int ImportMDHistoWorkspace::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ImportMDHistoWorkspace::category() const {
  return "MDAlgorithms\\DataHandling";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ImportMDHistoWorkspace::init() {
  std::vector<std::string> fileExtensions{".txt"};
  declareProperty(std::make_unique<API::FileProperty>(
                      "Filename", "", API::FileProperty::Load, fileExtensions),
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
  using box_collection = std::deque<std::string>;
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
  for (auto it = box_elements.begin(); it != box_elements.end(); it += 2) {
    auto temp = it;
    double signal = std::stod(*(temp));
    double error = std::stod(*(++temp));
    signals[currentBox] = signal;
    errors[currentBox] = error * error;
    ++currentBox;
  }

  // Set the output.
  setProperty("OutputWorkspace", ws);
}

} // namespace MDAlgorithms
} // namespace Mantid
