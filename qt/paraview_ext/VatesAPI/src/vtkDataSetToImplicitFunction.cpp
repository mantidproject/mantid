// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesAPI/vtkDataSetToImplicitFunction.h"
#include "MantidAPI/ImplicitFunctionFactory.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLDefinitions.h"
#include "MantidGeometry/MDGeometry/NullImplicitFunction.h"

#include "MantidVatesAPI/FieldDataToMetadata.h"
#include "MantidVatesAPI/VatesXMLDefinitions.h"
#include <vtkDataSet.h>

namespace Mantid {
namespace VATES {
/**
Static creational method to run functionality in one method call.
@param dataSet : input dataset containing field data.
@return extracted implicit function.
*/
Mantid::Geometry::MDImplicitFunction *
vtkDataSetToImplicitFunction::exec(vtkDataSet *dataSet) {
  vtkDataSetToImplicitFunction temp(dataSet);
  return temp.execute();
}

/**
Constructor
@param dataSet : input dataset containing field data.
*/
vtkDataSetToImplicitFunction::vtkDataSetToImplicitFunction(vtkDataSet *dataSet)
    : m_dataset(dataSet) {
  if (!m_dataset) {
    throw std::runtime_error(
        "Tried to construct vtkDataSetToImplicitFunction with NULL vtkDataSet");
  }
}

/**
Execution method to run the extraction.
@return implicit function if one could be found, or a NullImplicitFunction.
*/
Mantid::Geometry::MDImplicitFunction *vtkDataSetToImplicitFunction::execute() {
  using Mantid::Geometry::MDGeometryXMLDefinitions;
  using Mantid::Geometry::NullImplicitFunction;
  std::unique_ptr<Mantid::Geometry::MDImplicitFunction> function =
      std::make_unique<NullImplicitFunction>();

  FieldDataToMetadata convert;
  std::string xmlString =
      convert(m_dataset->GetFieldData(), XMLDefinitions::metaDataId());
  if (false == xmlString.empty()) {
    Poco::XML::DOMParser pParser;
    Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlString);
    Poco::XML::Element *pRootElem = pDoc->documentElement();
    Poco::XML::Element *functionElem = pRootElem->getChildElement(
        MDGeometryXMLDefinitions::functionElementName());
    if (functionElem) {
      auto existingFunction =
          std::unique_ptr<Mantid::Geometry::MDImplicitFunction>(
              Mantid::API::ImplicitFunctionFactory::Instance().createUnwrapped(
                  functionElem));
      function.swap(existingFunction);
    }
  }
  return function.release();
}

/// Destructor.
vtkDataSetToImplicitFunction::~vtkDataSetToImplicitFunction() = default;
} // namespace VATES
} // namespace Mantid
