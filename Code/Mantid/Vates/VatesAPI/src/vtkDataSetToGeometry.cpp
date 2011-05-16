#include "MantidVatesAPI/vtkDataSetToGeometry.h"
#include "MantidVatesAPI/FieldDataToMetadata.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidMDAlgorithms/DimensionFactory.h"

#include "vtkDataSet.h"

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
    /// Helper unary comparison type for IMDDimensions.
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

     /**
     Validate the current object. Take action if not set up properly.
     */
     void vtkDataSetToGeometry::validate() const
      {
        if(!m_executed)
        {
          throw std::runtime_error("Attempting to get dimension information from vtkDataSetToGeometry, before calling ::execute()");
        }
      }
      
     /**
     Peforms the processing associated with these transformations.
     */
      void vtkDataSetToGeometry::execute()
      {
        typedef std::vector<Mantid::Geometry::IMDDimension_sptr>::iterator Iterator;
        FieldDataToMetadata convert;
        std::string xmlString = convert(m_dataSet->GetFieldData(), XMLDefinitions::metaDataId());

        Poco::XML::DOMParser pParser;
        Poco::XML::Document* pDoc = pParser.parseString(xmlString);
        Poco::XML::Element* pRootElem = pDoc->documentElement();
        Poco::XML::Element* geometryXMLElement = pRootElem->getChildElement(XMLDefinitions::workspaceGeometryElementName());
        if (geometryXMLElement == NULL)
        {
          throw std::runtime_error("The element containing the workspace geometry must be present.");
        }


        Poco::XML::NodeList* dimensionsXML = geometryXMLElement-> getElementsByTagName(XMLDefinitions::workspaceDimensionElementName());
        std::vector<Mantid::Geometry::IMDDimension_sptr > dimensionVec;

        ////Extract dimensions
        int nDimensions = dimensionsXML->length();
        for (int i = 0; i < nDimensions; i++)
        {
          Poco::XML::Element* dimensionXML = static_cast<Poco::XML::Element*> (dimensionsXML->item(i));
          Mantid::MDAlgorithms::DimensionFactory factory(dimensionXML);
          Mantid::Geometry::IMDDimension* dimension = factory.create();

          dimensionVec.push_back(boost::shared_ptr<Mantid::Geometry::IMDDimension>(dimension));
        }

        Poco::XML::Element* xDimensionElement = geometryXMLElement->getChildElement(XMLDefinitions::workspaceXDimensionElementName());
        std::string xDimId = xDimensionElement->getChildElement(XMLDefinitions::workspaceRefDimensionElementName())->innerText();
        if(!xDimId.empty())
        {
          Iterator xDimensionIt = find_if(dimensionVec.begin(), dimensionVec.end(), findID(xDimId));
          if (xDimensionIt == dimensionVec.end())
          {
            throw std::invalid_argument("Cannot determine x-dimension mapping.");
          }
          m_xDimension = *xDimensionIt;
        }

        Poco::XML::Element* yDimensionElement = geometryXMLElement->getChildElement(XMLDefinitions::workspaceYDimensionElementName());
        std::string yDimId = yDimensionElement->getChildElement(XMLDefinitions::workspaceRefDimensionElementName())->innerText();
        
        if(!yDimId.empty())
        {
          Iterator yDimensionIt = find_if(dimensionVec.begin(), dimensionVec.end(), findID(yDimId));
          if (yDimensionIt == dimensionVec.end())
          {
            throw std::invalid_argument("Cannot determine y-dimension mapping.");
          }
          m_yDimension = *yDimensionIt;
        }

        Poco::XML::Element* zDimensionElement = geometryXMLElement->getChildElement(XMLDefinitions::workspaceZDimensionElementName());
        std::string zDimId = zDimensionElement->getChildElement(XMLDefinitions::workspaceRefDimensionElementName())->innerText();
        
        if(!zDimId.empty())
        {
          Iterator zDimensionIt = find_if(dimensionVec.begin(), dimensionVec.end(), findID(zDimId));
          if (zDimensionIt == dimensionVec.end())
          {
            throw std::invalid_argument("Cannot determine z-dimension mapping.");
          }
          m_zDimension = *zDimensionIt;
        }

        Poco::XML::Element* tDimensionElement = geometryXMLElement->getChildElement(XMLDefinitions::workspaceTDimensionElementName());
        std::string tDimId = tDimensionElement->getChildElement(XMLDefinitions::workspaceRefDimensionElementName())->innerText();
        if(!tDimId.empty())
        {
          Iterator tDimensionIt = find_if(dimensionVec.begin(), dimensionVec.end(), findID(tDimId));
          if (tDimensionIt == dimensionVec.end())
          {
            throw std::invalid_argument("Cannot determine t-dimension mapping.");
          }
          m_tDimension = *tDimensionIt;
        }

        m_executed = true;
      }

     /**
     Constructor
     @param dataSet : vtkDataSet to process
     */
      vtkDataSetToGeometry::vtkDataSetToGeometry(vtkDataSet* dataSet) : m_dataSet(dataSet), m_executed(false)
      {
      }

      /**
     Destructor
     */
      vtkDataSetToGeometry::~vtkDataSetToGeometry()
      {
      }
  }
}