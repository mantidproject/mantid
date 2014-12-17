#include "MantidDataHandling/PDLoadCharacterizations.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/Strings.h"
#include <boost/algorithm/string.hpp>
#include <fstream>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PDLoadCharacterizations)

/// key for a instrument parameter file being listed
static const std::string IPARM_KEY("Instrument parameter file:");
static const std::string L1_KEY("L1");
static const std::string ZERO("0.");

//----------------------------------------------------------------------------------------------
/** Constructor
 */
PDLoadCharacterizations::PDLoadCharacterizations() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
PDLoadCharacterizations::~PDLoadCharacterizations() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string PDLoadCharacterizations::name() const {
  return "PDLoadCharacterizations";
};

/// Algorithm's version for identification. @see Algorithm::version
int PDLoadCharacterizations::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string PDLoadCharacterizations::category() const {
  return "Workflow\\DataHandling";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void PDLoadCharacterizations::init() {
  std::vector<std::string> exts;
  exts.push_back(".txt");
  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
                  "Characterizations file");

  declareProperty(new WorkspaceProperty<ITableWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "Output for the information of characterizations and runs");

  declareProperty("IParmFilename", std::string(""),
                  "Name of the gsas instrument parameter file.",
                  Direction::Output);
  declareProperty("PrimaryFlightPath", EMPTY_DBL(),
                  "Primary flight path L1 of the powder diffractomer. ",
                  Direction::Output);
  declareProperty(new ArrayProperty<int32_t>("SpectrumIDs", Direction::Output),
                  "Spectrum IDs (note that it is not detector ID or workspace "
                  "indices). The list must be either empty or have a size "
                  "equal to input workspace's histogram number. ");
  declareProperty(new ArrayProperty<double>("L2", Direction::Output),
                  "Seconary flight (L2) paths for each detector.  Number of L2 "
                  "given must be same as number of histogram.");
  declareProperty(new ArrayProperty<double>("Polar", Direction::Output),
                  "Polar angles (two thetas) for detectors. Number of 2theta "
                  "given must be same as number of histogram.");
  declareProperty(
      new ArrayProperty<double>("Azimuthal", Direction::Output),
      "Azimuthal angles (out-of-plane) for detectors. "
      "Number of azimuthal angles given must be same as number of histogram.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void PDLoadCharacterizations::exec() {
  // open the file for reading
  std::string filename = this->getProperty("Filename");
  std::ifstream file(filename.c_str());
  if (!file) {
    throw Exception::FileError("Unable to open file", filename);
  }

  // read the first line and decide what to do
  std::string firstLine = Strings::getLine(file);
  if (firstLine.substr(0, IPARM_KEY.size()) == IPARM_KEY) {
    firstLine = Strings::strip(firstLine.substr(IPARM_KEY.size()));
    this->setProperty("IParmFilename", firstLine);
    this->readFocusInfo(file);
  } else {
    // things expect the L1 to be zero if it isn't set
    this->setProperty("PrimaryFlightPath", 0.);
  }

  // now the rest of the file
  // setup the default table workspace for the characterization runs
  ITableWorkspace_sptr wksp = WorkspaceFactory::Instance().createTable();
  wksp->addColumn("double", "frequency");
  wksp->addColumn("double", "wavelength");
  wksp->addColumn("int", "bank");
  wksp->addColumn("int", "vanadium");
  wksp->addColumn("int", "container");
  wksp->addColumn("int", "empty");
  wksp->addColumn("str", "d_min"); // b/c it is an array for NOMAD
  wksp->addColumn("str", "d_max"); // b/c it is an array for NOMAD
  wksp->addColumn("double", "tof_min");
  wksp->addColumn("double", "tof_max");
  this->readCharInfo(file, wksp);
  this->setProperty("OutputWorkspace", wksp);
}

/**
 * Parse the stream for the focus positions and instrument parameter filename.
 *
 * @param file The stream to parse.
 */
void PDLoadCharacterizations::readFocusInfo(std::ifstream &file) {
  // end early if already at the end of the file
  if (file.eof())
    return;

  std::vector<int32_t> specIds;
  std::vector<double> l2;
  std::vector<double> polar;

  // parse the file
  for (std::string line = Strings::getLine(file); !file.eof();
       line = Strings::getLine(file)) {
    line = Strings::strip(line);
    // skip empty lines and "comments"
    if (line.empty())
      continue;
    if (line.substr(0, 1) == "#")
      continue;

    std::vector<std::string> splitted;
    boost::split(splitted, line, boost::is_any_of("\t "),
                 boost::token_compress_on);
    if (splitted[0] == L1_KEY) {
      this->setProperty("PrimaryFlightPath",
                        boost::lexical_cast<double>(splitted[1]));
      break;
    } else if (splitted.size() >= 3) // specid, L2, theta
    {
      specIds.push_back(boost::lexical_cast<int32_t>(splitted[0]));
      l2.push_back(boost::lexical_cast<double>(splitted[1]));
      polar.push_back(boost::lexical_cast<double>(splitted[2]));
    }
  }
  if (specIds.size() != l2.size() || specIds.size() != polar.size())
    throw std::runtime_error(
        "Found different number of spectra, L2 and polar angles");

  // azimuthal angles are all zero
  std::vector<double> azi(polar.size(), 0.);

  // set the values
  this->setProperty("SpectrumIDs", specIds);
  this->setProperty("L2", l2);
  this->setProperty("Polar", polar);
  this->setProperty("Azimuthal", azi);
}

/**
 * Parse the stream for the characterization file information.
 *
 * @param file The stream to parse.
 * @param wksp The table workspace to fill in.
 */
void PDLoadCharacterizations::readCharInfo(std::ifstream &file,
                                           ITableWorkspace_sptr &wksp) {
  // end early if already at the end of the file
  if (file.eof())
    return;

  // parse the file
  for (std::string line = Strings::getLine(file); !file.eof();
       line = Strings::getLine(file)) {
    line = Strings::strip(line);

    // skip empty lines and "comments"
    if (line.empty())
      continue;
    if (line.substr(0, 1) == "#")
      continue;

    // parse the line
    std::vector<std::string> splitted;
    boost::split(splitted, line, boost::is_any_of("\t "),
                 boost::token_compress_on);
    while (splitted.size() < 10)
      splitted.push_back(ZERO); // extra values default to zero

    // add the row
    API::TableRow row = wksp->appendRow();
    row << boost::lexical_cast<double>(splitted[0]);  // frequency
    row << boost::lexical_cast<double>(splitted[1]);  // wavelength
    row << boost::lexical_cast<int32_t>(splitted[2]); // bank
    row << boost::lexical_cast<int32_t>(splitted[3]); // vanadium
    row << boost::lexical_cast<int32_t>(splitted[4]); // container
    row << boost::lexical_cast<int32_t>(splitted[5]); // empty
    row << splitted[6];                               // d_min
    row << splitted[7];                               // d_max
    row << boost::lexical_cast<double>(splitted[8]);  // tof_min
    row << boost::lexical_cast<double>(splitted[9]);  // tof_max
  }
}

} // namespace DataHandling
} // namespace Mantid
