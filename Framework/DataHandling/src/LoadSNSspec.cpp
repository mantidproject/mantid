// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadSNSspec.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/UnitFactory.h"

#include <cstring>
#include <fstream>

namespace Mantid {
namespace DataHandling {
DECLARE_FILELOADER_ALGORITHM(LoadSNSspec)

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadSNSspec::confidence(Kernel::FileDescriptor &descriptor) const {
  if (!descriptor.isAscii())
    return 0;

  auto &file = descriptor.data();

  int confidence(0);
  size_t axiscols(0), datacols(0);
  std::string str;
  using tokenizer = Mantid::Kernel::StringTokenizer;
  const std::string sep = " ";
  bool snsspec(false);

  while (std::getline(file, str)) {
    // File is opened in binary mode so getline will leave a \r at the end of an
    // empty line if it exists
    if (str.empty() || str == "\r")
      continue;

    try {
      // if it's comment line
      tokenizer tok(str, sep,
                    Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
      if (str.at(0) == '#') {
        if (str.at(1) == 'L') {
          axiscols = tok.count();
          // if the file contains a comment line starting with "#L" followed
          // by three columns this could be loadsnsspec file
          if (axiscols > 2) {
            snsspec = true;
          }
        }
      } else {
        // check first data line is a 3 column line
        datacols = tok.count();
        break;
      }
    } catch (std::out_of_range &) {
    }
  }
  if (snsspec && datacols == 3) // three column data
  {
    confidence = 80;
  }
  return confidence;
}

using namespace Kernel;
using namespace API;

/// Empty constructor
LoadSNSspec::LoadSNSspec() : API::DeprecatedAlgorithm() {
  useAlgorithm("LoadSpec");
  deprecatedDate("2017-01-30");
}

/// Initialisation method.
void LoadSNSspec::init() {
  const std::vector<std::string> exts{".dat", ".txt"};
  declareProperty(
      std::make_unique<FileProperty>("Filename", "", FileProperty::Load, exts),
      "The name of the text file to read, including its full or "
      "relative path. The file extension must be .txt or .dat.");
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                            Direction::Output),
      "The name of the workspace that will be created, filled with the read-in "
      "data and stored in the [[Analysis Data Service]].");

  std::vector<std::string> units = UnitFactory::Instance().getKeys();
  declareProperty("Unit", "Energy",
                  boost::make_shared<Kernel::StringListValidator>(units),
                  "The unit to assign to the X axis (anything known to the "
                  "[[Unit Factory]] or \"Dimensionless\") (default: Energy)");
}

void LoadSNSspec::exec() {
  auto alg = createChildAlgorithm("LoadSpec");
  alg->setPropertyValue("Filename", getPropertyValue("Filename"));
  alg->setPropertyValue("OutputWorkspace", getPropertyValue("OutputWorkspace"));
  alg->setPropertyValue("unit", getPropertyValue("unit"));
  alg->execute();

  MatrixWorkspace_sptr ws = alg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", ws);
}

} // namespace DataHandling
} // namespace Mantid
