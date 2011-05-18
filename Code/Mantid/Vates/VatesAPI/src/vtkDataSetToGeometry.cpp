#include "MantidVatesAPI/vtkDataSetToGeometry.h"
#include "MantidVatesAPI/FieldDataToMetadata.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidMDAlgorithms/DimensionFactory.h"

#include "vtkDataSet.h"

#include<algorithm>

namespace Mantid
{
  namespace VATES
  {

      
     /**
     Peforms the processing associated with these transformations.
     */
      void vtkDataSetToGeometry::execute()
      {
        FieldDataToMetadata convert;
        m_xmlToProcess = convert(m_dataSet->GetFieldData(), XMLDefinitions::metaDataId());

        return GeometryXMLParser::execute();
      }

     /**
     Constructor
     @param dataSet : vtkDataSet to process
     */
      vtkDataSetToGeometry::vtkDataSetToGeometry(vtkDataSet* dataSet) : m_dataSet(dataSet)
      {
        //Format is to have DimensionSet as a nested element below MDInstructions.
        SetRootNodeCheck(XMLDefinitions::workspaceGeometryElementName());
      }

      /**
      Destructor
      */
      vtkDataSetToGeometry::~vtkDataSetToGeometry()
      {
      }

      /**
      Copy constructor
      */
      vtkDataSetToGeometry::vtkDataSetToGeometry(const vtkDataSetToGeometry& other) : GeometryXMLParser(other)
        ,m_dataSet(other.m_dataSet)
      {
      }

      /**
      Assignment operator
      @return ref to assigned object.
      */
      vtkDataSetToGeometry& vtkDataSetToGeometry::operator=(const vtkDataSetToGeometry& other)
      {
        if(this != &other)
        {
          GeometryXMLParser::operator=(other);
          m_dataSet = other.m_dataSet;
        }
        return *this;
      }
  }
}
