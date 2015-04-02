#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/BoxController.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/optional.hpp>
#include <Poco/File.h>
#include <Poco/DOM/Attr.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/NodeList.h>

#include <sstream>

using namespace Mantid::Kernel;
using Mantid::Kernel::Strings::convert;
using Mantid::Kernel::VectorHelper::splitStringIntoVector;

namespace Mantid {
namespace API {

//-----------------------------------------------------------------------------------
/** create new box controller from the existing one. Drops file-based state if
 * the box-controller was file-based   */
BoxController *BoxController::clone() const {
  // reset the clone file IO controller to avoid dublicated file based
  // operations for different box controllers
  return new BoxController(*this);
}

/*Private Copy constructor used in cloning */
BoxController::BoxController(const BoxController &other)
    : nd(other.nd), m_maxId(other.m_maxId),
      m_SplitThreshold(other.m_SplitThreshold), m_maxDepth(other.m_maxDepth),
      m_splitInto(other.m_splitInto), m_splitTopInto(other.m_splitTopInto), m_numSplit(other.m_numSplit),
      m_addingEvents_eventsPerTask(other.m_addingEvents_eventsPerTask),
      m_addingEvents_numTasksPerBlock(other.m_addingEvents_numTasksPerBlock),
      m_numMDBoxes(other.m_numMDBoxes),
      m_numMDGridBoxes(other.m_numMDGridBoxes),
      m_maxNumMDBoxes(other.m_maxNumMDBoxes),
      m_fileIO(boost::shared_ptr<API::IBoxControllerIO>()) {}

bool BoxController::operator==(const BoxController &other) const {
  if (nd != other.nd || m_maxId != other.m_maxId ||
      m_SplitThreshold != other.m_SplitThreshold ||
      m_maxDepth != other.m_maxDepth || m_numSplit != other.m_numSplit ||
      m_splitInto.size() != other.m_splitInto.size() ||
      m_numMDBoxes.size() != other.m_numMDBoxes.size() ||
      m_numMDGridBoxes.size() != other.m_numMDGridBoxes.size() ||
      m_maxNumMDBoxes.size() != other.m_maxNumMDBoxes.size())
    return false;

  for (size_t i = 0; i < m_splitInto.size(); i++) {
    if (m_splitInto[i] != other.m_splitInto[i])
      return false;
  }

  for (size_t i = 0; i < m_numMDBoxes.size(); i++) {
    if (m_numMDBoxes[i] != other.m_numMDBoxes[i])
      return false;
    if (m_numMDGridBoxes[i] != other.m_numMDGridBoxes[i])
      return false;
    if (m_maxNumMDBoxes[i] != other.m_maxNumMDBoxes[i])
      return false;
  }

  // Check top level splitting if they are set in both or not
  if (m_splitTopInto && !other.m_splitTopInto ||
      !m_splitTopInto && other.m_splitTopInto ) {
    return false;
  }

  if (m_splitTopInto && other.m_splitTopInto) {
    if (m_splitTopInto.get().size() != other.m_splitTopInto.get().size()) {
      return false;
    } else {
      for (size_t i = 0; i < m_splitTopInto.get().size(); i++) {
        if (m_splitTopInto.get()[i] != other.m_splitTopInto.get()[i])
          return false;
      }
    }
  }

  // There are number of variables which are
  // 1) derived:
  // Number of events sitting in the boxes which should be split but are already
  // split up to the max depth: volatile size_t m_numEventsAtMax;
  // 2) Dynamical and related to current processor and dynamical jobs
  // allocation:
  // For adding events tasks: size_t m_addingEvents_eventsPerTask;
  // m_addingEvents_numTasksPerBlock;
  // These variables are not compared here but may need to be compared in a
  // future for some purposes.

  return true;
}

/// Destructor
BoxController::~BoxController() {
  if (m_fileIO) {
    m_fileIO->closeFile();
    m_fileIO.reset();
  }
}
/**reserve range of id-s for use on set of adjacent boxes.
 * Needed to be thread safe as adjacent boxes have to have subsequent ID-s
 * @param range  --range number of box-id-s to lock
 * @returns initial ID to use in the range
 */
size_t BoxController::claimIDRange(size_t range) {
  m_idMutex.lock();
  size_t tmp = m_maxId;
  m_maxId += range;
  m_idMutex.unlock();
  return tmp;
}
/** Serialize to an XML string
 * @return XML string
 */
std::string BoxController::toXMLString() const {
  using namespace Poco::XML;

  // Create the root element for this fragment.
  AutoPtr<Document> pDoc = new Document;
  AutoPtr<Element> pBoxElement = pDoc->createElement("BoxController");
  pDoc->appendChild(pBoxElement);

  AutoPtr<Element> element;
  AutoPtr<Text> text;
  std::string vecStr;

  element = pDoc->createElement("NumDims");
  text =
      pDoc->createTextNode(boost::str(boost::format("%d") % this->getNDims()));
  element->appendChild(text);
  pBoxElement->appendChild(element);

  element = pDoc->createElement("MaxId");
  text =
      pDoc->createTextNode(boost::str(boost::format("%d") % this->getMaxId()));
  element->appendChild(text);
  pBoxElement->appendChild(element);

  element = pDoc->createElement("SplitThreshold");
  text = pDoc->createTextNode(
      boost::str(boost::format("%d") % this->getSplitThreshold()));
  element->appendChild(text);
  pBoxElement->appendChild(element);

  element = pDoc->createElement("MaxDepth");
  text = pDoc->createTextNode(
      boost::str(boost::format("%d") % this->getMaxDepth()));
  element->appendChild(text);
  pBoxElement->appendChild(element);

  element = pDoc->createElement("SplitInto");
  vecStr = Kernel::Strings::join(this->m_splitInto.begin(),
                                 this->m_splitInto.end(), ",");
  text = pDoc->createTextNode(vecStr);
  element->appendChild(text);
  pBoxElement->appendChild(element);

  element = pDoc->createElement("SplitTopInto");
  if (m_splitTopInto)
  {
    vecStr = Kernel::Strings::join(this->m_splitTopInto.get().begin(),
                                   this->m_splitTopInto.get().end(), ",");
  }
  else
  {
    vecStr = "";
  }
  text = pDoc->createTextNode(vecStr);
  element->appendChild(text);
  pBoxElement->appendChild(element);

  element = pDoc->createElement("NumMDBoxes");
  vecStr = Kernel::Strings::join(this->m_numMDBoxes.begin(),
                                 this->m_numMDBoxes.end(), ",");
  text = pDoc->createTextNode(vecStr);
  element->appendChild(text);
  pBoxElement->appendChild(element);

  element = pDoc->createElement("NumMDGridBoxes");
  vecStr = Kernel::Strings::join(this->m_numMDGridBoxes.begin(),
                                 this->m_numMDGridBoxes.end(), ",");
  text = pDoc->createTextNode(vecStr);
  element->appendChild(text);
  pBoxElement->appendChild(element);

  // Create a string representation of the DOM tree.
  std::stringstream xmlstream;
  DOMWriter writer;
  writer.writeNode(xmlstream, pDoc);

  return xmlstream.str().c_str();
}
/** the function left for compartibility with the previous bc python interface.
 @return  -- the file name of the file used for backup if file backup mode is
 enabled or emtpy sting if the workspace is not file backed   */
std::string BoxController::getFilename() const {
  if (m_fileIO)
    return m_fileIO->getFileName();
  else
    return "";
}
/** the function left for compartibility with the previous bc python interface.
@return true if the workspace is file based and false otherwise */
bool BoxController::useWriteBuffer() const {
  if (m_fileIO)
    return true;
  else
    return false;
}

//------------------------------------------------------------------------------------------------------
/** Static method that sets the data inside this BoxController from an XML
 *string
 *
 * @param xml :: string generated by BoxController::toXMLString()
 */
void BoxController::fromXMLString(const std::string &xml) {
  using namespace Poco::XML;
  Poco::XML::DOMParser pParser;
  Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xml);
  Poco::XML::Element *pBoxElement = pDoc->documentElement();

  std::string s;
  s = pBoxElement->getChildElement("NumDims")->innerText();
  Strings::convert(s, nd);
  if (nd <= 0 || nd > 20)
    throw std::runtime_error(
        "BoxController::fromXMLString(): Bad number of dimensions found.");

  size_t ival;
  Strings::convert(pBoxElement->getChildElement("MaxId")->innerText(), ival);
  this->setMaxId(ival);
  Strings::convert(pBoxElement->getChildElement("SplitThreshold")->innerText(),
                   ival);
  this->setSplitThreshold(ival);
  Strings::convert(pBoxElement->getChildElement("MaxDepth")->innerText(), ival);
  this->setMaxDepth(ival);

  s = pBoxElement->getChildElement("SplitInto")->innerText();
  this->m_splitInto = splitStringIntoVector<size_t>(s);

  // Need to make sure that we handle box controllers which did not have the SplitTopInto 
  // attribute 
  Poco::XML::NodeList* nodes = pBoxElement->getElementsByTagName("SplitTopInto");
  if (nodes->length() > 0)
  {
    s = pBoxElement->getChildElement("SplitTopInto")->innerText();
    if (s.empty())
    {
      this->m_splitTopInto = boost::none;
    }
    else
    {
      this->m_splitTopInto = splitStringIntoVector<size_t>(s);
    }
  }
  else
  {
    this->m_splitTopInto = boost::none;
  }

  s = pBoxElement->getChildElement("NumMDBoxes")->innerText();
  this->m_numMDBoxes = splitStringIntoVector<size_t>(s);

  s = pBoxElement->getChildElement("NumMDGridBoxes")->innerText();
  this->m_numMDGridBoxes = splitStringIntoVector<size_t>(s);

  this->calcNumSplit();
}
/** function clears the file-backed status of the box controller */
void BoxController::clearFileBacked() {
  if (m_fileIO) {
    // flush DB cache
    m_fileIO->flushCache();
    // close underlying file
    m_fileIO->closeFile();
    // decrease the sp counter by one and nullify this instance of sp.
    m_fileIO.reset(); // = boost::shared_ptr<API::IBoxControllerIO>();
  }
}
/** makes box controller file based by providing class, responsible for fileIO.
 *The box controller become responsible for the FileIO pointer
 *@param newFileIO -- instance of the box controller responsible for the IO;
 *@param fileName  -- if newFileIO comes without opened file, this is the file
 *name to open for the file based IO operations
*/
void BoxController::setFileBacked(boost::shared_ptr<IBoxControllerIO> newFileIO,
                                  const std::string &fileName) {
  if (!newFileIO->isOpened())
    newFileIO->openFile(fileName, "w");

  if (!newFileIO->isOpened()) {
    throw(Kernel::Exception::FileError(
        "Can not open target file for filebased box controller ", fileName));
  }

  this->m_fileIO = newFileIO;
}

} // namespace Mantid

} // namespace API
