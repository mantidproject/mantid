#include "MantidVatesAPI/vtkDataSetToImplicitFunction.h"
#include "MantidVatesAPI/FieldDataToMetadata.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLDefinitions.h"
#include "MantidAPI/ImplicitFunctionFactory.h"
#include "MantidGeometry/MDGeometry/NullImplicitFunction.h"
#include <vtkDataSet.h>

namespace Mantid
{
  namespace VATES
  {
    /**
    Static creational method to run functionality in one method call.
    @param dataSet : input dataset containing field data.
    @return extracted implicit function.
    */
    Mantid::Geometry::MDImplicitFunction* vtkDataSetToImplicitFunction::exec(vtkDataSet* dataSet)
    {
      vtkDataSetToImplicitFunction temp(dataSet);
      return temp.execute();
    }

    
    /**
    Constructor
    @param dataSet : input dataset containing field data.
    */
    vtkDataSetToImplicitFunction::vtkDataSetToImplicitFunction(vtkDataSet* dataSet) : m_dataset(dataSet)
    {
      if(m_dataset == NULL)
      {
        throw std::runtime_error("Tried to construct vtkDataSetToImplicitFunction with NULL vtkDataSet");
      }
    }

    /**
    Execution method to run the extraction.
    @return implicit function if one could be found, or a NullImplicitFunction.
    */
    Mantid::Geometry::MDImplicitFunction* vtkDataSetToImplicitFunction::execute()
    {
      using Mantid::Geometry::NullImplicitFunction;
      using Mantid::Geometry::MDGeometryXMLDefinitions;
      Mantid::Geometry::MDImplicitFunction* function = new NullImplicitFunction;

      FieldDataToMetadata convert;
      std::string xmlString = convert(m_dataset->GetFieldData(), XMLDefinitions::metaDataId()); 
      if (false == xmlString.empty())
      {
        Poco::XML::DOMParser pParser;
        Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlString);
        Poco::XML::Element* pRootElem = pDoc->documentElement();
        Poco::XML::Element* functionElem = pRootElem->getChildElement(MDGeometryXMLDefinitions::functionElementName());
        if(NULL != functionElem)
        {
          delete function;
          function = Mantid::API::ImplicitFunctionFactory::Instance().createUnwrapped(functionElem);
        }
      }
      return function;
    }

    /// Destructor.
    vtkDataSetToImplicitFunction::~vtkDataSetToImplicitFunction()
    {
    }
  }
}
