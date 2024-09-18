// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadCanSAS1D.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/UnitFactory.h"

#include <Poco/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/SAX/InputSource.h>
#include <boost/algorithm/string.hpp>

using Poco::XML::Document;
using Poco::XML::DOMParser;
using Poco::XML::Element;
using Poco::XML::Node;
using Poco::XML::NodeList;

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace {
int getGeometryID(std::string selection) {
  int geometryID = 0;
  boost::to_lower(selection);
  if (selection == "cylinder") {
    geometryID = 1;
  } else if (selection == "flatplate" || selection == "flat plate") {
    geometryID = 2;
  } else if (selection == "disc") {
    geometryID = 3;
  }
  return geometryID;
}

/** Set a log value on the given run from the given element value, if the
 * element has the given name
 *
 * @param searchName : the element name to check against
 * @param elem : the element
 * @param run : the run to update the logs for
 * @param logName : the name of the log to update
 */
bool setLogFromElementIfNameIs(std::string const &searchName, Element *elem, Run &run, std::string const &logName) {
  if (!elem)
    return false;

  const std::string termName = elem->getAttribute("name");
  if (termName == searchName) {
    std::string file = elem->innerText();
    run.addLogData(new PropertyWithValue<std::string>(logName, file));
    return true;
  }

  return false;
}
} // namespace

namespace Mantid::DataHandling {

DECLARE_FILELOADER_ALGORITHM(LoadCanSAS1D)

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadCanSAS1D::confidence(Kernel::FileDescriptor &descriptor) const {
  const std::string &extn = descriptor.extension();
  if (extn != ".xml")
    return 0;

  std::istream &is = descriptor.data();
  int confidence(0);

  { // start of inner scope
    Poco::XML::InputSource src(is);
    // Set up the DOM parser and parse xml file
    DOMParser pParser;
    Poco::AutoPtr<Document> pDoc;
    try {
      pDoc = pParser.parse(&src);
    } catch (...) {
      throw Kernel::Exception::FileError("Unable to parse File:", descriptor.filename());
    }
    // Get pointer to root element
    Element *pRootElem = pDoc->documentElement();
    if (pRootElem) {
      if (pRootElem->tagName() == "SASroot") {
        confidence = 80;
      }
    }
  } // end of inner scope

  return confidence;
}

/// Overwrites Algorithm Init method.
void LoadCanSAS1D::init() {
  declareProperty(std::make_unique<API::FileProperty>("Filename", "", API::FileProperty::Load, ".xml"),
                  "The name of the CanSAS1D file to load");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Kernel::Direction::Output),
                  "The name to use for the output workspace");
}

/** Overwrites Algorithm exec method
 * @throw FileError if the file isn't valid xml
 * @throw NotFoundError if any expected elements couldn't be read
 * @throw NotImplementedError if any SASentry doesn't contain exactly one run
 */
void LoadCanSAS1D::exec() {
  const std::string fileName = getPropertyValue("Filename");
  // Set up the DOM parser and parse xml file
  DOMParser pParser;
  Poco::AutoPtr<Document> pDoc;
  try {
    pDoc = pParser.parse(fileName);
  } catch (...) {
    throw Exception::FileError("Unable to parse File:", fileName);
  }
  // Get pointer to root element
  Element *pRootElem = pDoc->documentElement();
  if (!pRootElem->hasChildNodes()) {
    throw Kernel::Exception::NotFoundError("No root element in CanSAS1D XML file", fileName);
  }
  // there can be multiple <SASentry> elements, each one contains a period which
  // will go into a workspace group if there are more than one of them
  Poco::AutoPtr<NodeList> entryList = pRootElem->getElementsByTagName("SASentry");
  size_t numEntries = entryList->length();
  Workspace_sptr outputWork;
  MatrixWorkspace_sptr WS;
  std::string runName;
  switch (numEntries) {
  case 0:
    throw Exception::NotFoundError("No <SASentry>s were found in the file", fileName);
    break;
  case 1:
    // the value of the string runName is unused in this case
    WS = loadEntry(entryList->item(0), runName);
    WS->mutableRun().addProperty("Filename", fileName);
    outputWork = WS;
    break;
  default:
    auto group = std::make_shared<WorkspaceGroup>();
    for (unsigned int i = 0; i < numEntries; ++i) {
      std::string run;
      MatrixWorkspace_sptr newWork = loadEntry(entryList->item(i), run);
      newWork->mutableRun().addProperty("Filename", fileName);
      appendDataToOutput(newWork, run, group);
    }
    outputWork = group;
  }
  setProperty("OutputWorkspace", outputWork);
}
/** Load an individual "<SASentry>" element into a new workspace
 * @param[in] workspaceData points to a "<SASentry>" element
 * @param[out] runName the name this workspace should take
 * @return dataWS this workspace will be filled with data
 * @throw NotFoundError if any expected elements couldn't be read
 * @throw NotImplementedError if the entry doesn't contain exactly one run
 */
MatrixWorkspace_sptr LoadCanSAS1D::loadEntry(Poco::XML::Node *const workspaceData, std::string &runName) {
  auto *workspaceElem = dynamic_cast<Element *>(workspaceData);
  check(workspaceElem, "<SASentry>");
  runName = workspaceElem->getAttribute("name");

  Poco::AutoPtr<NodeList> runs = workspaceElem->getElementsByTagName("Run");
  if (runs->length() != 1) {
    throw Exception::NotImplementedError("<SASentry>s containing multiple "
                                         "runs, or no runs, are not currently "
                                         "supported");
  }

  Element *sasDataElem = workspaceElem->getChildElement("SASdata");
  check(sasDataElem, "<SASdata>");
  // getting number of Idata elements in the xml file
  Poco::AutoPtr<NodeList> idataElemList = sasDataElem->getElementsByTagName("Idata");
  size_t nBins = idataElemList->length();

  MatrixWorkspace_sptr dataWS = WorkspaceFactory::Instance().create("Workspace2D", 1, nBins, nBins);

  createLogs(workspaceElem, dataWS);

  Element *titleElem = workspaceElem->getChildElement("Title");
  check(titleElem, "<Title>");
  dataWS->setTitle(titleElem->innerText());
  dataWS->setDistribution(true);
  dataWS->setYUnit("");

  // load workspace data
  auto &X = dataWS->mutableX(0);
  auto &Y = dataWS->mutableY(0);
  auto &E = dataWS->mutableE(0);

  dataWS->setPointStandardDeviations(0, nBins);
  auto &Dx = dataWS->mutableDx(0);
  int vecindex = 0;
  std::string yUnit;
  bool isCommon = true;
  // iterate through each Idata element  and get the values of "Q",
  //"I" and "Idev" text nodes and fill X,Y,E vectors
  for (unsigned long index = 0; index < nBins; ++index) {
    Node *idataElem = idataElemList->item(index);
    auto *elem = dynamic_cast<Element *>(idataElem);
    if (elem) {
      // setting X vector
      std::string nodeVal;
      Element *qElem = elem->getChildElement("Q");
      check(qElem, "Q");
      nodeVal = qElem->innerText();
      std::stringstream x(nodeVal);
      double d;
      x >> d;
      X[vecindex] = d;

      // setting dX vector [optional]
      Element *dqElem = elem->getChildElement("Qdev");
      if (dqElem) {
        nodeVal = dqElem->innerText();
        std::stringstream dx(nodeVal);
        dx >> d;
        Dx[vecindex] = d;
      }

      // setting Y vector
      Element *iElem = elem->getChildElement("I");
      check(qElem, "I");
      const std::string unit = iElem->getAttribute("unit");
      if (index == 0)
        yUnit = unit;
      else if (unit != yUnit)
        isCommon = false;
      nodeVal = iElem->innerText();
      std::stringstream y(nodeVal);
      y >> d;
      Y[vecindex] = d;

      // setting the error vector
      // If there is no error of the intensity recorded, then
      // it is assumed to be the sqare root of the intensity
      Element *idevElem = elem->getChildElement("Idev");
      if (idevElem) {
        check(qElem, "Idev");
        nodeVal = idevElem->innerText();
        std::stringstream e(nodeVal);
        e >> d;
        E[vecindex] = d;
      } else {
        E[vecindex] = std::sqrt(d);
      }

      ++vecindex;
    }
  }

  Element *instrElem = workspaceElem->getChildElement("SASinstrument");
  check(instrElem, "SASinstrument");
  std::string instname;
  Element *nameElem = instrElem->getChildElement("name");
  check(nameElem, "name");
  instname = nameElem->innerText();
  // run load instrument
  runLoadInstrument(instname, dataWS);

  // Load the sample information
  createSampleInformation(workspaceElem, dataWS);

  dataWS->getAxis(0)->setUnit("MomentumTransfer");
  if (isCommon)
    dataWS->setYUnitLabel(yUnit);
  return dataWS;
}
/* This method throws not found error if a element is not found in the xml file
 * @param[in] toCheck pointer to  element
 * @param[in] name element name
 *  @throw NotFoundError if the pointer is NULL
 */
void LoadCanSAS1D::check(const Poco::XML::Element *const toCheck, const std::string &name) const {
  if (!toCheck) {
    std::string fileName = getPropertyValue("Filename");
    throw Kernel::Exception::NotFoundError("<" + name + "> element not found in CanSAS1D XML file", fileName);
  }
}
/** Appends the first workspace to the second workspace. The second workspace
 * will became a group unless
 *  it was initially empty, in that situation it becames a copy of the first
 * workspace
 * @param[in] newWork the new data to add
 * @param[in] newWorkName the name that the new workspace will take
 * @param[out] container the data will be added to this group
 * @throw ExistsError if a workspace with this name had already been added
 */
void LoadCanSAS1D::appendDataToOutput(const API::MatrixWorkspace_sptr &newWork, const std::string &newWorkName,
                                      const API::WorkspaceGroup_sptr &container) {
  // the name of the property, like the workspace name must be different for
  // each workspace. Add "_run" at the end to stop problems with names like
  // "outputworkspace"
  std::string propName = newWorkName + "_run";

  // the following code registers the workspace with the AnalysisDataService and
  // with the workspace group, I'm taking this oone trust I don't know why it's
  // done this way sorry, Steve
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(propName, newWorkName, Direction::Output));
  container->addWorkspace(newWork);
  setProperty(propName, newWork);
}
/** Run the Child Algorithm LoadInstrument (as for LoadRaw)
 * @param inst_name :: The name written in the Nexus file
 * @param localWorkspace :: The workspace to insert the instrument into
 */
void LoadCanSAS1D::runLoadInstrument(const std::string &inst_name, const API::MatrixWorkspace_sptr &localWorkspace) {

  auto loadInst = createChildAlgorithm("LoadInstrument");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    loadInst->setPropertyValue("InstrumentName", inst_name);
    loadInst->setProperty<API::MatrixWorkspace_sptr>("Workspace", localWorkspace);
    loadInst->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
    loadInst->execute();
  } catch (std::invalid_argument &) {
    g_log.information("Invalid argument to LoadInstrument Child Algorithm");
  } catch (std::runtime_error &) {
    g_log.information("Unable to successfully run LoadInstrument Child Algorithm");
  }
}

/** Loads data into the run log
 *  @param[in] sasEntry the entry corresponding to the passed workspace
 *  @param[in] wSpace the log will be created in this workspace
 */
void LoadCanSAS1D::createLogs(const Poco::XML::Element *const sasEntry, const API::MatrixWorkspace_sptr &wSpace) const {
  API::Run &run = wSpace->mutableRun();
  Element *runText = sasEntry->getChildElement("Run");
  check(runText, "Run");
  run.addLogData(new PropertyWithValue<std::string>("run_number", runText->innerText()));

  Element *process = sasEntry->getChildElement("SASprocess");
  if (process) {
    Poco::AutoPtr<NodeList> terms = process->getElementsByTagName("term");
    auto setUserFile = false;
    auto setBatchFile = false;
    for (unsigned int i = 0; i < terms->length() && (!setUserFile || !setBatchFile); ++i) {
      Node *term = terms->item(i);
      auto *elem = dynamic_cast<Element *>(term);
      if (!setUserFile && setLogFromElementIfNameIs("user_file", elem, run, "UserFile"))
        setUserFile = true;
      else if (!setBatchFile && setLogFromElementIfNameIs("batch_file", elem, run, "BatchFile"))
        setBatchFile = true;
    }
  }
}

void LoadCanSAS1D::createSampleInformation(const Poco::XML::Element *const sasEntry,
                                           const Mantid::API::MatrixWorkspace_sptr &wSpace) const {
  auto &sample = wSpace->mutableSample();

  // Get the thickness information
  auto sasSampleElement = sasEntry->getChildElement("SASsample");
  check(sasSampleElement, "<SASsample>");
  auto thicknessElement = sasSampleElement->getChildElement("thickness");
  if (thicknessElement) {
    double thickness = std::stod(thicknessElement->innerText());
    sample.setThickness(thickness);
  }

  auto sasInstrumentElement = sasEntry->getChildElement("SASinstrument");
  check(sasInstrumentElement, "<SASinstrument>");
  auto sasCollimationElement = sasInstrumentElement->getChildElement("SAScollimation");
  check(sasCollimationElement, "<SAScollimation>");

  // Since we have shipped a sligthly invalid CanSAS1D format we need to
  // make sure that we can read those files back in again
  bool isInValidOldFormat = true;
  try {
    auto nameElement = sasCollimationElement->getChildElement("name");
    check(nameElement, "name");
  } catch (Kernel::Exception::NotFoundError &) {
    isInValidOldFormat = false;
  }

  if (isInValidOldFormat) {
    // Get the geometry information
    auto geometryElement = sasCollimationElement->getChildElement("name");
    if (geometryElement) {
      auto geometry = geometryElement->innerText();
      auto geometryID = getGeometryID(std::move(geometry));
      sample.setGeometryFlag(geometryID);
    }

    // Get the width information
    auto widthElement = sasCollimationElement->getChildElement("X");
    if (widthElement) {
      double width = std::stod(widthElement->innerText());
      sample.setWidth(width);
    }

    // Get the height information
    auto heightElement = sasCollimationElement->getChildElement("Y");
    if (heightElement) {
      double height = std::stod(heightElement->innerText());
      sample.setHeight(height);
    }

  } else {
    // Get aperture
    auto aperture = sasCollimationElement->getChildElement("aperture");
    if (aperture) {
      // Get geometry element
      auto geometry = aperture->getAttribute("name");
      if (!geometry.empty()) {
        auto geometryID = getGeometryID(Poco::XML::fromXMLString(geometry));
        sample.setGeometryFlag(geometryID);
      }

      // Get size
      auto size = aperture->getChildElement("size");

      // Get the width information
      auto widthElement = size->getChildElement("x");
      if (widthElement) {
        double width = std::stod(widthElement->innerText());
        sample.setWidth(width);
      }

      // Get the height information
      auto heightElement = size->getChildElement("y");
      if (heightElement) {
        double height = std::stod(heightElement->innerText());
        sample.setHeight(height);
      }
    }
  }
}
} // namespace Mantid::DataHandling
