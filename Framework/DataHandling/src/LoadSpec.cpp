// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadSpec.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/UnitFactory.h"

#include <cstring>
#include <fstream>

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadSpec)

using namespace Kernel;
using namespace API;

/// Empty constructor
LoadSpec::LoadSpec() {}

/// Initialisation method.
void LoadSpec::init() {
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
  units.insert(units.begin(), "MomemtumTransfer");
  declareProperty("Unit", "Energy",
                  boost::make_shared<Kernel::StringListValidator>(units),
                  "The unit to assign to the X axis (anything known to the "
                  "[[Unit Factory]] or \"Dimensionless\")");
}

/**
 *   Executes the algorithm.
 */
void LoadSpec::exec() {
  std::string filename = getProperty("Filename");
  std::ifstream file(filename.c_str());

  file.seekg(0, std::ios::end);
  Progress progress(this, 0.0, 1.0, static_cast<int>(file.tellg()));
  file.seekg(0, std::ios::beg);

  std::string str;
  std::vector<double> input;

  const size_t nSpectra = readNumberOfSpectra(file);
  auto localWorkspace = boost::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceFactory::Instance().create("Workspace2D", nSpectra, 2, 1));

  localWorkspace->getAxis(0)->unit() =
      UnitFactory::Instance().create(getProperty("Unit"));

  file.clear(); // end of file has been reached so we need to clear file state
  file.seekg(0, std::ios::beg); // go back to beginning of file

  int specNum = -1; // spectrum number
  while (getline(file, str)) {
    progress.report(static_cast<int>(file.tellg()));

    // line with data, need to be parsed by white spaces
    readLine(str, input);

    if (str.empty() && specNum != -1) {
      auto histogram = localWorkspace->histogram(specNum);
      readHistogram(input, histogram);
      localWorkspace->setHistogram(specNum, histogram);
    }

    if (str.empty()) {
      specNum++;
      input.clear();
    }

  } // end of read file

  try {
    if (nSpectra == 0)
      throw "Undefined number of spectra";

    if (static_cast<size_t>(specNum) == (nSpectra - 1)) {
      auto histogram = localWorkspace->histogram(specNum);
      readHistogram(input, histogram);
      localWorkspace->setHistogram(specNum, histogram);
    }
  } catch (...) {
  }

  setProperty("OutputWorkspace", localWorkspace);
}

/**
 * Read the total number of specrta contained in the file
 *
 * @param file :: file stream to read from
 * @return a size_t representing the number of spectra
 */
size_t LoadSpec::readNumberOfSpectra(std::ifstream &file) const {
  // determine the number of lines starting by #L
  // as there is one per set of data
  size_t spectra_nbr = 0;
  std::string str;
  while (getline(file, str)) {
    if (str.empty())
      continue;
    if (str[0] == '#' && str[1] == 'L') {
      spectra_nbr++;
    }
  }
  return spectra_nbr;
}

/**
 * Read a single line from the file into the data buffer
 *
 * @param line :: the current line in the file to process
 * @param buffer :: the buffer to store loaded data in
 */
void LoadSpec::readLine(const std::string &line,
                        std::vector<double> &buffer) const {
  if (!line.empty() && line[0] != '#') {
    using tokenizer = Mantid::Kernel::StringTokenizer;
    const std::string sep = " ";
    tokenizer tok(line, sep, Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
    buffer.reserve(buffer.size() + tok.size());
    for (const auto &beg : tok) {
      buffer.emplace_back(std::stod(beg));
    }
  }
}

/**
 * Convert the data from the input buffer to a histogram object
 *
 * @param input :: the input buffer containing the raw data
 * @param histogram :: the histogram object to fill with values
 * */
void LoadSpec::readHistogram(const std::vector<double> &input,
                             HistogramData::Histogram &histogram) const {
  std::vector<double> x, y, e;

  auto isHist = input.size() % 3 > 0;
  auto nElements = (isHist) ? input.size() - 1 : input.size() - 2;

  x.reserve(nElements);
  y.reserve(nElements);
  e.reserve(nElements);

  for (size_t index = 0; index < nElements; index++) {
    x.push_back(input[index]);
    index++;
    y.push_back(input[index]);
    index++;
    e.push_back(input[index]);
  }

  histogram.resize(y.size());

  if (isHist) {
    // we're loading binned data
    // last value is final x bin
    x.push_back(input.back());
    histogram.setBinEdges(x);
  } else {
    histogram.setPoints(x);
  }

  histogram.setCounts(y);
  histogram.setCountStandardDeviations(e);
}

} // namespace DataHandling
} // namespace Mantid
