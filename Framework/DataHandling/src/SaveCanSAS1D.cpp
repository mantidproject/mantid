// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveCanSAS1D.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceUnitValidator.h"

#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"

#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MantidVersion.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <memory>

#include <list>

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

namespace Mantid::DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveCanSAS1D)

/// Overwrites Algorithm method.
void SaveCanSAS1D::init() {
  declareProperty(
      std::make_unique<API::WorkspaceProperty<>>("InputWorkspace", "", Kernel::Direction::Input,
                                                 std::make_shared<API::WorkspaceUnitValidator>("MomentumTransfer")),
      "The input workspace, which must be in units of Q. Must be a 1D workspace.");
  declareProperty(std::make_unique<API::FileProperty>("Filename", "", API::FileProperty::Save, ".xml"),
                  "The name of the xml file to save");

  std::vector<std::string> radiation_source{"Spallation Neutron Source",
                                            "Pulsed Reactor Neutron Source",
                                            "Reactor Neutron Source",
                                            "Synchrotron X-ray Source",
                                            "Pulsed Muon Source",
                                            "Rotating Anode X-ray",
                                            "Fixed Tube X-ray",
                                            "neutron",
                                            "x-ray",
                                            "muon",
                                            "electron"};
  declareProperty("RadiationSource", "Spallation Neutron Source",
                  std::make_shared<Kernel::StringListValidator>(radiation_source), "The type of radiation used.");
  declareProperty("Append", false,
                  "Selecting append allows the workspace to "
                  "be added to an existing canSAS 1-D file as "
                  "a new SASentry");
  declareProperty("Process", "", "Text to append to Process section");
  declareProperty("DetectorNames", "",
                  "Specify in a comma separated list, which detectors to store "
                  "information about; \nwhere each name must match a name "
                  "given for a detector in the [[IDF|instrument definition "
                  "file (IDF)]]. \nIDFs are located in the instrument "
                  "sub-directory of the Mantid install directory.");

  // Collimation information
  std::vector<std::string> collimationGeometry{
      "Cylinder", "FlatPlate", "Flat plate", "Disc", "Unknown",
  };
  declareProperty("Geometry", "Unknown", std::make_shared<Kernel::StringListValidator>(collimationGeometry),
                  "The geometry type of the collimation.");
  auto mustBePositiveOrZero = std::make_shared<Kernel::BoundedValidator<double>>();
  mustBePositiveOrZero->setLower(0);
  declareProperty("SampleHeight", 0.0, mustBePositiveOrZero,
                  "The height of the collimation element in mm. If specified "
                  "as 0 it will not be recorded.");
  declareProperty("SampleWidth", 0.0, mustBePositiveOrZero,
                  "The width of the collimation element in mm. If specified as "
                  "0 it will not be recorded.");

  // Sample information
  declareProperty("SampleThickness", 0.0, mustBePositiveOrZero,
                  "The thickness of the sample in mm. If specified as 0 it "
                  "will not be recorded.");
}
/** Is called when the input workspace was actually a group, it sets the
 *  for all group members after the first so that the whole group is saved
 *  @param alg :: pointer to the algorithm
 *  @param propertyName :: name of the property
 *  @param propertyValue :: value  of the property
 *  @param perioidNum :: period number
 */
void SaveCanSAS1D::setOtherProperties(API::IAlgorithm *alg, const std::string &propertyName,
                                      const std::string &propertyValue, int perioidNum) {
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
    throw std::invalid_argument("Invalid inputworkspace ,Error in  SaveCanSAS1D");
  }

  if (m_workspace->getNumberHistograms() > 1) {
    throw std::invalid_argument("Error in SaveCanSAS1D - more than one histogram.");
  }

  // write xml manually as the user requires a specific format were the
  // placement of new line characters is controled
  // and this can't be done in using the stylesheet part in Poco or libXML
  prepareFileToWriteEntry(getPropertyValue("FileName"));

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
  createSASDataElement(sasData, 0);
  m_outFile << sasData;

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
/** Opens the output file and either moves the file pointer to beyond the last
 *  entry or blanks the file and writes a header
 *  @param fileName :: name of the output file
 *  @throw logic_error if append was selected but end of an entry tag couldn't
 * be found
 *  @throw FileError if there was a problem writing to the file
 */
void SaveCanSAS1D::prepareFileToWriteEntry(const std::string &fileName) {
  // reduce error handling code by making file access errors throw
  m_outFile.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);

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
    g_log.information() << "File " << filename << " couldn't be opened for a appending, will try to create the file\n";
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
    if (std::string(test_tag, LAST_TAG_LEN) != std::string(LAST_TAG, LAST_TAG_LEN)) {
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
    std::string sasroot;
    createSASRootElement(sasroot);
    m_outFile << sasroot;
  } catch (std::fstream::failure &) {
    throw Exception::FileError("Error opening the output file for writing", fileName);
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
  for (char specialchar : specialchars) {
    while (searchIndex < input.length()) {
      findIndex = input.find(specialchar, searchIndex);
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
void SaveCanSAS1D::replacewithEntityReference(std::string &input, const std::string::size_type &index) {
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
 *  @param workspaceIndex :: workspace index to be exported in SASdata entry
 */
void SaveCanSAS1D::createSASDataElement(std::string &sasData, size_t workspaceIndex) {
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

  const auto intensities = m_workspace->points(workspaceIndex);
  auto intensityDeltas = m_workspace->pointStandardDeviations(workspaceIndex);
  if (!intensityDeltas)
    intensityDeltas = HistogramData::PointStandardDeviations(intensities.size(), 0.0);
  const auto &ydata = m_workspace->y(workspaceIndex);
  const auto &edata = m_workspace->e(workspaceIndex);
  sasData += "\n\t\t<SASdata>";
  for (size_t j = 0; j < ydata.size(); ++j) {
    // x data is the QData in xml.If histogramdata take the mean
    std::stringstream x;
    x << intensities[j];
    std::stringstream dx_str;
    dx_str << intensityDeltas[j];
    sasData += "\n\t\t\t<Idata><Q unit=\"1/A\">";
    sasData += x.str();
    sasData += "</Q>";
    sasData += "<I unit=";
    sasData += "\"";
    sasData += dataUnit;
    sasData += "\">";
    //// workspace Y data is the I data in the xml file
    std::stringstream y;
    y << (ydata[j]);
    sasData += y.str();
    sasData += "</I>";

    // workspace error data is the Idev data in the xml file
    std::stringstream e;
    e << edata[j];

    sasData += "<Idev unit=";
    sasData += "\"";
    sasData += dataUnit;
    sasData += "\">";

    sasData += e.str();
    sasData += "</Idev>";

    sasData += "<Qdev unit=\"1/A\">";
    sasData += dx_str.str();
    sasData += "</Qdev>";

    sasData += "</Idata>";
  }
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
  // Add sample thickness information here. We only add it if
  // has been given a value larger than 0
  double thickness = getProperty("SampleThickness");
  if (thickness > 0) {
    std::string thicknessTag = "\n\t\t\t<thickness unit=\"mm\">";
    thicknessTag += std::to_string(thickness);
    thicknessTag += "</thickness>";
    sasSample += thicknessTag;
  }
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
  using std::placeholders::_1;
  boost::algorithm::split(detList, detectorNames, std::bind(std::equal_to<char>(), _1, ','));
  for (auto detectorName : detList) {
    boost::algorithm::trim(detectorName);

    // get first component with name detectorName in IDF
    std::shared_ptr<const IComponent> comp = m_workspace->getInstrument()->getComponentByName(detectorName);
    if (comp != std::shared_ptr<const IComponent>()) {
      sasDet += "\n\t\t\t<SASdetector>";

      std::string sasDetname = "\n\t\t\t\t<name>";
      sasDetname += detectorName;
      sasDetname += "</name>";
      sasDet += sasDetname;

      std::string sasDetUnit = "\n\t\t\t\t<SDD unit=\"m\">";

      std::stringstream sdd;
      double distance = comp->getDistance(*m_workspace->getInstrument()->getSample());
      sdd << distance;

      sasDetUnit += sdd.str();
      sasDetUnit += "</SDD>";

      sasDet += sasDetUnit;
      sasDet += "\n\t\t\t</SASdetector>";
    } else {
      g_log.notice() << "Detector with name " << detectorName
                     << " does not exist in the instrument of the workspace: " << m_workspace->getName() << '\n';
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
  std::string user_file;
  if (run.hasProperty("UserFile")) {
    user_file = run.getLogData("UserFile")->value();
  }

  std::string sasProcuserfile = "\n\t\t\t<term name=\"user_file\">";
  sasProcuserfile += user_file;
  sasProcuserfile += "</term>";
  // outFile<<sasProcuserfile;
  sasProcess += sasProcuserfile;

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

/** This method creates an XML element named "SASinstrument"
 *
 * The structure for required(r) parts of the SASinstrument and the parts
 * we want to add is:
 *
 * SASinstrument
 *   name(r)
 *   SASsource(r)
 *     radiation(r)
 *   SAScollimation(r)
 *     aperature[name]
 *       size
 *         x[units]
 *         y[units]
 *   SASdetector
 *     name
 *
 *  @param sasInstrument :: string for sasinstrument element in the xml
 */
void SaveCanSAS1D::createSASInstrument(std::string &sasInstrument) {
  sasInstrument = "\n\t\t<SASinstrument>";

  // Set name(r)
  std::string sasInstrName = "\n\t\t\t<name>";
  std::string instrname = m_workspace->getInstrument()->getName();
  // look for xml special characters and replace with entity refrence
  searchandreplaceSpecialChars(instrname);
  sasInstrName += instrname;
  sasInstrName += "</name>";
  sasInstrument += sasInstrName;

  // Set SASsource(r)
  std::string sasSource;
  createSASSourceElement(sasSource);
  sasInstrument += sasSource;

  // Set SAScollimation(r)
  // Add the collimation. We add the collimation information if
  // either the width of the height is different from 0
  double collimationHeight = getProperty("SampleHeight");
  double collimationWidth = getProperty("SampleWidth");
  std::string sasCollimation = "\n\t\t\t<SAScollimation/>";
  if (collimationHeight > 0 || collimationWidth > 0) {
    sasCollimation = "\n\t\t\t<SAScollimation>";

    // aperture with name
    std::string collimationGeometry = getProperty("Geometry");
    sasCollimation += "\n\t\t\t\t<aperture name=\"" + collimationGeometry + "\">";

    // size
    sasCollimation += "\n\t\t\t\t\t<size>";

    // Width
    sasCollimation += "\n\t\t\t\t\t\t<x unit=\"mm\">" + std::to_string(collimationWidth) + "</x>";
    // Height
    sasCollimation += "\n\t\t\t\t\t\t<y unit=\"mm\">" + std::to_string(collimationHeight) + "</y>";

    sasCollimation += "\n\t\t\t\t\t</size>";
    sasCollimation += "\n\t\t\t\t</aperture>";
    sasCollimation += "\n\t\t\t</SAScollimation>";
  }
  sasInstrument += sasCollimation;

  // Set SASdetector
  std::string sasDet;
  createSASDetectorElement(sasDet);
  sasInstrument += sasDet;
  sasInstrument += "\n\t\t</SASinstrument>";
}
} // namespace Mantid::DataHandling
