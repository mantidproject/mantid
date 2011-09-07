#include "MantidGeometry/ComponentParser.h"
#include "MantidKernel/System.h"
#include "MantidGeometry/Instrument/Component.h"

using namespace Mantid::Kernel;

namespace Mantid
{
namespace Geometry
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ComponentParser::ComponentParser()
  {
    m_current.clear();
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ComponentParser::~ComponentParser()
  {
  }
  
  /** @return the top-level component created */
  Component * ComponentParser::getComponent()
  {
    if (m_current.size() > 0)
      return m_current[0];
    else
      return NULL;
  }


  //----------------------------------------------------------------------------------------------
  /// Signals start of element
  void ComponentParser::startElement(const Poco::XML::XMLString &, const Poco::XML::XMLString& localName, const Poco::XML::XMLString&, const Poco::XML::Attributes& attr)
  {
    // Find the parent of this new component.
    Component * parent = NULL;
    if (!m_current.empty())
      parent = m_current.back();

    //for (int i=0; i<attr.getLength(); i++)  std::cout << i << " : "<< attr.getQName(i) << "," << attr.getLocalName(i) << std::endl;

    // Find the name in the attributes
    std::string name = attr.getValue("", "name");

    Component * newComp = NULL;
    if (localName == "Component")
      newComp = new Component(name, parent);
    else
      throw std::runtime_error("ComponentParser:: unexpected XML tag '" + localName + "'.");

    // A new component was created
    if (newComp)
    {
      m_current.push_back(newComp);
      //TODO: Read from the new component
    }
  }


  //----------------------------------------------------------------------------------------------
  /// Signals end of element
  void ComponentParser::endElement(const Poco::XML::XMLString&, const Poco::XML::XMLString& localName, const Poco::XML::XMLString&)
  {
    UNUSED_ARG(localName);
  }

} // namespace Mantid
} // namespace Geometry

