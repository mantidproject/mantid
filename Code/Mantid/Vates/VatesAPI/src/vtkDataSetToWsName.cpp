#include "MantidVatesAPI/vtkDataSetToWsName.h"
#include "MantidVatesAPI/FieldDataToMetadata.h"
#include "MantidVatesAPI/VatesXMLDefinitions.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLDefinitions.h"
#include <vtkDataSet.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Document.h>
#include <Poco/AutoPtr.h>

namespace Mantid
{
  namespace VATES
  {
    /**
    Static creational method to run functionality in one method call.
    @param dataset : input dataset containing field data.
    @return extracted workspace name.
    */
    std::string vtkDataSetToWsName::exec(vtkDataSet* dataset)
    {
      vtkDataSetToWsName temp(dataset);
      return temp.execute();
    }

    /**
    Constructor
    @param dataSet : input dataset containing field data.
    */
    vtkDataSetToWsName::vtkDataSetToWsName(vtkDataSet* dataSet)  : m_dataset(dataSet)
    {
      if(m_dataset == NULL)
      {
        throw std::runtime_error("Tried to construct vtkDataSetToWsName with NULL vtkDataSet");
      }
    }

    /**
    Runs the extraction
    @return extracted workspace name.
    */
    std::string vtkDataSetToWsName::execute()
    {
      using Mantid::Geometry::MDGeometryXMLDefinitions;
      FieldDataToMetadata convert;
      std::string xmlString = convert(m_dataset->GetFieldData(), XMLDefinitions::metaDataId());

      Poco::XML::DOMParser pParser;
      Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlString);
      Poco::XML::Element* pRootElem = pDoc->documentElement();
      Poco::XML::Element* wsNameElem = pRootElem->getChildElement(MDGeometryXMLDefinitions::workspaceNameElementName());
      if(wsNameElem == NULL)
      {
        throw std::runtime_error("The element containing the workspace name must be present.");
      }
      return wsNameElem->innerText();
    }

    /// Destructor.
    vtkDataSetToWsName::~vtkDataSetToWsName()
    {
    }
  }
}
