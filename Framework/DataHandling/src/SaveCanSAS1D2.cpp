#include "MantidDataHandling/SaveCanSAS1D2.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/Unit.h"

namespace {
void encode(std::string &data) {
  std::string buffer;
  buffer.reserve(data.size());

  for (auto &element : data) {
    switch (element) {
    case '&':
      buffer.append("&amp;");
      break;
    case '\"':
      buffer.append("&quot;");
      break;
    case '\'':
      buffer.append("&apos;");
      break;
    case '<':
      buffer.append("&lt;");
      break;
    case '>':
      buffer.append("&gt;");
      break;
    default:
      buffer.push_back(element);
    }
  }

  data.swap(buffer);
}
} // namespace

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveCanSAS1D2)

/// Overwrites Algorithm method.
void SaveCanSAS1D2::init() {
  SaveCanSAS1D::init();

  declareProperty(
      make_unique<API::WorkspaceProperty<>>(
          "Transmission", "", Kernel::Direction::Input, PropertyMode::Optional,
          boost::make_shared<API::WorkspaceUnitValidator>("Wavelength")),
      "The transmission workspace. Optional. If given, will be saved at "
      "TransmissionSpectrum");

  declareProperty(
      make_unique<API::WorkspaceProperty<>>(
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

  // Recording the SAS instrument can throw, if there
  // are no detecors present
  std::string sasInstrument;
  try {
    createSASInstrument(sasInstrument);
  } catch (Kernel::Exception::NotFoundError &) {
    throw;
  } catch (std::runtime_error &) {
    throw;
  }
  m_outFile << sasInstrument;

  std::string sasProcess;
  createSASProcessElement(sasProcess);
  m_outFile << sasProcess;

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

/** This method creates an XML element named "SASprocess"
 *  @param sasProcess :: string for sasprocess element in the xml
 */
void SaveCanSAS1D2::createSASProcessElement(std::string &sasProcess) {
  sasProcess = "\n\t\t<SASprocess>";
  // outFile<<sasProcess;

  std::string sasProcname = "\n\t\t\t<name>";
  sasProcname += "Mantid generated CanSAS1D XML";
  sasProcname += "</name>";
  sasProcess += sasProcname;

  time_t rawtime;
  time(&rawtime);

  char temp[25];
  strftime(temp, 25, "%d-%b-%Y %H:%M:%S", localtime(&rawtime));
  std::string sasDate(temp);

  std::string sasProcdate = "\n\t\t\t<date>";
  sasProcdate += sasDate;
  sasProcdate += "</date>";
  sasProcess += sasProcdate;

  std::string sasProcsvn = "\n\t\t\t<term name=\"svn\">";
  sasProcsvn += MantidVersion::version();
  sasProcsvn += "</term>";
  sasProcess += sasProcsvn;

  const API::Run &run = m_workspace->run();
  std::string user_file;
  if (run.hasProperty("UserFile")) {
    user_file = run.getLogData("UserFile")->value();
  }

  std::string sasProcuserfile = "\n\t\t\t<term name=\"user_file\">";
  sasProcuserfile += user_file;
  sasProcuserfile += "</term>";
  // outFile<<sasProcuserfile;
  sasProcess += sasProcuserfile;

  // can run number if available
  if (m_transcan_ws) {
    if (m_transcan_ws->run().hasProperty("run_number")) {
      Kernel::Property *logP = m_transcan_ws->run().getLogData("run_number");
      auto can_run = logP->value();
      std::string sasProcCanRun = "\n\t\t\t<term name=\"can_trans_run\">";
      sasProcCanRun += can_run;
      sasProcCanRun += "</term>";
      sasProcess += sasProcCanRun;
    } else {
      g_log.debug() << "Didn't find RunNumber log in workspace. Writing "
                       "<Run></Run> to the CANSAS file\n";
    }
  }

  // Reduction process note, if available
  std::string process_xml = getProperty("Process");
  if (!process_xml.empty()) {
    std::string processNote = "\n\t\t\t<SASprocessnote>";
    encode(process_xml);
    processNote += process_xml;
    processNote += "</SASprocessnote>";
    sasProcess += processNote;
  } else {
    sasProcess += "\n\t\t\t<SASprocessnote/>";
  }

  sasProcess += "\n\t\t</SASprocess>";
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
  std::string t_unit = m_ws->YUnit();
  std::string lambda_unit = m_ws->getAxis(0)->unit()->label();
  if (t_unit.empty())
    t_unit = "none";
  if (lambda_unit.empty() || lambda_unit == "Angstrom")
    lambda_unit = "A";

  // x data is the Lambda in xml. If histogramdata take the mean
  const auto lambda = m_ws->points(0);
  // y data is the T in xml.
  const auto &trans_value = m_ws->y(0);
  // e data is the Tdev in xml.
  const auto &trans_err = m_ws->e(0);
  for (size_t j = 0; j < trans_value.size(); ++j) {
    trans << "\n\t\t\t<Tdata><Lambda unit=\"" << lambda_unit << "\">";
    if (std::isnan(lambda[j]))
      trans << "NaN";
    else
      trans << lambda[j];
    trans << "</Lambda>"
          << "<T unit=\"" << t_unit << "\">";
    if (std::isnan(trans_value[j]))
      trans << "NaN";
    else
      trans << trans_value[j];
    trans << "</T><Tdev unit=\"none\">";
    if (std::isnan(trans_err[j]))
      trans << "NaN";
    else
      trans << trans_err[j];
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
    std::string sasroot;
    createSASRootElement(sasroot);
    m_outFile << sasroot;
  } catch (std::fstream::failure &) {
    throw Exception::FileError("Error opening the output file for writing",
                               fileName);
  }
}
} // namespace DataHandling
} // namespace Mantid
