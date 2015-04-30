//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/SaveFocusedXYE.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Exception.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <exception>

using namespace Mantid::DataHandling;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveFocusedXYE)

//---------------------------------------------------
// Private member functions
//---------------------------------------------------
/**
 * Initialise the algorithm
 */
void SaveFocusedXYE::init() {
  declareProperty(
      new API::WorkspaceProperty<>("InputWorkspace", "",
                                   Kernel::Direction::Input),
      "The name of the workspace containing the data you wish to save");
  declareProperty(
      new API::FileProperty("Filename", "", API::FileProperty::Save),
      "The filename to use when saving data");
  declareProperty("SplitFiles", true,
                  "Save each spectrum in a different file (default true)");
  declareProperty("StartAtBankNumber", 0,
                  "Start bank (spectrum) numbers at this number in the file.  "
                  "The bank number in the file will be the workspace index + "
                  "StartAtBankNumber.");
  declareProperty(
      "Append", false,
      "If true and Filename already exists, append, else overwrite");
  declareProperty("IncludeHeader", true,
                  "Whether to include the header lines (default: true)");
  std::vector<std::string> header(3);
  header[0] = "XYE";
  header[1] = "MAUD";
  header[2] = "TOPAS";
  declareProperty("Format", "XYE",
                  boost::make_shared<Kernel::StringListValidator>(header),
                  "A type of the header: XYE (default) or MAUD.");
}

/**
 * Execute the algorithm
 */
void SaveFocusedXYE::exec() {
  using namespace Mantid::API;
  // Retrieve the input workspace
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const size_t nHist = inputWS->getNumberHistograms();
  const bool isHistogram = inputWS->isHistogramData();

  // this would be a subroutine if it were easier to return
  // two strings
  std::string filename = getProperty("Filename");
  std::string ext;
  {
      Poco::Path path(filename);
      std::string directory = path.parent().toString();
      std::string name = path.getFileName();

      std::size_t pos = name.find_first_of(".");
      if (pos != std::string::npos) // Remove the extension
      {
        ext = name.substr(pos + 1, name.npos);
        name = name.substr(0, pos);
      }

      filename = Poco::Path(directory, name).toString();
  }

  const bool append = getProperty("Append");
  const bool headers = getProperty("IncludeHeader");

  int startingbank = getProperty("StartAtBankNumber");
  if (startingbank < 0) {
    g_log.error() << "Starting bank number cannot be less than 0. "
                  << std::endl;
    throw std::invalid_argument("Incorrect starting bank number");
  }
  bool split = getProperty("SplitFiles");
  std::ostringstream number;
  std::fstream out;
  using std::ios_base;
  ios_base::openmode mode =
      (append ? (ios_base::out | ios_base::app) : ios_base::out);

  m_comment = "#";
  std::string headerType = getProperty("Format");
  if (headerType == "XYE") {
    m_headerType = XYE;
  } else if (headerType == "MAUD") {
    m_headerType = MAUD;
  } else if (headerType == "TOPAS") {
    m_headerType = TOPAS;
    m_comment = "'";
  } else {
    std::stringstream msg;
    msg << "Unrecognized format \"" << m_headerType << "\"";
    throw std::runtime_error(msg.str());
  }

  Progress progress(this, 0.0, 1.0, nHist);
  for (size_t i = 0; i < nHist; i++) {
    const MantidVec &X = inputWS->readX(i);
    const MantidVec &Y = inputWS->readY(i);
    const MantidVec &E = inputWS->readE(i);

    double l1 = 0;
    double l2 = 0;
    double tth = 0;
    if (headers) {
      // try to get detector information
      try {
        getFocusedPos(inputWS, i, l1, l2, tth);
      } catch (Kernel::Exception::NotFoundError &) {
        // if detector not found or there was an error skip this spectrum
        g_log.warning() << "Skipped spectrum " << i << std::endl;
        continue;
      }
    }

    if ((!split) && out) // Assign only one file
    {
      const std::string file(filename + '.' + ext);
      Poco::File fileObj(file);
      const bool exists = fileObj.exists();
      out.open(file.c_str(), mode);
      if (headers && (!exists || !append))
        writeHeaders(out, inputWS);
    } else if (split) // Several files will be created with names:
                      // filename-i.ext
    {
      number << "-" << i + startingbank;
      const std::string file(filename + number.str() + "." + ext);
      Poco::File fileObj(file);
      const bool exists = fileObj.exists();
      out.open(file.c_str(), mode);
      number.str("");
      if (headers && (!exists || !append))
        writeHeaders(out, inputWS);
    }

    if (!out.is_open()) {
      g_log.information("Could not open filename: " + filename);
      throw std::runtime_error("Could not open filename: " + filename);
    }

    if (headers) {
      writeSpectraHeader(out, i + startingbank,
                         inputWS->getSpectrum(i)->getSpectrumNo(), l1 + l2, tth,
                         inputWS->getAxis(0)->unit()->caption());
      // out << "# Data for spectra :" << i + startingbank << std::endl;
      // out << "# " << inputWS->getAxis(0)->unit()->caption() << "
      // Y                 E"
      //    << std::endl;
    }
    const size_t datasize = Y.size();
    for (size_t j = 0; j < datasize; j++) {
      double xvalue(0.0);
      if (isHistogram) {
        xvalue = (X[j] + X[j + 1]) / 2.0;
      } else {
        xvalue = X[j];
      }
      out << std::fixed << std::setprecision(5) << std::setw(15) << xvalue
          << std::fixed << std::setprecision(8) << std::setw(18) << Y[j]
          << std::fixed << std::setprecision(8) << std::setw(18) << E[j]
          << "\n";
    }
    // Close at each iteration
    if (split) {
      out.close();
    }
    progress.report();
  }
  // Close if single file
  if (!split) {
    out.close();
  }
  return;
}

/** virtual method to set the non workspace properties for this algorithm
 *  @param alg :: pointer to the algorithm
 *  @param propertyName :: name of the property
 *  @param propertyValue :: value  of the property
 *  @param perioidNum :: period number
 */
void SaveFocusedXYE::setOtherProperties(IAlgorithm *alg,
                                        const std::string &propertyName,
                                        const std::string &propertyValue,
                                        int perioidNum) {
  if (!propertyName.compare("Append")) {
    if (perioidNum != 1) {
      alg->setPropertyValue(propertyName, "1");
    } else
      alg->setPropertyValue(propertyName, propertyValue);
  } else
    Algorithm::setOtherProperties(alg, propertyName, propertyValue, perioidNum);
}

/**
 * Write the header information for the given workspace
 * @param os :: The stream to use to write the information
 * @param workspace :: A shared pointer to MatrixWorkspace
 */
void SaveFocusedXYE::writeHeaders(
    std::ostream &os,
    Mantid::API::MatrixWorkspace_const_sptr &workspace) const {
  if (m_headerType == XYE || m_headerType == TOPAS) {
    writeXYEHeaders(os, workspace);
  } else // MAUD
  {
    writeMAUDHeaders(os, workspace);
  }
}

/**
 * Write the header information in default "XYE" format
 * @param os :: The stream to use to write the information
 * @param workspace :: A shared pointer to MatrixWorkspace
 */
void SaveFocusedXYE::writeXYEHeaders(
    std::ostream &os,
    Mantid::API::MatrixWorkspace_const_sptr &workspace) const {
  os << m_comment << " File generated by Mantid:" << std::endl;
  os << m_comment << " Instrument: " << workspace->getInstrument()->getName()
     << std::endl;
  os << m_comment
     << " The X-axis unit is: " << workspace->getAxis(0)->unit()->caption()
     << std::endl;
  os << m_comment << " The Y-axis unit is: " << workspace->YUnitLabel()
     << std::endl;
}

/**
 * Write the header information in MAUD format
 * @param os :: The stream to use to write the information
 * @param workspace :: A shared pointer to MatrixWorkspace
 */
void SaveFocusedXYE::writeMAUDHeaders(
    std::ostream &os,
    Mantid::API::MatrixWorkspace_const_sptr &workspace) const {
  os << "#C  " << workspace->getTitle() << std::endl;
  os << "#C  " << workspace->getInstrument()->getName()
     << workspace->getRunNumber() << std::endl;
  os << "#A  OMEGA      90.00" << std::endl;
  os << "#A  CHI         0.00" << std::endl;
  os << "#A  PHI       -90.00" << std::endl;
  os << "#A  ETA         0.00" << std::endl;
}

/// Write spectra header
void SaveFocusedXYE::writeSpectraHeader(std::ostream &os, size_t index1,
                                        size_t index2, double flightPath,
                                        double tth,
                                        const std::string &caption) {
  if (m_headerType == XYE || m_headerType == TOPAS) {
    writeXYESpectraHeader(os, index1, index2, flightPath, tth, caption);
  } else // MAUD
  {
    writeMAUDSpectraHeader(os, index1, index2, flightPath, tth, caption);
  }
}

/// Write spectra XYE header
void SaveFocusedXYE::writeXYESpectraHeader(std::ostream &os, size_t index1,
                                           size_t index2, double flightPath,
                                           double tth,
                                           const std::string &caption) {
  UNUSED_ARG(index2);
  UNUSED_ARG(flightPath);
  UNUSED_ARG(tth);
  os << m_comment << " Data for spectra :" << index1 << std::endl;
  os << m_comment << " " << caption << "              Y                 E"
     << std::endl;
}

/// Write spectra MAUD header
void SaveFocusedXYE::writeMAUDSpectraHeader(std::ostream &os, size_t index1,
                                            size_t index2, double flightPath,
                                            double tth,
                                            const std::string &caption) {
  os << "#S" << std::setw(5) << index1 + 1 << " - Group" << std::setw(4)
     << index2 << std::endl;
  os << "#P0 0 0 " << tth << ' ' << flightPath << std::endl;
  os << "#L " << caption << " Data Error" << std::endl;
}

/**
* Determine the focused position for the supplied spectrum. The position
* (l1, l2, tth) is returned via the references passed in.
*/
void SaveFocusedXYE::getFocusedPos(Mantid::API::MatrixWorkspace_const_sptr wksp,
                                   const size_t spectrum, double &l1,
                                   double &l2, double &tth) {
  Geometry::Instrument_const_sptr instrument = wksp->getInstrument();
  if (instrument == NULL) {
    l1 = 0.;
    l2 = 0.;
    tth = 0.;
    return;
  }
  Geometry::IComponent_const_sptr source = instrument->getSource();
  Geometry::IComponent_const_sptr sample = instrument->getSample();
  if (source == NULL || sample == NULL) {
    l1 = 0.;
    l2 = 0.;
    tth = 0.;
    return;
  }
  l1 = source->getDistance(*sample);
  Geometry::IDetector_const_sptr det = wksp->getDetector(spectrum);
  l2 = det->getDistance(*sample);
  tth = wksp->detectorTwoTheta(det) * 180. / M_PI;
}
