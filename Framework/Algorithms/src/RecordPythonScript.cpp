// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/RecordPythonScript.h"
#include "MantidAPI/FileProperty.h"

#include <Poco/Thread.h>

#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RecordPythonScript)

//----------------------------------------------------------------------------------------------
/// Constructor
RecordPythonScript::RecordPythonScript()
    : Algorithms::GeneratePythonScript(), API::AlgorithmObserver() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void RecordPythonScript::init() {

  declareProperty(std::make_unique<API::FileProperty>(
                      "Filename", "", API::FileProperty::Save, ".py"),
                  "The file into which the Python script will be generated.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void RecordPythonScript::exec() {
  observeStarting();
  // Keep going until you get cancelled
  while (true) {
    try {
      // Exit if the user presses cancel
      interruption_point();
    } catch (...) {
      break;
    }
    progress(0.0, "Recording...");

    // Sleep for 50 msec
    Poco::Thread::sleep(50);
  }

  // save the script to a file
  const std::string filename = getPropertyValue("Filename");
  std::ofstream file(filename.c_str(), std::ofstream::trunc);

  if (file.is_open()) {
    file << m_generatedScript;
    file.flush();
    file.close();
  } else {
    throw Exception::FileError("Unable to create file: ", filename);
  }

  stopObservingManager();
}

/** Handler of the start notifications. Adds an algorithm call to the script.
 * @param alg :: Shared pointer to the starting algorithm.
 */
void RecordPythonScript::startingHandle(API::IAlgorithm_sptr alg) {
  auto props = alg->getProperties();

  std::string algString;
  for (auto &prop : props) {
    std::string opener = "='";
    if ((*prop).value().find('\\') != std::string::npos) {
      opener = "=r'";
    }

    std::string paramString = (*prop).name() + opener + (*prop).value() + "'";

    // Miss out parameters that are empty.
    if (paramString.length() != 0) {
      if (algString.length() != 0) {
        algString += ",";
      }
      algString += paramString;
    }
  }

  m_generatedScript += alg->name() + "(" + algString + ")\n";
}

} // namespace Algorithms
} // namespace Mantid
