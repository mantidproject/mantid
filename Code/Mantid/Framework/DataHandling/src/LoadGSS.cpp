//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/LoadGSS.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/Component.h"

#include <boost/math/special_functions/fpclassify.hpp>
#include <Poco/File.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid {
namespace DataHandling {

DECLARE_FILELOADER_ALGORITHM(LoadGSS)

//----------------------------------------------------------------------------------------------
/** Return the confidence with with this algorithm can load the file
  * @param descriptor A descriptor for the file
  * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
  */
int LoadGSS::confidence(Kernel::FileDescriptor &descriptor) const {
  if (!descriptor.isAscii())
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
        (str.find("RALF") != std::string::npos ||
         str.find("SLOG") != std::string::npos) &&
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
  std::vector<std::string> exts;
  exts.push_back(".gsa");
  exts.push_back(".gss");
  exts.push_back(".gda");
  exts.push_back(".txt");
  declareProperty(
      new API::FileProperty("Filename", "", API::FileProperty::Load, exts),
      "The input filename of the stored data");

  declareProperty(new API::WorkspaceProperty<>("OutputWorkspace", "",
                                               Kernel::Direction::Output),
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

  MatrixWorkspace_sptr outputWorkspace =
      loadGSASFile(filename, useBankAsSpectrum);

  setProperty("OutputWorkspace", outputWorkspace);

  return;
}

//----------------------------------------------------------------------------------------------
/** Main method to load GSAS file
  */
API::MatrixWorkspace_sptr LoadGSS::loadGSASFile(const std::string &filename,
                                                bool useBankAsSpectrum) {
  // Vectors for detector information
  double primaryflightpath = -1;
  std::vector<double> twothetas;
  std::vector<double> difcs;
  std::vector<double> totalflightpaths;
  std::vector<int> detectorIDs;

  // Vectors to store data
  std::vector<std::vector<double>> gsasDataX, gsasDataY, gsasDataE;
  std::vector<double> vecX, vecY, vecE;

  // progress
  Progress *prog = NULL;

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
    if (nSpec != 0 && prog == NULL) {
      prog = new Progress(this, 0.0, 1.0, nSpec);
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
        nSpec = atoi(key1.c_str());
        g_log.information() << "Histogram Line:  " << key1
                            << "  nSpec = " << nSpec << "\n";
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
          g_log.information() << "Y is multiplied by bin width" << std::endl;
        } else {
          g_log.warning() << "In line '" << currentLine << "', key word " << s1
                          << " is not allowed!\n";
        }
      } else if (key1 == "Primary") {
        // Primary flight path ...
        std::string s1, s2;
        inputLine >> s1 >> s2;
        primaryflightpath = atof(s2.c_str()); // convertToDouble(s2);
        g_log.information() << "L1 = " << primaryflightpath << std::endl;
      } else if (key1 == "Total") {
        // Total flight path .... .... including total flying path, difc and
        // 2theta of 1 bank
        std::string s1, s2, s3, s4, s5, s6;
        inputLine >> s1 >> s2 >> s3 >> s4 >> s5 >> s6;
#if 0
          double totalpath = convertToDouble(s2);
          double tth = convertToDouble(s4);
          double difc = convertToDouble(s6);
#else
        double totalpath = atof(s2.c_str());
        double tth = atof(s4.c_str());
        double difc = atof(s6.c_str());
#endif

        totalflightpaths.push_back(totalpath);
        twothetas.push_back(tth);
        difcs.push_back(difc);

        g_log.information() << "Bank " << difcs.size() - 1
                            << ": Total flight path = " << totalpath
                            << "  2Theta = " << tth << "  DIFC = " << difc
                            << "\n";
      } // if keys....

    } // ENDIF for Line with #
    else if (currentLine[0] == 'B') {
      // Line start with Bank including file format, X0 information and etc.
      isOutOfHead = true;

      // If there is, Save the previous to array and initialze new MantiVec for
      // (X, Y, E)
      if (vecX.size() != 0) {
        std::vector<double> storeX = vecX;
        std::vector<double> storeY = vecY;
        std::vector<double> storeE = vecE;

        gsasDataX.push_back(storeX);
        gsasDataY.push_back(storeY);
        gsasDataE.push_back(storeE);
        vecX.clear();
        vecY.clear();
        vecE.clear();

        if (prog != NULL)
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
      g_log.debug() << "Bank: " << specno
                    << "  filetypestring = " << filetypestring << std::endl;

      detectorIDs.push_back(specno);

      if (filetypestring[0] == 'S') {
        // SLOG
        filetype = 's';
        inputLine >> bc1 >> bc2 >> bc3 >> bc4;
      } else if (filetypestring[0] == 'R') {
        // RALF
        filetype = 'r';
        inputLine >> bc1 >> bc2 >> bc1 >> bc4;
      } else {
        g_log.error() << "Unsupported GSAS File Type: " << filetypestring
                      << "\n";
        throw Exception::FileError("Not a GSAS file", filename);
      }

      // Determine x0
      if (filetype == 'r') {
        double x0 = bc1 / 32;
        g_log.debug() << "RALF: x0 = " << x0 << "  bc4 = " << bc4 << std::endl;
        vecX.push_back(x0);
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
      if (vecX.size() != 0) {
        xPrev = vecX.back();
      } else if (filetype == 'r') {
        // Except if RALF
        throw Mantid::Kernel::Exception::NotImplementedError(
            "LoadGSS: File was not in expected format.");
      } else {
        xPrev = -0.0;
      }

      std::istringstream inputLine(currentLine, std::ios::in);

      // It is different for the definition of X, Y, Z in SLOG and RALF format
      if (filetype == 'r') {
        // RALF
        // LoadGSS produces overlapping columns for some datasets, due to
        // std::setw
        // For this reason we need to read the column values as string and then
        // convert to double
        std::string xString, yString, eString;
        inputLine >> std::setw(11) >> xString >> std::setw(18) >> yString >>
            std::setw(18) >> eString;
        xValue = boost::lexical_cast<double>(xString);
        yValue = boost::lexical_cast<double>(yString);
        eValue = boost::lexical_cast<double>(eString);
        xValue = (2 * xValue) - xPrev;

      } else if (filetype == 's') {
        // SLOG
        inputLine >> xValue >> yValue >> eValue;
        if (calslogx0) {
          // calculation of x0 must use the x'[0]
          g_log.debug() << "x'_0 = " << xValue << "  bc3 = " << bc3
                        << std::endl;

          double x0 = 2 * xValue / (bc3 + 2.0);
          vecX.push_back(x0);
          xPrev = x0;
          g_log.debug() << "SLOG: x0 = " << x0 << std::endl;
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
      vecX.push_back(xValue);
      vecY.push_back(yValue);
      vecE.push_back(eValue);
    } // Date Line
    else {
      g_log.warning() << "Line not defined: " << currentLine << std::endl;
    }
  } // ENDWHILE of readling all lines

  // Push the vectors (X, Y, E) of the last bank to gsasData
  if (vecX.size() != 0) { // Put final spectra into data
    gsasDataX.push_back(vecX);
    gsasDataY.push_back(vecY);
    gsasDataE.push_back(vecE);
  }
  input.close();

  //********************************************************************************************
  // Construct the workspace for GSS data
  //********************************************************************************************

  // Create workspace & GSS Files data is always in TOF
  int nHist(static_cast<int>(gsasDataX.size()));
  int xWidth(static_cast<int>(vecX.size()));
  int yWidth(static_cast<int>(vecY.size()));

  MatrixWorkspace_sptr outputWorkspace =
      boost::dynamic_pointer_cast<MatrixWorkspace>(
          WorkspaceFactory::Instance().create("Workspace2D", nHist, xWidth,
                                              yWidth));
  outputWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

  // set workspace title
  if (filetype == 'r')
    outputWorkspace->setTitle(wsTitle);
  else
    outputWorkspace->setTitle(slogTitle);

  // put data from MatidVec's into outputWorkspace
  if (detectorIDs.size() != static_cast<size_t>(nHist)) {
    // File error is found
    std::ostringstream mess("");
    mess << "Number of spectra (" << detectorIDs.size()
         << ") is not equal to number of histograms (" << nHist << ").";
    throw std::runtime_error(mess.str());
  }

  for (int i = 0; i < nHist; ++i) {
    // Move data across
    outputWorkspace->dataX(i) = gsasDataX[i];
    outputWorkspace->dataY(i) = gsasDataY[i];
    outputWorkspace->dataE(i) = gsasDataE[i];

    // Reset spectrum number if
    if (useBankAsSpectrum) {
      specid_t specno = static_cast<specid_t>(detectorIDs[i]);
      outputWorkspace->getSpectrum(i)->setSpectrumNo(specno);
    }
  }

  // build instrument geometry
  createInstrumentGeometry(outputWorkspace, instrumentname, primaryflightpath,
                           detectorIDs, totalflightpaths, twothetas);

  // Clean up
  delete prog;

  return outputWorkspace;
}

//----------------------------------------------------------------------------------------------
/** Convert a string containing number and unit to double
  */
double LoadGSS::convertToDouble(std::string inputstring) {
  std::string temps = "";
  int isize = (int)inputstring.size();
  for (int i = 0; i < isize; i++) {
    char thechar = inputstring[i];
    if ((thechar <= 'Z' && thechar >= 'A') ||
        (thechar <= 'z' && thechar >= 'a')) {
      break;
    } else {
      temps += thechar;
    }
  }

  double rd = atof(temps.c_str());

  return rd;
}

//----------------------------------------------------------------------------------------------
/** Create the instrument geometry with Instrument
  */
void LoadGSS::createInstrumentGeometry(
    MatrixWorkspace_sptr workspace, const std::string &instrumentname,
    const double &primaryflightpath, const std::vector<int> &detectorids,
    const std::vector<double> &totalflightpaths,
    const std::vector<double> &twothetas) {
  // Check Input
  if (detectorids.size() != totalflightpaths.size() ||
      totalflightpaths.size() != twothetas.size()) {
    g_log.warning("Cannot create geometry, because the numbers of L2 and Polar "
                  "are not equal.");
    return;
  }

  // Debug output
  std::stringstream dbss;
  dbss << "L1 = " << primaryflightpath << "\n";
  for (size_t i = 0; i < detectorids.size(); i++) {
    dbss << "Detector " << detectorids[i] << "  L1+L2 = " << totalflightpaths[i]
         << "  2Theta = " << twothetas[i] << "\n";
  }
  g_log.debug(dbss.str());

  // Create a new instrument and set its name
  Geometry::Instrument_sptr instrument(
      new Geometry::Instrument(instrumentname));
  workspace->setInstrument(instrument);

  // Add dummy source and samplepos to instrument
  Geometry::ObjComponent *samplepos =
      new Geometry::ObjComponent("Sample", instrument.get());
  instrument->add(samplepos);
  instrument->markAsSamplePos(samplepos);
  samplepos->setPos(0.0, 0.0, 0.0);

  Geometry::ObjComponent *source =
      new Geometry::ObjComponent("Source", instrument.get());
  instrument->add(source);
  instrument->markAsSource(source);

  double l1 = primaryflightpath;
  source->setPos(0.0, 0.0, -1.0 * l1);

  // Add detectors
  // The L2 and 2-theta values from Raw file assumed to be relative to sample
  // position
  const int numDetector = (int)detectorids.size(); // number of detectors
  // std::vector<int> detID = detectorids;    // detector IDs
  // std::vector<double> angle = twothetas;  // angle between indicent beam and
  // direction from sample to detector (two-theta)

  // Assumption: detector IDs are in the same order of workspace index
  for (int i = 0; i < numDetector; ++i) {
    // a) Create a new detector. Instrument will take ownership of pointer so no
    // need to delete.
    Geometry::Detector *detector =
        new Geometry::Detector("det", detectorids[i], samplepos);
    Kernel::V3D pos;

    // r is L2
    double r = totalflightpaths[i] - l1;
    pos.spherical(r, twothetas[i], 0.0);

    detector->setPos(pos);

    // add copy to instrument, spectrum and mark it
    API::ISpectrum *spec = workspace->getSpectrum(i);
    if (spec) {
      spec->clearDetectorIDs();
      spec->addDetectorID(detectorids[i]);
      instrument->add(detector);
      instrument->markAsDetector(detector);
    } else {
      g_log.error() << "Workspace " << i << " has no spectrum!" << std::endl;
      continue;
    }

  } // ENDFOR (i: spectrum)

  return;
}

} // namespace
} // namespace
