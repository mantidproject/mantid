// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesAPI/vtkDataSetToWsLocation.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLDefinitions.h"
#include "MantidVatesAPI/FieldDataToMetadata.h"
#include "MantidVatesAPI/VatesXMLDefinitions.h"
#include <Poco/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <vtkDataSet.h>

namespace Mantid {
namespace VATES {
/**
Static creational method to run functionality in one method call.
@param dataset : input dataset containing field data.
@return location string.
*/
std::string vtkDataSetToWsLocation::exec(vtkDataSet *dataset) {
  vtkDataSetToWsLocation temp(dataset);
  return temp.execute();
}

/**
Constructor
@param dataSet : input dataset containing field data.
*/
vtkDataSetToWsLocation::vtkDataSetToWsLocation(vtkDataSet *dataSet)
    : m_dataset(dataSet) {
  if (!m_dataset) {
    throw std::runtime_error(
        "Tried to construct vtkDataSetToWsLocation with NULL vtkDataSet");
  }
}

/**
Execution method to run the extraction.
@return location string
*/
std::string vtkDataSetToWsLocation::execute() {
  using Mantid::Geometry::MDGeometryXMLDefinitions;
  FieldDataToMetadata convert;
  std::string xmlString =
      convert(m_dataset->GetFieldData(), XMLDefinitions::metaDataId());

  Poco::XML::DOMParser pParser;
  Poco::AutoPtr<Poco::XML::Document> pDoc = pParser.parseString(xmlString);
  Poco::XML::Element *pRootElem = pDoc->documentElement();
  Poco::XML::Element *wsLocationElem = pRootElem->getChildElement(
      MDGeometryXMLDefinitions::workspaceLocationElementName());
  if (!wsLocationElem) {
    throw std::runtime_error(
        "The element containing the workspace location must be present.");
  }
  return wsLocationElem->innerText();
}

/// Destructor
vtkDataSetToWsLocation::~vtkDataSetToWsLocation() {}
} // namespace VATES
} // namespace Mantid
