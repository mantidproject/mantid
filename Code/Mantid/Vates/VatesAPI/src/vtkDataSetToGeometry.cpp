#include "MantidVatesAPI/vtkDataSetToGeometry.h"
#include "MantidVatesAPI/FieldDataToMetadata.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidGeometry/MDGeometry/IMDDimensionFactory.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLDefinitions.h"
#include "vtkDataSet.h"

#include<algorithm>
using namespace Mantid::Geometry;

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

        return MDGeometryXMLParser::execute();
      }

     /**
     Constructor
     @param dataSet : vtkDataSet to process
     */
      vtkDataSetToGeometry::vtkDataSetToGeometry(vtkDataSet* dataSet) : m_dataSet(dataSet)
      {
        //Format is to have DimensionSet as a nested element below MDInstructions.
        SetRootNodeCheck(Mantid::Geometry::MDGeometryXMLDefinitions::workspaceGeometryElementName());
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
      vtkDataSetToGeometry::vtkDataSetToGeometry(const vtkDataSetToGeometry& other) : MDGeometryXMLParser(other)
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
          MDGeometryXMLParser::operator=(other);
          m_dataSet = other.m_dataSet;
        }
        return *this;
      }
  }
}
