// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveFocusedXYE.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Unit.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <cmath>
#include <exception>
#include <fstream>

using namespace Mantid::DataHandling;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveFocusedXYE)

/**
 * Initialise the algorithm
 */
void SaveFocusedXYE::init() {
  declareProperty(std::make_unique<API::WorkspaceProperty<>>("InputWorkspace", "", Kernel::Direction::Input),
                  "The name of the workspace containing the data you wish to save");
  declareProperty(std::make_unique<API::FileProperty>("Filename", "", API::FileProperty::Save),
                  "The filename to use when saving data");
  declareProperty("SplitFiles", true, "Save each spectrum in a different file (default true)");
  declareProperty("StartAtBankNumber", 0,
                  "Start bank (spectrum) numbers at this number in the file.  "
                  "The bank number in the file will be the workspace index + "
                  "StartAtBankNumber.");
  declareProperty("Append", false, "If true and Filename already exists, append, else overwrite");
  declareProperty("IncludeHeader", true, "Whether to include the header lines (default: true)");
  std::vector<std::string> header(3);
  header[0] = "XYE";
  header[1] = "MAUD";
  header[2] = "TOPAS";
  declareProperty("Format", "XYE", std::make_shared<Kernel::StringListValidator>(header),
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
  std::string filepath = getProperty("Filename");
  std::string ext;
  {
    Poco::Path path(filepath);
    std::string directory = path.parent().toString();
    std::string filename = path.getFileName();

    std::size_t pos = filename.find_first_of('.');
    if (pos != std::string::npos) // Remove the extension
    {
      ext = filename.substr(pos + 1, filename.npos);
      filename = filename.substr(0, pos);
    }

    filepath = Poco::Path(directory, filename).toString();
  }

  const bool append = getProperty("Append");
  const bool headers = getProperty("IncludeHeader");

  const int startingbank = getProperty("StartAtBankNumber");
  if (startingbank < 0) {
    g_log.error() << "Starting bank number cannot be less than 0. \n";
    throw std::invalid_argument("Incorrect starting bank number");
  }
  const bool split = getProperty("SplitFiles");
  std::ostringstream number;
  std::fstream out;
  std::ios_base::openmode mode = (append ? (std::ios_base::out | std::ios_base::app) : std::ios_base::out);

  m_comment = "#";
  const std::string headerType = getProperty("Format");
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

  const auto &detectorInfo = inputWS->detectorInfo();

  if (!split) {
    const std::string file(std::string(filepath).append(".").append(ext));
    Poco::File fileObj(file);
    const bool exists = fileObj.exists();
    out.open(file.c_str(), mode);
    if (headers && (!exists || !append))
      writeHeaders(out, inputWS);
  }

  Progress progress(this, 0.0, 1.0, nHist);
  for (size_t i = 0; i < nHist; i++) {
    const auto &X = inputWS->x(i);
    const auto &Y = inputWS->y(i);
    const auto &E = inputWS->e(i);

    double l1 = 0;
    double l2 = 0;
    double tth = 0;
    if (headers) {
      // try to get detector information
      try {
        l1 = detectorInfo.l1();
        l2 = detectorInfo.l2(i);
        tth = detectorInfo.twoTheta(i) * 180. / M_PI;
      } catch (std::logic_error &ex) {
        // DetectorInfo::twoTheta throws for monitors. Ignore and continue with
        // default value.
        g_log.warning() << ex.what() << '\n';
      } catch (std::runtime_error &ex) {
        g_log.warning() << ex.what() << '\n';
      }
    }

    if (split) {
      // Several files will be created with names filename-i.ext
      number << "-" << i + startingbank;
      const std::string file(std::string(filepath).append(number.str()).append(".").append(ext));
      Poco::File fileObj(file);
      const bool exists = fileObj.exists();
      out.open(file.c_str(), mode);
      number.str("");
      if (headers && (!exists || !append))
        writeHeaders(out, inputWS);
    }

    if (!out.is_open()) {
      g_log.information("Could not open filename: " + filepath);
      throw std::runtime_error("Could not open filename: " + filepath);
    }

    if (headers) {
      writeSpectraHeader(out, i + startingbank, inputWS->getSpectrum(i).getSpectrumNo(), l1 + l2, tth,
                         inputWS->getAxis(0)->unit()->caption(), inputWS->getAxis(1)->unit()->caption(),
                         inputWS->getAxis(1)->unit()->label(), inputWS->getAxis(1)->getValue(i));
    }
    const size_t datasize = Y.size();
    for (size_t j = 0; j < datasize; j++) {
      double xvalue(0.0);
      if (isHistogram) {
        xvalue = (X[j] + X[j + 1]) / 2.0;
      } else {
        xvalue = X[j];
      }
      out << std::fixed << std::setprecision(5) << std::setw(15) << xvalue << std::fixed << std::setprecision(8)
          << std::setw(18) << Y[j] << std::fixed << std::setprecision(8) << std::setw(18) << E[j] << "\n";
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
}

/** virtual method to set the non workspace properties for this algorithm
 *  @param alg :: pointer to the algorithm
 *  @param propertyName :: name of the property
 *  @param propertyValue :: value  of the property
 *  @param perioidNum :: period number
 */
void SaveFocusedXYE::setOtherProperties(IAlgorithm *alg, const std::string &propertyName,
                                        const std::string &propertyValue, int perioidNum) {
  if (propertyName == "Append") {
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
void SaveFocusedXYE::writeHeaders(std::ostream &os, Mantid::API::MatrixWorkspace_const_sptr &workspace) const {
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
void SaveFocusedXYE::writeXYEHeaders(std::ostream &os, Mantid::API::MatrixWorkspace_const_sptr &workspace) const {
  if (m_headerType != TOPAS)
    os << "XYDATA\n";
  os << m_comment << " File generated by Mantid, "
     << "Instrument " << workspace->getInstrument()->getName() << '\n';
  os << m_comment << " The X-axis unit is: " << workspace->getAxis(0)->unit()->caption()
     << ", The Y-axis unit is: " << workspace->YUnitLabel() << '\n';
}

/**
 * Write the header information in MAUD format
 * @param os :: The stream to use to write the information
 * @param workspace :: A shared pointer to MatrixWorkspace
 */
void SaveFocusedXYE::writeMAUDHeaders(std::ostream &os, Mantid::API::MatrixWorkspace_const_sptr &workspace) const {
  os << "#C  " << workspace->getTitle() << '\n';
  os << "#C  " << workspace->getInstrument()->getName() << workspace->getRunNumber() << '\n';
  os << "#A  OMEGA      90.00\n";
  os << "#A  CHI         0.00\n";
  os << "#A  PHI       -90.00\n";
  os << "#A  ETA         0.00\n";
}

/// Write spectra header
void SaveFocusedXYE::writeSpectraHeader(std::ostream &os, size_t index1, size_t index2, double flightPath, double tth,
                                        const std::string &caption, const std::string &spectrumAxisCaption,
                                        const std::string &spectraAxisLabel, double observable) {
  if (m_headerType == XYE || m_headerType == TOPAS) {
    writeXYESpectraHeader(os, index1, caption, spectrumAxisCaption, spectraAxisLabel, observable);
  } else // MAUD
  {
    writeMAUDSpectraHeader(os, index1, index2, flightPath, tth, caption);
  }
}

/// Write spectra XYE header
void SaveFocusedXYE::writeXYESpectraHeader(std::ostream &os, size_t index1, const std::string &caption,
                                           const std::string &spectrumAxisCaption, const std::string &spectraAxisLabel,
                                           double observable) {
  os << m_comment << " Data for spectra :" << index1 << '\n';
  if (spectrumAxisCaption == "Temperature") {
    os << "TEMP " << observable << ' ' << spectraAxisLabel << '\n';
  } else {
    os << m_comment << " " << spectrumAxisCaption << " " << observable << ' ' << spectraAxisLabel << '\n';
  }
  os << m_comment << " " << caption << "              Y                 E\n";
}

/// Write spectra MAUD header
void SaveFocusedXYE::writeMAUDSpectraHeader(std::ostream &os, size_t index1, size_t index2, double flightPath,
                                            double tth, const std::string &caption) {
  os << "#S" << std::setw(5) << index1 + 1 << " - Group" << std::setw(4) << index2 << '\n';
  os << "#P0 0 0 " << tth << ' ' << flightPath << '\n';
  os << "#L " << caption << " Data Error\n";
}
