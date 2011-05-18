#include "MantidVatesAPI/GeometryXMLParser.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidMDAlgorithms/DimensionFactory.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NamedNodeMap.h>

#include<algorithm>

namespace Mantid
{
  namespace VATES
  {
    /// Helper unary comparison type for finding IMDDimensions by a specified id.
    struct findID : public std::unary_function <Mantid::Geometry::IMDDimension, bool>
    {
      const std::string m_id;
      findID(const std::string id) : m_id(id){ }

      bool operator ()(const boost::shared_ptr<Mantid::Geometry::IMDDimension> obj) const
      {
        return m_id == obj->getDimensionId();
      }
      findID& operator=(const  findID&);
    };

    /// Helper unary comparison type for finding non-integrated dimensions.
    struct findIntegrated : public std::unary_function <Mantid::Geometry::IMDDimension, bool>
    {
      bool operator ()(const boost::shared_ptr<Mantid::Geometry::IMDDimension> obj) const
      {
        return obj->getIsIntegrated();
      }
      findIntegrated& operator=(const  findIntegrated&);
    };

     /**
     Validate the current object. Take action if not set up properly.
     */
     void GeometryXMLParser::validate() const
      {
        if(!m_executed)
        {
          throw std::runtime_error("Attempting to get dimension information from GeometryXMLParser, before calling ::execute()");
        }
      }
      
     /**
     Peforms the processing associated with these transformations.
     */
      void GeometryXMLParser::execute()
      {
        typedef std::vector<Mantid::Geometry::IMDDimension_sptr>::iterator Iterator;
        using namespace Mantid::Geometry;

        Poco::XML::DOMParser pParser;
        Poco::XML::Document* pDoc = pParser.parseString(m_xmlToProcess);
        Poco::XML::Element* pRootElem = pDoc->documentElement();
        //Apply root node checking if supplied.
        Poco::XML::Element* geometryXMLElement = NULL;
        if(!m_rootNodeName.empty())
        {
          Poco::XML::Element* temp = pRootElem->getChildElement(m_rootNodeName);
          geometryXMLElement = temp;
          if (geometryXMLElement == NULL)
          {
            std::string message = "Root node was not found to be the expected value of " + m_rootNodeName;
            throw std::runtime_error(message);
          }
        }
        else
        {
          //The default is to take the root node to be the geometry xml element.
          geometryXMLElement = pRootElem;
        }


        Poco::XML::NodeList* dimensionsXML = geometryXMLElement-> getElementsByTagName(XMLDefinitions::workspaceDimensionElementName());
        size_t nDimensions = dimensionsXML->length();
        VecIMDDimension_sptr vecAllDims(nDimensions);
        
        ////Extract dimensions
        for (size_t i = 0; i < nDimensions; i++)
        {
          Poco::XML::Element* dimensionXML = static_cast<Poco::XML::Element*> (dimensionsXML->item(i));
          Mantid::MDAlgorithms::DimensionFactory factory(dimensionXML);
          Mantid::Geometry::IMDDimension* dimension = factory.create();
          vecAllDims[i] = boost::shared_ptr<Mantid::Geometry::IMDDimension>(dimension);
        }
        VecIMDDimension_sptr vecNonMappedDims = vecAllDims;
        Poco::XML::Element* xDimensionElement = geometryXMLElement->getChildElement(XMLDefinitions::workspaceXDimensionElementName());
        std::string xDimId = xDimensionElement->getChildElement(XMLDefinitions::workspaceRefDimensionElementName())->innerText();
        if(!xDimId.empty())
        {
          Iterator xDimensionIt = find_if(vecAllDims.begin(), vecAllDims.end(), findID(xDimId));
          if (xDimensionIt == vecAllDims.end())
          {
            throw std::invalid_argument("Cannot determine x-dimension mapping.");
          }
          m_xDimension = *xDimensionIt;
          vecNonMappedDims.erase(std::remove_if(vecNonMappedDims.begin(), vecNonMappedDims.end(), findID(xDimId)));
        }

        Poco::XML::Element* yDimensionElement = geometryXMLElement->getChildElement(XMLDefinitions::workspaceYDimensionElementName());
        std::string yDimId = yDimensionElement->getChildElement(XMLDefinitions::workspaceRefDimensionElementName())->innerText();
        
        if(!yDimId.empty())
        {
          Iterator yDimensionIt = find_if(vecAllDims.begin(), vecAllDims.end(), findID(yDimId));
          if (yDimensionIt == vecAllDims.end())
          {
            throw std::invalid_argument("Cannot determine y-dimension mapping.");
          }
          m_yDimension = *yDimensionIt;
          vecNonMappedDims.erase(std::remove_if(vecNonMappedDims.begin(), vecNonMappedDims.end(), findID(yDimId)));
        }

        Poco::XML::Element* zDimensionElement = geometryXMLElement->getChildElement(XMLDefinitions::workspaceZDimensionElementName());
        std::string zDimId = zDimensionElement->getChildElement(XMLDefinitions::workspaceRefDimensionElementName())->innerText();
        
        if(!zDimId.empty())
        {
          Iterator zDimensionIt = find_if(vecAllDims.begin(), vecAllDims.end(), findID(zDimId));
          if (zDimensionIt == vecAllDims.end())
          {
            throw std::invalid_argument("Cannot determine z-dimension mapping.");
          }
          m_zDimension = *zDimensionIt;
          vecNonMappedDims.erase(std::remove_if(vecNonMappedDims.begin(), vecNonMappedDims.end(), findID(zDimId)));
        }

        Poco::XML::Element* tDimensionElement = geometryXMLElement->getChildElement(XMLDefinitions::workspaceTDimensionElementName());
        std::string tDimId = tDimensionElement->getChildElement(XMLDefinitions::workspaceRefDimensionElementName())->innerText();
        if(!tDimId.empty())
        {
          Iterator tDimensionIt = find_if(vecAllDims.begin(), vecAllDims.end(), findID(tDimId));
          if (tDimensionIt == vecAllDims.end())
          {
            throw std::invalid_argument("Cannot determine t-dimension mapping.");
          }
          m_tDimension = *tDimensionIt;
          vecNonMappedDims.erase(std::remove_if(vecNonMappedDims.begin(), vecNonMappedDims.end(), findID(tDimId)));
        }
        m_vecNonMappedDims = vecNonMappedDims; //Copy with strong guarantee.
        m_vecAllDims = vecAllDims;
        m_executed = true;
      }

     /**
     Constructor
     @param dataSet : vtkDataSet to process
     */
      GeometryXMLParser::GeometryXMLParser(const std::string& xmlToProcess) : m_xmlToProcess(xmlToProcess), m_executed(false)
      {
      }

      /**
      Constructor
     */
      GeometryXMLParser::GeometryXMLParser() : m_xmlToProcess(""), m_executed(false)
      {
      }

      /**
     Destructor
     */
      GeometryXMLParser::~GeometryXMLParser()
      {
      }

      
      /**
     Getter for x dimension
     @return x dimension.
     */
      Mantid::Geometry::IMDDimension_sptr GeometryXMLParser::getXDimension() const
      {
        validate();
        return m_xDimension;
      }

      /**
     Getter for y dimension
     @return y dimension.
     */
      Mantid::Geometry::IMDDimension_sptr GeometryXMLParser::getYDimension() const
      {
        validate();
        return m_yDimension;
      }

      /**
     Getter for z dimension
     @return z dimension.
     */
      Mantid::Geometry::IMDDimension_sptr GeometryXMLParser::getZDimension() const
      {
        validate();
        return m_zDimension;
      }

     /**
     Getter for t dimension
     @return t dimension.
     */
      Mantid::Geometry::IMDDimension_sptr GeometryXMLParser::getTDimension() const
      {
        validate();
        return m_tDimension;
      }

      /**
       Getter for all those dimensions which are not mapped.
       @return collection of non-mapped dimensions parsed.
      */
      Mantid::Geometry::VecIMDDimension_sptr GeometryXMLParser::getNonMappedDimensions() const
      {
        validate();
        return m_vecNonMappedDims;
      }

      /**
       Getter for all those dimensions which are not integrated.
       @return collection of non integrated dimensions parsed.
      */
      Mantid::Geometry::VecIMDDimension_sptr GeometryXMLParser::getNonIntegratedDimensions() const
      {
        validate();
        Mantid::Geometry::VecIMDDimension_sptr temp = m_vecAllDims;
        temp.erase(std::remove_if(temp.begin(), temp.end(), findIntegrated()), temp.end());
        return temp;
      }

       /**
       Getter for all dimensions parsed.
       @return collection of all dimensions parsed.
      */
      Mantid::Geometry::VecIMDDimension_sptr GeometryXMLParser::getAllDimensions() const
      {
        validate();
        return m_vecAllDims;
      }

      /**
       Determine wheter x dimension is present.
       @return true if available.
      */
      bool GeometryXMLParser::hasXDimension() const
      {
        validate();
        return NULL != m_xDimension.get();
      }

      /**
       Determine wheter y dimension is present.
       @return true if available.
      */
      bool GeometryXMLParser::hasYDimension() const
      {
        validate();
        return NULL != m_yDimension.get();
      }

      /**
       Determine wheter z dimension is present.
       @return true if available.
      */
      bool GeometryXMLParser::hasZDimension() const
      {
        validate();
        return NULL != m_zDimension.get();
      }

      /**
       Determine wheter t dimension is present.
       @return true if available.
      */
      bool GeometryXMLParser::hasTDimension() const
      {
        validate();
        return NULL != m_tDimension.get();
      }

      /**
      Setter for the root element.
      @parameter elementName : name of the element containing xml dimensions. Usually "Dimensions" unless xml snippet passed in directly, in which case do not set.
     */
      void GeometryXMLParser::SetRootNodeCheck(std::string elementName)
      {
        m_rootNodeName = elementName;
      }

       /**
      Assignement operator
      @parameter other : existing GeometryXMLParser to assign from.
      */
      GeometryXMLParser& GeometryXMLParser::operator=(const GeometryXMLParser& other)
      {
        if(this != &other)
        {
          m_executed = other.m_executed;
          m_rootNodeName = other.m_rootNodeName;
          m_vecNonMappedDims = other.m_vecNonMappedDims; 
          m_xDimension = other.m_xDimension;
          m_yDimension = other.m_yDimension;
          m_zDimension = other.m_zDimension;
          m_tDimension = other.m_tDimension;
        }
        return *this;
      }
      
      /**
      Copy constructor
      @parameter other : existing GeometryXMLParser to assign from.
      */
      GeometryXMLParser::GeometryXMLParser(const GeometryXMLParser& other) :
          m_executed(other.m_executed),
          m_rootNodeName(other.m_rootNodeName),
          m_vecNonMappedDims(other.m_vecNonMappedDims),
          m_xDimension(other.m_xDimension),
          m_yDimension(other.m_yDimension),
          m_zDimension(other.m_zDimension),
          m_tDimension(other.m_tDimension)
      {
      }
  }
}
