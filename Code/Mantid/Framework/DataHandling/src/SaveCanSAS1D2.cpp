//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveCanSAS1D2.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/IComponent.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/Run.h"
#include <boost/shared_ptr.hpp>

//-----------------------------------------------------------------------------
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveCanSAS1D2)

/// constructor
SaveCanSAS1D2::SaveCanSAS1D2() {}

/// destructor
SaveCanSAS1D2::~SaveCanSAS1D2() {}

/// Overwrites Algorithm method.
void SaveCanSAS1D2::init() {
  SaveCanSAS1D::init();

  declareProperty(
      new API::WorkspaceProperty<>(
          "Transmission", "", Kernel::Direction::Input, PropertyMode::Optional,
          boost::make_shared<API::WorkspaceUnitValidator>("Wavelength")),
      "The transmission workspace. Optional. If given, will be saved at "
      "TransmissionSpectrum");

  declareProperty(
      new API::WorkspaceProperty<>(
          "TransmissionCan", "", Kernel::Direction::Input,
          PropertyMode::Optional,
          boost::make_shared<API::WorkspaceUnitValidator>("Wavelength")),
      "The transmission workspace of the Can. Optional. If given, will be "
      "saved at TransmissionSpectrum");
}

/// Overwrites Algorithm method
void SaveCanSAS1D2::exec() {
  m_workspace = getProperty("InputWorkspace");
  m_trans_ws = getProperty("Transmission");
  m_transcan_ws = getProperty("TransmissionCan");
  if (!m_workspace) {
    throw std::invalid_argument(
        "Invalid inputworkspace ,Error in  SaveCanSAS1D");
  }

  if (m_workspace->getNumberHistograms() > 1) {
    throw std::invalid_argument(
        "Error in SaveCanSAS1D - more than one histogram.");
  }

  if ((m_trans_ws && m_trans_ws->getNumberHistograms() > 1) ||
      (m_transcan_ws && m_transcan_ws->getNumberHistograms() > 1)) {
    throw std::invalid_argument("Error in SaveCanSAS1D - more than one "
                                "histogram for the transmission workspaces");
  }

  // write xml manually as the user requires a specific format were the
  // placement of new line characters is controled
  // and this can't be done in using the stylesheet part in Poco or libXML
  prepareFileToWriteEntry();

  m_outFile << "\n\t<SASentry name=\"" << m_workspace->getName() << "\">";

  std::string sasTitle;
  createSASTitleElement(sasTitle);
  m_outFile << sasTitle;

  std::string sasRun;
  createSASRunElement(sasRun);
  m_outFile << sasRun;

  std::string dataUnit = m_workspace->YUnitLabel();
  // look for xml special characters and replace with entity refrence
  searchandreplaceSpecialChars(dataUnit);

  std::string sasData;
  createSASDataElement(sasData);
  m_outFile << sasData;

  if (m_trans_ws) {
    std::string transData;
    createSASTransElement(transData, "sample");
    m_outFile << transData;
  }
  if (m_transcan_ws) {
    std::string transData;
    createSASTransElement(transData, "can");
    m_outFile << transData;
  }

  std::string sasSample;
  createSASSampleElement(sasSample);
  m_outFile << sasSample;

  std::string sasInstr = "\n\t\t<SASinstrument>";
  m_outFile << sasInstr;
  std::string sasInstrName = "\n\t\t\t<name>";
  std::string instrname = m_workspace->getInstrument()->getName();
  // look for xml special characters and replace with entity refrence
  searchandreplaceSpecialChars(instrname);
  sasInstrName += instrname;
  sasInstrName += "</name>";
  m_outFile << sasInstrName;

  std::string sasSource;
  createSASSourceElement(sasSource);
  m_outFile << sasSource;

  std::string sasCollimation = "\n\t\t\t<SAScollimation/>";
  m_outFile << sasCollimation;

  try {
    std::string sasDet;
    createSASDetectorElement(sasDet);
    m_outFile << sasDet;
  } catch (Kernel::Exception::NotFoundError &) {
    m_outFile.close();
    throw;
  } catch (std::runtime_error &) {
    m_outFile.close();
    throw;
  }

  sasInstr = "\n\t\t</SASinstrument>";
  m_outFile << sasInstr;

  std::string sasProcess;
  createSASProcessElement(sasProcess);
  m_outFile << sasProcess;

  // Reduction process, if available
  const std::string process_xml = getProperty("Process");
  if (process_xml.size() > 0) {
    m_outFile << "\n\t\t<SASProcess>\n";
    m_outFile << process_xml;
    m_outFile << "\n\t\t</SASProcess>\n";
  }

  std::string sasNote = "\n\t\t<SASnote>";
  sasNote += "\n\t\t</SASnote>";
  m_outFile << sasNote;

  m_outFile << "\n\t</SASentry>";
  m_outFile << "\n</SASroot>";
  m_outFile.close();
}

/** This method creates an XML element named "SASroot"
 *  @param rootElem ::  xml root element string
 */
void SaveCanSAS1D2::createSASRootElement(std::string &rootElem) {
  rootElem = "<SASroot version=\"1.1\"";
  rootElem += "\n\t\t xmlns=\"urn:cansas1d:1.1\"";
  rootElem += "\n\t\t xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"";
  rootElem += "\n\t\t xsi:schemaLocation=\"urn:cansas1d:1.1 "
              "http://www.cansas.org/formats/1.1/cansas1d.xsd\"\n\t\t>";
}

/** This method creates an XML element named "SAStransmission_spectrum"
 *  @param sasTrans :: string for sasdata element in the xml
 * @param name :: name of the type of spectrum. Two values are acceptable:
 * sample, can
 */
void SaveCanSAS1D2::createSASTransElement(std::string &sasTrans,
                                          const std::string &name) {
  API::MatrixWorkspace_const_sptr m_ws;
  if (name == "sample") {
    m_ws = m_trans_ws;
  } else if (name == "can") {
    m_ws = m_transcan_ws;
  } else {
    return; // does not change sasTrans... it will add nothing
  }

  if (m_ws->getNumberHistograms() != 1)
    return; // does not change sasTrans

  std::stringstream trans;

  trans << "\n\t\t<SAStransmission_spectrum name=\"" << name << "\">";
  std::string t_unit = m_ws->YUnitLabel();
  std::string lambda_unit = m_ws->getAxis(0)->unit()->label();
  if (t_unit.empty())
    t_unit = "none";
  if (lambda_unit.empty() || lambda_unit == "Angstrom")
    lambda_unit = "A";

  const MantidVec &xdata = m_ws->readX(0);
  const MantidVec &ydata = m_ws->readY(0);
  const MantidVec &edata = m_ws->readE(0);
  const bool isHistogram = m_ws->isHistogramData();
  for (size_t j = 0; j < m_ws->blocksize(); ++j) {
    // x data is the Lambda in xml. If histogramdata take the mean
    double lambda = isHistogram ? (xdata[j] + xdata[j + 1]) / 2 : xdata[j];
    // y data is the T in xml.
    double trans_value = ydata[j];
    // e data is the Tdev in xml.
    double trans_err = edata[j];

    trans << "\n\t\t\t<Tdata><Lambda unit=\"" << lambda_unit << "\">";
    if (lambda == lambda) // check for NAN
      trans << lambda;
    else
      trans << "NaN";
    trans << "</Lambda>"
          << "<T unit=\"" << t_unit << "\">";
    if (trans_value == trans_value)
      trans << trans_value;
    else
      trans << "NaN";
    trans << "</T><Tdev unit=\"none\">";
    if (trans_err == trans_err)
      trans << trans_err;
    else
      trans << "NaN";
    trans << "</Tdev></Tdata>";
  }
  trans << "\n\t\t</SAStransmission_spectrum>";
  sasTrans += trans.str();
}

/** Write xml header tags including the root element and starting the SASentry
 *  element, this overrides the method in SaveCanSAS1D
 *  @param fileName :: the name of the file to write to
 *  @throw FileError if the file can't be opened or writen to
 */
void SaveCanSAS1D2::writeHeader(const std::string &fileName) {
  try {
    m_outFile.open(fileName.c_str(), std::ios::out | std::ios::trunc);
    // write the file header
    m_outFile
        << "<?xml version=\"1.0\"?>\n"
        << "<?xml-stylesheet type=\"text/xsl\" href=\"cansas1d.xsl\" ?>\n";
    std::string sasroot = "";
    createSASRootElement(sasroot);
    m_outFile << sasroot;
  } catch (std::fstream::failure &) {
    throw Exception::FileError("Error opening the output file for writing",
                               fileName);
  }
}
}
}
