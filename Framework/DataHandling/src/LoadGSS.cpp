// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/LoadGSS.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/UnitFactory.h"

#include <Poco/File.h>
#include <boost/regex.hpp>
#include <fstream>
#include <sstream>
#include <string>

using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;

namespace Mantid::DataHandling {

DECLARE_FILELOADER_ALGORITHM(LoadGSS)

namespace { // anonymous namespace
const boost::regex DET_POS_REG_EXP{"^#.+flight path\\s+([0-9.]+).+"
                                   "tth\\s+([0-9.]+).+"
                                   "DIFC\\s+([0-9.]+)"};
const boost::regex L1_REG_EXP{"^#.+flight path\\s+([0-9.]+)\\s*m"};
} // end of anonymous namespace

//----------------------------------------------------------------------------------------------
/** Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadGSS::confidence(Kernel::FileDescriptor &descriptor) const {

  if (!descriptor.isAscii() || descriptor.extension() == ".tar")
    return 0;

  std::string str;
  std::istream &file = descriptor.data();
  std::getline(file, str); // workspace title first line
  while (!file.eof()) {
    std::getline(file, str);
    // Skip over empty and comment lines, as well as those coming from files
    // saved with the 'ExtendedHeader' option
    if (str.empty() || str[0] == '#' || str.compare(0, 8, "Monitor:") == 0) {
      continue;
    }
    if (str.compare(0, 4, "BANK") == 0 &&
        (str.find("RALF") != std::string::npos || str.find("SLOG") != std::string::npos) &&
        (str.find("FXYE") != std::string::npos)) {
      return 80;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------------------------
/** Initialise the algorithm
 */
void LoadGSS::init() {
  const std::vector<std::string> exts{".gsa", ".gss", ".gda", ".txt"};
  declareProperty(std::make_unique<API::FileProperty>("Filename", "", API::FileProperty::Load, exts),
                  "The input filename of the stored data");

  declareProperty(std::make_unique<API::WorkspaceProperty<>>("OutputWorkspace", "", Kernel::Direction::Output),
                  "Workspace name to load into.");

  declareProperty("UseBankIDasSpectrumNumber", false,
                  "If true, spectrum number corresponding to each bank is to "
                  "be its bank ID. ");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm
 */
void LoadGSS::exec() {
  // Process input parameters
  std::string filename = getPropertyValue("Filename");

  bool useBankAsSpectrum = getProperty("UseBankIDasSpectrumNumber");

  MatrixWorkspace_sptr outputWorkspace = loadGSASFile(filename, useBankAsSpectrum);

  setProperty("OutputWorkspace", outputWorkspace);
}

//----------------------------------------------------------------------------------------------
/** Main method to load GSAS file
 */
API::MatrixWorkspace_sptr LoadGSS::loadGSASFile(const std::string &filename, bool useBankAsSpectrum) {
  // Vectors for detector information
  double primaryflightpath = -1;
  std::vector<double> twothetas;
  std::vector<double> difcs;
  std::vector<double> totalflightpaths;
  std::vector<int> detectorIDs;

  // Vectors to store data
  std::vector<HistogramData::BinEdges> gsasDataX;
  std::vector<HistogramData::Counts> gsasDataY;
  std::vector<HistogramData::CountStandardDeviations> gsasDataE;

  std::vector<double> vecX, vecY, vecE;

  // progress
  std::unique_ptr<Progress> prog = nullptr;

  // Parameters for reading file
  char currentLine[256];
  std::string wsTitle;
  std::string slogTitle;
  std::string instrumentname = "Generic";
  char filetype = 'x';

  // Gather data
  std::ifstream input(filename.c_str(), std::ios_base::in);
  if (!input.is_open()) {
    // throw exception if file cannot be opened
    std::stringstream errss;
    errss << "Unable to open GSAS file " << filename;
    throw std::runtime_error(errss.str());
  }

  // First line: Title
  if (!input.eof()) {
    // Get workspace title (should be first line or 2nd line for SLOG)
    input.getline(currentLine, 256);
    wsTitle = currentLine;
  } else {
    throw std::runtime_error("File is empty");
  }

  // Loop all the lines
  bool isOutOfHead = false;
  bool slogtitleset = false;
  bool multiplybybinwidth = false;
  int nSpec = 0;
  bool calslogx0 = true;
  double bc4 = 0;
  double bc3 = 0;

  while (!input.eof() && input.getline(currentLine, 256)) {
    // Initialize progress after NSpec is imported
    if (nSpec != 0 && !prog) {
      prog = std::make_unique<Progress>(this, 0.0, 1.0, nSpec);
    }

    // Set flag to test SLOG
    if (!slogtitleset) {
      slogTitle = currentLine;
      slogtitleset = true;
    }

    if (currentLine[0] == '\n' || currentLine[0] == '#') {
      // Comment/information line
      std::string key1, key2;
      std::istringstream inputLine(currentLine, std::ios::in);
      inputLine.ignore(256, ' ');
      inputLine >> key1 >> key2;

      if (key2 == "Histograms") {
        // NSpec (Format: 'nspec HISTOGRAM')
        nSpec = std::stoi(key1);
        g_log.information() << "Histogram Line:  " << key1 << "  nSpec = " << nSpec << "\n";
      } else if (key1 == "Instrument:") {
        // Instrument (Format: 'Instrument XXXX')
        instrumentname = key2;
        g_log.information() << "Instrument    :  " << key2 << "\n";
      } else if (key1 == "with") {
        // Multiply by bin width: (Format: 'with multiplied')
        std::string s1;
        inputLine >> s1;
        if (s1 == "multiplied") {
          multiplybybinwidth = true;
          g_log.information() << "Y is multiplied by bin width\n";
        } else {
          g_log.warning() << "In line '" << currentLine << "', key word " << s1 << " is not allowed!\n";
        }
      } else if (key1 == "Primary") {
        // Primary flight path ...
        boost::smatch result;
        // Have to force a copy of the input or the stack gets corrupted
        // on MSVC when inputLine.str() falls out of scope which then
        // corrupts the value in result
        const std::string line = inputLine.str();
        if (boost::regex_search(line, result, L1_REG_EXP) && result.size() == 2) {
          primaryflightpath = std::stod(std::string(result[1]));

        } else {
          std::stringstream msg;
          msg << "Failed to parse primary flight path from line \"" << inputLine.str() << "\"";
          g_log.warning(msg.str());
        }

        std::stringstream msg;
        msg << "L1 = " << primaryflightpath;
        g_log.information(msg.str());
      } else if (key1 == "Total") {
        // Total flight path .... .... including total flying path, difc and
        // 2theta of 1 bank

        double totalpath(0.f);
        double tth(0.f);
        double difc(0.f);

        boost::smatch result;
        const std::string line = inputLine.str();
        if (boost::regex_search(line, result, DET_POS_REG_EXP) && result.size() == 4) {
          totalpath = std::stod(std::string(result[1]));
          tth = std::stod(std::string(result[2]));
          difc = std::stod(std::string(result[3]));
        } else {
          std::stringstream msg;
          msg << "Failed to parse position from line \"" << inputLine.str() << "\"";
          g_log.warning(msg.str());
        }

        totalflightpaths.emplace_back(totalpath);
        twothetas.emplace_back(tth);
        difcs.emplace_back(difc);

        std::stringstream msg;
        msg << "Bank " << difcs.size() - 1 << ": Total flight path = " << totalpath << "  2Theta = " << tth
            << "  DIFC = " << difc;
        g_log.information(msg.str());
      } // if keys....

    } // ENDIF for Line with #
    else if (currentLine[0] == 'B') {
      // Line start with Bank including file format, X0 information and etc.
      isOutOfHead = true;

      // If there is, Save the previous to array and initialize new MantiVec for
      // (X, Y, E)
      if (!vecX.empty()) {
        gsasDataX.emplace_back(std::move(vecX));
        gsasDataY.emplace_back(std::move(vecY));
        gsasDataE.emplace_back(std::move(vecE));
        vecX.clear();
        vecY.clear();
        vecE.clear();

        if (prog != nullptr)
          prog->report();
      }

      // Parse the bank line in format
      // RALF: BANK <SpectraNo> <NBins> <NBins> RALF <BC1> <BC2> <BC1> <BC4>
      // SLOG: BANK <SpectraNo> <NBins> <NBins> SLOG <BC1> <BC2> <BC3> 0>
      // where,
      // BC1 = X[0] * 32
      // BC2 = X[1] * 32 - BC1
      // BC4 = ( X[1] - X[0] ) / X[0]

      int specno, nbin1, nbin2;
      std::istringstream inputLine(currentLine, std::ios::in);

      double bc1 = 0;
      double bc2 = 0;

      inputLine.ignore(256, 'K');
      std::string filetypestring;

      inputLine >> specno >> nbin1 >> nbin2 >> filetypestring;
      g_log.debug() << "Bank: " << specno << "  filetypestring = " << filetypestring << '\n';

      detectorIDs.emplace_back(specno);

      if (filetypestring[0] == 'S') {
        // SLOG
        filetype = 's';
        inputLine >> bc1 >> bc2 >> bc3 >> bc4;
      } else if (filetypestring[0] == 'R') {
        // RALF
        filetype = 'r';
        inputLine >> bc1 >> bc2 >> bc1 >> bc4;
      } else {
        g_log.error() << "Unsupported GSAS File Type: " << filetypestring << "\n";
        throw Exception::FileError("Not a GSAS file", filename);
      }

      // Determine x0
      if (filetype == 'r') {
        double x0 = bc1 / 32;
        g_log.debug() << "RALF: x0 = " << x0 << "  bc4 = " << bc4 << '\n';
        vecX.emplace_back(x0);
      } else {
        // Cannot calculate x0, turn on the flag
        calslogx0 = true;
      }
    } // Line with B
    else if (isOutOfHead) {
      // Parse data line
      double xValue;
      double yValue;
      double eValue;

      double xPrev;

      // * Get previous X value
      if (!vecX.empty()) {
        xPrev = vecX.back();
      } else if (filetype == 'r') {
        // Except if RALF
        throw Mantid::Kernel::Exception::NotImplementedError("LoadGSS: File was not in expected format.");
      } else {
        xPrev = -0.0;
      }

      // It is different for the definition of X, Y, Z in SLOG and RALF format
      if (filetype == 'r') {
        // RALF
        // LoadGSS produces overlapping columns for some datasets, due to
        // std::setw
        // For this reason we need to read the column values as string and then
        // convert to double
        {
          std::string str(currentLine, 15);
          std::istringstream istr(str);
          istr >> xValue;
        }
        {
          std::string str(currentLine + 15, 18);
          std::istringstream istr(str);
          istr >> yValue;
        }
        {
          std::string str(currentLine + 15 + 18, 18);
          std::istringstream istr(str);
          istr >> eValue;
        }

        xValue = (2 * xValue) - xPrev;

      } else if (filetype == 's') {
        // SLOG
        std::istringstream inputLine(currentLine, std::ios::in);
        inputLine >> xValue >> yValue >> eValue;
        if (calslogx0) {
          // calculation of x0 must use the x'[0]
          g_log.debug() << "x'_0 = " << xValue << "  bc3 = " << bc3 << '\n';

          double x0 = 2 * xValue / (bc3 + 2.0);
          vecX.emplace_back(x0);
          xPrev = x0;
          g_log.debug() << "SLOG: x0 = " << x0 << '\n';
          calslogx0 = false;
        }

        xValue = (2 * xValue) - xPrev;
      } else {
        g_log.error() << "Unsupported GSAS File Type: " << filetype << "\n";
        throw Exception::FileError("Not a GSAS file", filename);
      }

      if (multiplybybinwidth) {
        yValue = yValue / (xValue - xPrev);
        eValue = eValue / (xValue - xPrev);
      }

      // store read in data (x, y, e) to vector
      vecX.emplace_back(xValue);
      vecY.emplace_back(yValue);
      vecE.emplace_back(eValue);
    } // Date Line
    else {
      g_log.warning() << "Line not defined: " << currentLine << '\n';
    }
  } // ENDWHILE of reading all lines

  // Get the sizes before using std::move
  auto nHist(static_cast<int>(gsasDataX.size()));
  auto xWidth(static_cast<int>(vecX.size()));
  auto yWidth(static_cast<int>(vecY.size()));

  // Push the vectors (X, Y, E) of the last bank to gsasData
  if (!vecX.empty()) { // Put final spectra into data
    gsasDataX.emplace_back(std::move(vecX));
    gsasDataY.emplace_back(std::move(vecY));
    gsasDataE.emplace_back(std::move(vecE));
    ++nHist;
  }
  input.close();

  //********************************************************************************************
  // Construct the workspace for GSS data
  //********************************************************************************************

  // Create workspace & GSS Files data is always in TOF

  MatrixWorkspace_sptr outputWorkspace = std::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceFactory::Instance().create("Workspace2D", nHist, xWidth, yWidth));
  outputWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

  // set workspace title
  if (filetype == 'r')
    outputWorkspace->setTitle(wsTitle);
  else
    outputWorkspace->setTitle(slogTitle);

  // put data from constructed histograms into outputWorkspace
  if (detectorIDs.size() != static_cast<size_t>(nHist)) {
    // File error is found
    std::ostringstream mess("");
    mess << "Number of spectra (" << detectorIDs.size() << ") is not equal to number of histograms (" << nHist << ").";
    throw std::runtime_error(mess.str());
  }

  for (int i = 0; i < nHist; ++i) {
    // Move data across
    outputWorkspace->setHistogram(i, BinEdges(std::move(gsasDataX[i])), Counts(std::move(gsasDataY[i])),
                                  CountStandardDeviations(std::move(gsasDataE[i])));

    // Reset spectrum number if
    if (useBankAsSpectrum) {
      auto specno = static_cast<specnum_t>(detectorIDs[i]);
      outputWorkspace->getSpectrum(i).setSpectrumNo(specno);
    }
  }

  // build instrument geometry
  createInstrumentGeometry(outputWorkspace, instrumentname, primaryflightpath, detectorIDs, totalflightpaths, twothetas,
                           difcs);

  return outputWorkspace;
}

//----------------------------------------------------------------------------------------------
/** Convert a string containing number and unit to double
 */
double LoadGSS::convertToDouble(const std::string &inputstring) {
  std::string temps;
  auto isize = static_cast<int>(inputstring.size());
  for (int i = 0; i < isize; i++) {
    char thechar = inputstring[i];
    if ((thechar <= 'Z' && thechar >= 'A') || (thechar <= 'z' && thechar >= 'a')) {
      break;
    } else {
      temps += thechar;
    }
  }

  double rd = std::stod(temps);

  return rd;
}

//----------------------------------------------------------------------------------------------
/** Create the instrument geometry with Instrument
 */
void LoadGSS::createInstrumentGeometry(const MatrixWorkspace_sptr &workspace, const std::string &instrumentname,
                                       const double &primaryflightpath, const std::vector<int> &detectorids,
                                       const std::vector<double> &totalflightpaths,
                                       const std::vector<double> &twothetas, const std::vector<double> &difcs) {
  // Check Input
  if (detectorids.size() != totalflightpaths.size() || totalflightpaths.size() != twothetas.size()) {
    g_log.warning("Cannot create geometry, because the numbers of L2 and Polar "
                  "are not equal.");
    return;
  }

  // Debug output
  std::stringstream dbss;
  dbss << "L1 = " << primaryflightpath << "\n";
  for (size_t i = 0; i < detectorids.size(); i++) {
    dbss << "Detector " << detectorids[i] << "  L1+L2 = " << totalflightpaths[i] << "  2Theta = " << twothetas[i]
         << "\n";
  }
  g_log.debug(dbss.str());

  // Create a new instrument and set its name
  Geometry::Instrument_sptr instrument(new Geometry::Instrument(instrumentname));

  // Add dummy source and samplepos to instrument
  Geometry::Component *samplepos = new Geometry::Component("Sample", instrument.get());
  instrument->add(samplepos);
  instrument->markAsSamplePos(samplepos);
  samplepos->setPos(0.0, 0.0, 0.0);

  Geometry::ObjComponent *source = new Geometry::ObjComponent("Source", instrument.get());
  instrument->add(source);
  instrument->markAsSource(source);

  double l1 = primaryflightpath;
  source->setPos(0.0, 0.0, -1.0 * l1);

  // Add detectors
  // The L2 and 2-theta values from Raw file assumed to be relative to sample
  // position
  const auto numDetector = static_cast<int>(detectorids.size()); // number of detectors
  // std::vector<int> detID = detectorids;    // detector IDs
  // std::vector<double> angle = twothetas;  // angle between indicent beam and
  // direction from sample to detector (two-theta)

  // Assumption: detector IDs are in the same order of workspace index
  for (int i = 0; i < numDetector; ++i) {
    // a) Create a new detector. Instrument will take ownership of pointer so no
    // need to delete.
    Geometry::Detector *detector = new Geometry::Detector("det", detectorids[i], samplepos);
    Kernel::V3D pos;

    // r is L2
    double r = totalflightpaths[i] - l1;
    pos.spherical(r, twothetas[i], 0.0);

    detector->setPos(pos);

    // add copy to instrument, spectrum and mark it
    auto &spec = workspace->getSpectrum(i);
    spec.clearDetectorIDs();
    spec.addDetectorID(detectorids[i]);
    instrument->add(detector);
    instrument->markAsDetector(detector);

  } // ENDFOR (i: spectrum)
  workspace->setInstrument(instrument);

  auto &paramMap = workspace->instrumentParameters();
  for (size_t i = 0; i < workspace->getNumberHistograms(); i++) {
    auto detector = workspace->getDetector(i);
    paramMap.addDouble(detector->getComponentID(), "DIFC", difcs[i]);
  }
}
} // namespace Mantid::DataHandling
