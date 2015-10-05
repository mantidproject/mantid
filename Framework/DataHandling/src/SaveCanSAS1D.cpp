//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveCanSAS1D.h"
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
DECLARE_ALGORITHM(SaveCanSAS1D)

/// constructor
SaveCanSAS1D::SaveCanSAS1D() {}

/// destructor
SaveCanSAS1D::~SaveCanSAS1D() {}

/// Overwrites Algorithm method.
void SaveCanSAS1D::init() {
  declareProperty(
      new API::WorkspaceProperty<>(
          "InputWorkspace", "", Kernel::Direction::Input,
          boost::make_shared<API::WorkspaceUnitValidator>("MomentumTransfer")),
      "The input workspace, which must be in units of Q");
  declareProperty(
      new API::FileProperty("Filename", "", API::FileProperty::Save, ".xml"),
      "The name of the xml file to save");

  std::vector<std::string> radiation_source;
  radiation_source.push_back("Spallation Neutron Source");
  radiation_source.push_back("Pulsed Reactor Neutron Source");
  radiation_source.push_back("Reactor Neutron Source");
  radiation_source.push_back("Synchrotron X-ray Source");
  radiation_source.push_back("Pulsed Muon Source");
  radiation_source.push_back("Rotating Anode X-ray");
  radiation_source.push_back("Fixed Tube X-ray");
  radiation_source.push_back("neutron");
  radiation_source.push_back("x-ray");
  radiation_source.push_back("muon");
  radiation_source.push_back("electron");
  declareProperty(
      "RadiationSource", "Spallation Neutron Source",
      boost::make_shared<Kernel::StringListValidator>(radiation_source),
      "The type of radiation used.");
  declareProperty("Append", false, "Selecting append allows the workspace to "
                                   "be added to an existing canSAS 1-D file as "
                                   "a new SASentry");
  declareProperty("Process", "", "Text to append to Process section");
  declareProperty("DetectorNames", "",
                  "Specify in a comma separated list, which detectors to store "
                  "information about; \nwhere each name must match a name "
                  "given for a detector in the [[IDF|instrument definition "
                  "file (IDF)]]. \nIDFs are located in the instrument "
                  "sub-directory of the MantidPlot install directory.");
}
/** Is called when the input workspace was actually a group, it sets the
 *  for all group members after the first so that the whole group is saved
 *  @param alg :: pointer to the algorithm
 *  @param propertyName :: name of the property
 *  @param propertyValue :: value  of the property
 *  @param perioidNum :: period number
 */
void SaveCanSAS1D::setOtherProperties(API::IAlgorithm *alg,
                                      const std::string &propertyName,
                                      const std::string &propertyValue,
                                      int perioidNum) {
  // call the base class method
  Algorithm::setOtherProperties(alg, propertyName, propertyValue, perioidNum);

  if ((propertyName == "Append") && (perioidNum > 1)) {
    alg->setPropertyValue(propertyName, "1");
  }
}
/// Overwrites Algorithm method
void SaveCanSAS1D::exec() {
  m_workspace = getProperty("InputWorkspace");
  if (!m_workspace) {
    throw std::invalid_argument(
        "Invalid inputworkspace ,Error in  SaveCanSAS1D");
  }

  if (m_workspace->getNumberHistograms() > 1) {
    throw std::invalid_argument(
        "Error in SaveCanSAS1D - more than one histogram.");
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
/** Opens the output file and either moves the file pointer to beyond the last
 *  entry or blanks the file and writes a header
 *  @throw logic_error if append was selected but end of an entry tag couldn't
 * be found
 *  @throw FileError if there was a problem writing to the file
 */
void SaveCanSAS1D::prepareFileToWriteEntry() {
  // reduce error handling code by making file access errors throw
  m_outFile.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);

  const std::string fileName = getPropertyValue("FileName");
  bool append(getProperty("Append"));

  // write xml manually as the user requires a specific format were the
  // placement of new line characters is controled
  // and this can't be done in using the stylesheet part in Poco or libXML
  if (append) {
    append = openForAppending(fileName);
  }

  if (append) {
    findEndofLastEntry();
  } else {
    writeHeader(fileName);
  }
}
/** opens the named file if possible or returns false
 *  @param filename
::  *  @return true if the file was opened successfully and isn't empty
 */
bool SaveCanSAS1D::openForAppending(const std::string &filename) {
  try {
    m_outFile.open(filename.c_str(), std::ios::out | std::ios::in);
    // check if the file already has data
    m_outFile.seekg(0, std::ios::end);
    if (m_outFile.tellg() > 0) {
      // a file exists with data leave the file open and state that appending
      // should be possible
      return true;
    }
  } catch (std::fstream::failure &) {
    g_log.information()
        << "File " << filename
        << " couldn't be opened for a appending, will try to create the file\n";
  }
  m_outFile.clear();
  if (m_outFile.is_open()) {
    m_outFile.close();
  }
  return false;
}
/** Moves to the end of the last entry in the file, after &ltSASentry&gt
 *  before &lt/SASroot&gt
 *  @throw fstream::failure if the read or write commands couldn't complete
 *  @throw logic_error if the tag at the end of the last entry couldn't be found
 */
void SaveCanSAS1D::findEndofLastEntry() {
  const int rootTagLen = static_cast<int>(std::string("</SASroot>").length());

  try {
    static const int LAST_TAG_LEN = 11;

    // move to the place _near_ the end of the file where the data will be
    // appended to
    m_outFile.seekg(-LAST_TAG_LEN - rootTagLen, std::ios::end);
    char test_tag[LAST_TAG_LEN + 1];
    m_outFile.read(test_tag, LAST_TAG_LEN);
    // check we're in the correct place in the file
    static const char LAST_TAG[LAST_TAG_LEN + 1] = "</SASentry>";
    if (std::string(test_tag, LAST_TAG_LEN) !=
        std::string(LAST_TAG, LAST_TAG_LEN)) {
      // we'll allow some extra charaters so there is some variablity in where
      // the tag might be found
      bool tagFound(false);
      // UNCERT should be less than the length of a SASentry
      static const int UNCERT = 20;
      for (int i = 1; i < UNCERT; ++i) {
        // together this seek and read move the file pointer back on byte at a
        // time and read
        m_outFile.seekg(-i - LAST_TAG_LEN - rootTagLen, std::ios::end);
        m_outFile.read(test_tag, LAST_TAG_LEN);
        std::string read = std::string(test_tag, LAST_TAG_LEN);
        if (read == std::string(LAST_TAG, LAST_TAG_LEN)) {
          tagFound = true;
          break;
        }
      }
      if (!tagFound) {
        throw std::logic_error("Couldn't find the end of the existing data, "
                               "missing </SASentry> tag");
      }
    }
    // prepare to write to the place found by reading
    m_outFile.seekp(m_outFile.tellg(), std::ios::beg);
  } catch (std::fstream::failure &) {
    // give users more explaination about no being able to read their files
    throw std::logic_error("Trouble reading existing data in the output file, "
                           "are you appending to an invalid CanSAS1D file?");
  }
}
/** Write xml header tags including the root element and starting the SASentry
 *  element
 *  @param fileName :: the name of the file to write to
 *  @throw FileError if the file can't be opened or writen to
 */
void SaveCanSAS1D::writeHeader(const std::string &fileName) {
  try {
    m_outFile.open(fileName.c_str(), std::ios::out | std::ios::trunc);
    // write the file header
    m_outFile << "<?xml version=\"1.0\"?>\n"
              << "<?xml-stylesheet type=\"text/xsl\" "
                 "href=\"cansasxml-html.xsl\" ?>\n";
    std::string sasroot = "";
    createSASRootElement(sasroot);
    m_outFile << sasroot;
  } catch (std::fstream::failure &) {
    throw Exception::FileError("Error opening the output file for writing",
                               fileName);
  }
}
/** This method search for xml special characters in the input string
 * and  replaces this with xml entity reference
 *@param input :: -input string
 */
void SaveCanSAS1D::searchandreplaceSpecialChars(std::string &input) {
  std::string specialchars = "&<>'\"";
  std::string::size_type searchIndex = 0;
  std::string::size_type findIndex;
  for (std::string::size_type i = 0; i < specialchars.size(); ++i) {
    while (searchIndex < input.length()) {
      findIndex = input.find(specialchars[i], searchIndex);
      if (findIndex != std::string::npos) {
        searchIndex = findIndex + 1;
        // replace with xml entity refrence
        replacewithEntityReference(input, findIndex);

      } else {
        searchIndex = 0;
        break;
      }
    }
  }
}

/** This method retrieves the character at index and if it's a xml
 *  special character replaces with XML entity reference.
 *  @param input :: -input string
 *  @param index ::  position of the special character in the input string
 */
void SaveCanSAS1D::replacewithEntityReference(
    std::string &input, const std::string::size_type &index) {
  std::basic_string<char>::reference str = input.at(index);
  switch (str) {
  case '&':
    input.replace(index, 1, "&amp;");
    break;
  case '<':
    input.replace(index, 1, "&lt;");
    break;
  case '>':
    input.replace(index, 1, "&gt;");
    break;
  case '\'':
    input.replace(index, 1, "&apos;");
    break;
  case '\"':
    input.replace(index, 1, "&quot;");
    break;
  }
}

/** This method creates an XML element named "SASroot"
 *  @param rootElem ::  xml root element string
 */
void SaveCanSAS1D::createSASRootElement(std::string &rootElem) {
  rootElem = "<SASroot version=\"1.0\"";
  rootElem += "\n\t\t xmlns=\"cansas1d/1.0\"";
  rootElem += "\n\t\t xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"";
  rootElem += "\n\t\t xsi:schemaLocation=\"cansas1d/1.0 "
              "http://svn.smallangles.net/svn/canSAS/1dwg/trunk/"
              "cansas1d.xsd\">";
}

/** This method creates an XML element named "Title"
 *  @param sasTitle :: string for title element in the xml
 */
void SaveCanSAS1D::createSASTitleElement(std::string &sasTitle) {
  std::string title = m_workspace->getTitle();
  // look for xml special characters and replace with entity refrence
  searchandreplaceSpecialChars(title);
  sasTitle = "\n\t\t<Title>";
  sasTitle += title;
  sasTitle += "</Title>";
}

/** This method creates an XML element named "Run"
 *  @param sasRun :: string for run element in the xml
 */
void SaveCanSAS1D::createSASRunElement(std::string &sasRun) {
  // initialise the run number to an empty string, this may or may not be
  // changed later
  std::string run;
  if (m_workspace->run().hasProperty("run_number")) {
    Kernel::Property *logP = m_workspace->run().getLogData("run_number");
    run = logP->value();
  } else {
    g_log.debug() << "Didn't find RunNumber log in workspace. Writing "
                     "<Run></Run> to the CANSAS file\n";
  }

  searchandreplaceSpecialChars(run);

  sasRun = "\n\t\t<Run>";
  sasRun += run;
  sasRun += "</Run>";
}

/** This method creates an XML element named "SASdata"
 *  @param sasData :: string for sasdata element in the xml
 */
void SaveCanSAS1D::createSASDataElement(std::string &sasData) {
  std::string dataUnit = m_workspace->YUnitLabel();
  // look for xml special characters and replace with entity refrence
  searchandreplaceSpecialChars(dataUnit);

  // Workspaces that come out of the ISIS SANS reduction have had their
  // YUnitLabel
  // changed to "I(q) (cm-1)", but the CanSAS schema requires that the intensity
  // unit
  // be "1/cm"...  In lieu of a better way to handle YUnitLabels, (and trying to
  // avoid
  // *always* writing out "1/cm") we have the following (brittle) if-statement.
  if (dataUnit == "I(q) (cm-1)")
    dataUnit = "1/cm";

  sasData = "\n\t\t<SASdata>";
  // outFile<<sasData;
  std::string sasIData;
  std::string sasIBlockData;
  std::string sasIHistData;
  for (size_t i = 0; i < m_workspace->getNumberHistograms(); ++i) {
    const MantidVec &xdata = m_workspace->readX(i);
    const MantidVec &ydata = m_workspace->readY(i);
    const MantidVec &edata = m_workspace->readE(i);
    const MantidVec &dxdata = m_workspace->readDx(i);
    const bool isHistogram = m_workspace->isHistogramData();
    for (size_t j = 0; j < m_workspace->blocksize(); ++j) {
      // x data is the QData in xml.If histogramdata take the mean
      double intensity = isHistogram ? (xdata[j] + xdata[j + 1]) / 2 : xdata[j];
      double dx = isHistogram ? (dxdata[j] + dxdata[j + 1]) / 2 : dxdata[j];
      std::stringstream x;
      x << intensity;
      std::stringstream dx_str;
      dx_str << dx;
      sasIData = "\n\t\t\t<Idata><Q unit=\"1/A\">";
      sasIData += x.str();
      sasIData += "</Q>";
      sasIData += "<I unit=";
      sasIData += "\"";
      sasIData += dataUnit;
      sasIData += "\">";
      //// workspace Y data is the I data in the xml file
      std::stringstream y;
      y << (ydata[j]);
      sasIData += y.str();
      sasIData += "</I>";

      // workspace error data is the Idev data in the xml file
      std::stringstream e;
      e << edata[j];

      sasIData += "<Idev unit=";
      sasIData += "\"";
      sasIData += dataUnit;
      sasIData += "\">";

      sasIData += e.str();
      sasIData += "</Idev>";

      sasIData += "<Qdev unit=\"1/A\">";
      sasIData += dx_str.str();
      sasIData += "</Qdev>";

      sasIData += "</Idata>";
      // outFile<<sasIData;
      sasIBlockData += sasIData;
    }
    sasIHistData += sasIBlockData;
  }
  sasData += sasIHistData;

  sasData += "\n\t\t</SASdata>";
}

/** This method creates an XML element named "SASsample"
 *  @param sasSample :: string for sassample element in the xml
 */
void SaveCanSAS1D::createSASSampleElement(std::string &sasSample) {
  sasSample = "\n\t\t<SASsample>";
  // outFile<<sasSample;
  std::string sasSampleId = "\n\t\t\t<ID>";
  std::string sampleid = m_workspace->getTitle();
  // look for xml special characters and replace with entity refrence
  searchandreplaceSpecialChars(sampleid);
  sasSampleId += sampleid;
  sasSampleId += "</ID>";
  sasSample += sasSampleId;
  // outFile<<sasSampleId;
  sasSample += "\n\t\t</SASsample>";
}

/** This method creates an XML element named "SASsource"
 *  @param sasSource :: string for sassource element in the xml
 */
void SaveCanSAS1D::createSASSourceElement(std::string &sasSource) {
  sasSource = "\n\t\t\t<SASsource>";
  // outFile<<sasSource;

  std::string radiation_source = getPropertyValue("RadiationSource");
  std::string sasrad = "\n\t\t\t\t<radiation>";
  sasrad += radiation_source;
  sasrad += "</radiation>";
  sasSource += sasrad;
  // outFile<<sasrad;
  sasSource += "\n\t\t\t</SASsource>";
}
/** This method creates XML elements named "SASdetector". This method
    appends to sasDet.
 *  @param sasDet :: string for one or more sasdetector elements
 */
void SaveCanSAS1D::createSASDetectorElement(std::string &sasDet) {
  const std::string detectorNames = getProperty("DetectorNames");

  if (detectorNames.empty()) {
    sasDet += "\n\t\t\t<SASdetector>";
    std::string sasDetname = "\n\t\t\t\t<name/>";
    sasDet += sasDetname;
    sasDet += "\n\t\t\t</SASdetector>";
    return;
  }

  std::list<std::string> detList;
  boost::algorithm::split(detList, detectorNames,
                          std::bind2nd(std::equal_to<char>(), ','));
  for (std::list<std::string>::const_iterator itr = detList.begin();
       itr != detList.end(); ++itr) {
    std::string detectorName = *itr;
    boost::algorithm::trim(detectorName);

    // get first component with name detectorName in IDF
    boost::shared_ptr<const IComponent> comp =
        m_workspace->getInstrument()->getComponentByName(detectorName);
    if (comp != boost::shared_ptr<const IComponent>()) {
      sasDet += "\n\t\t\t<SASdetector>";

      std::string sasDetname = "\n\t\t\t\t<name>";
      sasDetname += detectorName;
      sasDetname += "</name>";
      sasDet += sasDetname;

      std::string sasDetUnit = "\n\t\t\t\t<SDD unit=\"m\">";

      std::stringstream sdd;
      double distance =
          comp->getDistance(*m_workspace->getInstrument()->getSample());
      sdd << distance;

      sasDetUnit += sdd.str();
      sasDetUnit += "</SDD>";

      sasDet += sasDetUnit;
      sasDet += "\n\t\t\t</SASdetector>";
    } else {
      g_log.notice() << "Detector with name " << detectorName
                     << " does not exist in the instrument of the workspace: "
                     << m_workspace->name() << std::endl;
    }
  }
}

/** This method creates an XML element named "SASprocess"
 *  @param sasProcess :: string for sasprocess element in the xml
 */
void SaveCanSAS1D::createSASProcessElement(std::string &sasProcess) {
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
  std::string user_file("");
  if (run.hasProperty("UserFile")) {
    user_file = run.getLogData("UserFile")->value();
  }

  std::string sasProcuserfile = "\n\t\t\t<term name=\"user_file\">";
  sasProcuserfile += user_file;
  sasProcuserfile += "</term>";
  // outFile<<sasProcuserfile;
  sasProcess += sasProcuserfile;

  sasProcess += "\n\t\t\t<SASprocessnote/>";

  sasProcess += "\n\t\t</SASprocess>";
}
}
}
