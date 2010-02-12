//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveCanSAS1D.h"
#include "MantidKernel/FileProperty.h"
#include "MantidGeometry/IComponent.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/DOMWriter.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/Text.h"
#include "Poco/XML/XMLWriter.h"
#include <boost/shared_ptr.hpp>
#include <fstream>  
//-----------------------------------------------------------------------------
using namespace Mantid::Geometry;
using namespace Poco::XML;

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveCanSAS1D)

/// constructor
SaveCanSAS1D::SaveCanSAS1D()
{}

/// destructor
SaveCanSAS1D::~SaveCanSAS1D()
{}

/// Overwrites Algorithm method.
void SaveCanSAS1D::init()
{
  declareProperty(new API::WorkspaceProperty<>("InputWorkspace", "", Kernel::Direction::Input,
      new API::WorkspaceUnitValidator<>("MomentumTransfer")),
      "The input workspace, which must be in units of Q");
  std::vector<std::string> exts;
  exts.push_back("xml");
  declareProperty(new Kernel::FileProperty("Filename", "", Kernel::FileProperty::Save, exts),
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
  declareProperty("Radiation Source", "Spallation Neutron Source", new Kernel::ListValidator(
      radiation_source));
}

/// Overwrites Algorithm method
void SaveCanSAS1D::exec()
{
  m_workspace = getProperty("InputWorkspace");
  if (m_workspace->getNumberHistograms() > 1)
  {
    throw std::invalid_argument("Error in  SaveCanSAS1D");
  }
  std::string fileName = getPropertyValue("FileName");
  std::ofstream outFile(fileName.c_str());
  if (!outFile)
  {
    throw Kernel::Exception::FileError("Unable to open file:", fileName);
  }

  // Just write out header manually, because I can't see a way to do stylesheet part in Poco
  outFile << "<?xml version=\"1.0\"?>\n"
          << "<?xml-stylesheet type=\"text/xsl\" href=\"cansasxml-html.xsl\" ?>\n";

  DOMWriter writer;
  writer.setNewLine("\n");
  writer.setOptions(XMLWriter::PRETTY_PRINT);

  mDoc = new Document();
  Poco::XML::Element*sasRootElem = createSASRootElement();
  Poco::XML::Element*sasEntryElem = createSASEntryElement(sasRootElem);
  createTitleElement(sasEntryElem);
  createRunElement(sasEntryElem);
  createSASDataElement(sasEntryElem);
  createSASsample(sasEntryElem);
  createSASInstrument(sasEntryElem);
  createSASnote(sasEntryElem);

  writer.writeNode(outFile, mDoc);

}

/** This method creates an XML element named "SASroot"
 *  @return  Poco::XML::Element*  returns the element created
 */
Poco::XML::Element* SaveCanSAS1D::createSASRootElement()
{
  Poco::XML::Element*sasRootElem = mDoc->createElement("SASroot");
  throwException(sasRootElem, "SaveCanSAS1D::createSASRootElement", "SASroot");
  sasRootElem->setAttribute("version", "1.0");
  sasRootElem->setAttribute("xmlns", "cansas1d/1.0");
  sasRootElem->setAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
  sasRootElem->setAttribute("xsi:schemaLocation",
      "cansas1d/1.0 http://svn.smallangles.net/svn/canSAS/1dwg/trunk/cansas1d.xsd");
  mDoc->appendChild(sasRootElem);
  return sasRootElem;
}

/** This method creates an XML element named "SASentry"
 *  @param parent  parentelement 
 *  @return  Poco::XML::Element*  returns the element created
 */
Poco::XML::Element* SaveCanSAS1D::createSASEntryElement(Poco::XML::Element* parent)
{
  Poco::XML::Element* sasEntryElem = mDoc->createElement("SASentry");
  throwException(sasEntryElem, "SaveCanSAS1D::createSASEntryElement", "SASentry");
  parent->appendChild(sasEntryElem);
  return sasEntryElem;
}

/** This method creates an XML element named "Title"
 *  @param parent  parentelement 
 */
void SaveCanSAS1D::createTitleElement(Poco::XML::Element* parent)
{
  Poco::XML::Element* titleElem = mDoc->createElement("Title");
  throwException(titleElem, "SaveCanSAS1D::createTitleElement", "Title");
  Poco::XML::Text* titletext = mDoc->createTextNode(m_workspace->getTitle());
  throwException(titletext, "SaveCanSAS1D::createTitleElement", "Title");
  //throwException(titletext,"Title");
  parent->appendChild(titleElem);
  titleElem->appendChild(titletext);
}

/** This method creates an XML element named "Run"
 *  @param parent  parentelement 
 */
void SaveCanSAS1D::createRunElement(Poco::XML::Element* parent)
{
  Poco::XML::Element* runElem = mDoc->createElement("Run");
  throwException(runElem, "SaveCanSAS1D::createRunElement", "Run");
  Poco::XML::Text* runtext = mDoc->createTextNode(m_workspace->getName());
  throwException(runtext, "SaveCanSAS1D::createRunElement", "Run");
  parent->appendChild(runElem);
  runElem->appendChild(runtext);
}

/** This method creates an XML element named "SASData"
 *  @param parent  parent element 
 */
void SaveCanSAS1D::createSASDataElement(Poco::XML::Element* parent)
{
  Poco::XML::Element* sasDataElem = mDoc->createElement("SASdata");
  throwException(sasDataElem, "SaveCanSAS1D::createSASDataElement", "SASdata");
  parent->appendChild(sasDataElem);

  const std::string dataUnit = m_workspace->YUnitLabel();

  API::Progress progress(this,0.0,0.9,m_workspace->size());
  for (int i = 0; i < m_workspace->getNumberHistograms(); ++i)
  {
    const MantidVec& xdata = m_workspace->readX(i);
    const MantidVec& ydata = m_workspace->readY(i);
    const MantidVec& edata = m_workspace->readE(i);
    const bool isHistogram = m_workspace->isHistogramData();
    for (int j = 0; j < m_workspace->blocksize(); ++j)
    {
      Poco::XML::Element* iDataElem = mDoc->createElement("Idata");
      throwException(iDataElem, "SaveCanSAS1D::createSASDataElement", "Idata");
      sasDataElem->appendChild(iDataElem);

      //if histogramdata take the mean
      double intensity = isHistogram ? (xdata[j] + xdata[j + 1]) / 2 : xdata[j];
      std::stringstream x;
      x << intensity;
      // workspace X data is the Q element data in the xml file
      Poco::XML::Text* qText = mDoc->createTextNode(x.str());
      throwException(qText, "SaveCanSAS1D::createSASDataElement", "Q");
      Poco::XML::Element* qElem = mDoc->createElement("Q");
      throwException(qElem, "SaveCanSAS1D::createSASDataElement", "Q");
      qElem->setAttribute("unit", "1/A");
      iDataElem->appendChild(qElem);
      qElem->appendChild(qText);

      // workspace Y data is the I data in the xml file
      std::stringstream y;
      y << (ydata[j]);
      Poco::XML::Text*iText = mDoc->createTextNode(y.str());
      throwException(iText, "SaveCanSAS1D::createSASDataElement", "I");
      Poco::XML::Element*iElem = mDoc->createElement("I");
      throwException(iElem, "SaveCanSAS1D::createSASDataElement", "I");
      iElem->setAttribute("unit", dataUnit);
      iElem->appendChild(iText);
      iDataElem->appendChild(iElem);

      // workspace error data is the Idev data in the xml file
      std::stringstream e;
      e << edata[j];
      Poco::XML::Element* idevElem = mDoc->createElement("Idev");
      throwException(idevElem, "SaveCanSAS1D::createSASDataElement", "Idev");
      idevElem->setAttribute("unit", dataUnit);
      iDataElem->appendChild(idevElem);
      Poco::XML::Text*eText = mDoc->createTextNode(e.str());
      throwException(eText, "SaveCanSAS1D::createSASDataElement", "Idev");
      idevElem->appendChild(eText);

      progress.report();
    }
  }

}

/** This method creates an XML element named "SASsample"
 *  @param parent   is the parent element 
 */
void SaveCanSAS1D::createSASsample(Poco::XML::Element* parent)
{
  Poco::XML::Element* sasSampleElem = mDoc->createElement("SASsample");
  throwException(sasSampleElem, "SaveCanSAS1D::createSASsample", "SASsample");
  parent->appendChild(sasSampleElem);
  Poco::XML::Element* idElem = mDoc->createElement("ID");
  throwException(idElem, "SaveCanSAS1D::createSASsample", "ID");
  sasSampleElem->appendChild(idElem);

  Poco::XML::Text* idText = mDoc->createTextNode(m_workspace->getTitle());
  throwException(idText, "SaveCanSAS1D::createSASsample", "ID");
  idElem->appendChild(idText);
}

/** This method creates an XML element named "SASinstrument"
 and its conatined elements
 *  @param parent   is the parent element 
 */
void SaveCanSAS1D::createSASInstrument(Poco::XML::Element* parent)
{
  Poco::XML::Element* sasInstruementElem = mDoc->createElement("SASinstrument");
  throwException(sasInstruementElem, "SaveCanSAS1D::createSASInstrument", "SASinstrument");
  parent->appendChild(sasInstruementElem);

  Poco::XML::Element* instrnameElem = mDoc->createElement("name");
  throwException(instrnameElem, "SaveCanSAS1D::createSASInstrument", "name");
  sasInstruementElem->appendChild(instrnameElem);
  Poco::XML::Text* nameText = mDoc->createTextNode(m_workspace->getInstrument()->getName());
  throwException(nameText, "SaveCanSAS1D::createSASInstrument", "name");
  instrnameElem->appendChild(nameText);

  Poco::XML::Element* sasSourceElem = mDoc->createElement("SASsource");
  throwException(sasSourceElem, "SaveCanSAS1D::createSASInstrument", "SASsource");
  sasInstruementElem->appendChild(sasSourceElem);

  Poco::XML::Element* radiationElem = mDoc->createElement("radiation");
  throwException(radiationElem, "SaveCanSAS1D::createSASInstrument", "radiation");
  sasSourceElem->appendChild(radiationElem);
  std::string radiation_source = getPropertyValue("Radiation Source");
  Poco::XML::Text* radiationText = mDoc->createTextNode(radiation_source);
  throwException(radiationText, "SaveCanSAS1D::createSASInstrument", "radiation");
  radiationElem->appendChild(radiationText);

  Poco::XML::Element* sasCollimationElem = mDoc->createElement("SAScollimation");
  throwException(sasCollimationElem, "SaveCanSAS1D::createSASInstrument", "SAScollimation");
  sasInstruementElem->appendChild(sasCollimationElem);

  Poco::XML::Element* sasDetectorElem = mDoc->createElement("SASdetector");
  throwException(sasDetectorElem, "SaveCanSAS1D::createSASInstrument", "SASdetector");
  sasInstruementElem->appendChild(sasDetectorElem);

  Poco::XML::Element* detectornameElem = mDoc->createElement("name");
  throwException(detectornameElem, "SaveCanSAS1D::createSASInstrument", "name");
  sasDetectorElem->appendChild(detectornameElem);

  // Get the detector object (probably a group) that goes with the result spectrum
  Geometry::IDetector_const_sptr detgroup = m_workspace->getDetector(0);
  const int id = detgroup->getID(); //id of the first detector in the group
  // Now make sure we've got an individual detector object
  Geometry::IDetector_const_sptr det = m_workspace->getInstrument()->getDetector(id);
  // Get all its ancestors
  const std::vector<boost::shared_ptr<const IComponent> > ancs = det->getAncestors();
  // The one we want is the penultimate one
  // Shouldn't ever happen, but protect against detector having no ancestors
  const int size = ancs.size();
  const std::string detectorName = (size > 1) ? ancs[size-2]->getName() : det->getName();

  Poco::XML::Text* detectornameText = mDoc->createTextNode(detectorName);
  throwException(detectornameText, "SaveCanSAS1D::createSASInstrument", "name");
  detectornameElem->appendChild(detectornameText);

  Poco::XML::Element* detectorSDDElem = mDoc->createElement("SDD");
  throwException(detectorSDDElem, "SaveCanSAS1D::createSASInstrument", "SDD");
  sasDetectorElem->appendChild(detectorSDDElem);
  detectorSDDElem->setAttribute("unit", "m");
  std::stringstream sdd;
  double distance = detgroup->getDistance(*m_workspace->getInstrument()->getSample());
  sdd << distance;
  Poco::XML::Text* detectorSDDText = mDoc->createTextNode(sdd.str());
  throwException(detectorSDDText, "SaveCanSAS1D::createSASInstrument", "SDD");
  detectorSDDElem->appendChild(detectorSDDText);
}

/** This method creates an XML element named "SASnote"
 *  @param parent   is the parent element 
 */
void SaveCanSAS1D::createSASnote(Poco::XML::Element* parent)
{
  Poco::XML::Element* sasNoteElem = mDoc->createElement("SASnote");
  throwException(sasNoteElem, "SaveCanSAS1D::createSASnote", "SASnote");
  parent->appendChild(sasNoteElem);
  Poco::XML::Text* noteText = mDoc->createTextNode("Mantid generated CanSAS1d XML");
  throwException(noteText, "SaveCanSAS1D::createSASnote", "SASnote");
  sasNoteElem->appendChild(noteText);
}

/* This method throws NullPointerException when element creation fails 
 * @param elem pointer to  element
 * @param place  string which tells where the exception thrown
 * @param objectName  element name
 */
void SaveCanSAS1D::throwException(const Poco::XML::Element* elem, 
                                  const std::string & place,const std::string & objectName)
{
  if (!elem)
  {
    throw Kernel::Exception::NullPointerException(place, objectName);
  }
}

/* This method throws NullPointerException when text creation fails 
 * @param text pointer to  element
 * @param place  string which tells where the exception thrown
 * @param objectName  element name
 */
void SaveCanSAS1D::throwException(const Poco::XML::Text* text, 
                                  const std::string & place, const std::string & objectName)
{
  if (!text)
  {
    throw Kernel::Exception::NullPointerException(place, objectName);
  }
}

}

}
