// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VATESAPI_TEST_MOCKOBJECTS_H
#define VATESAPI_TEST_MOCKOBJECTS_H

//#include "MantidMDAlgorithms/CreateMDWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/NullCoordTransform.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/UnitLabel.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidVatesAPI/Common.h"
#include "MantidVatesAPI/MDLoadingView.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/VatesXMLDefinitions.h"
#include "MantidVatesAPI/WorkspaceProvider.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include <gmock/gmock.h>
#include <vtkCharArray.h>
#include <vtkFieldData.h>
#include <vtkStringArray.h>

using Mantid::Geometry::MDHistoDimension;
using Mantid::Geometry::MDHistoDimension_sptr;
using Mantid::coord_t;

//=====================================================================================
// Test Helper Types. These are shared by several tests in VatesAPI
//=====================================================================================
namespace {

const int dimension_size = 9;

//=================================================================================================
/// Helper class. Concrete instance of IMDDimension.
class FakeIMDDimension : public Mantid::Geometry::IMDDimension {
private:
  std::string m_id;
  const unsigned int m_nbins;

public:
  FakeIMDDimension(std::string id, unsigned int nbins = 10)
      : m_id(id), m_nbins(nbins) {}
  std::string getName() const override {
    throw std::runtime_error("Not implemented");
  }
  const Mantid::Kernel::UnitLabel getUnits() const override {
    throw std::runtime_error("Not implemented");
  }
  const std::string &getDimensionId() const override { return m_id; }
  coord_t getMaximum() const override { return 10; }
  coord_t getMinimum() const override { return 0; };
  size_t getNBins() const override { return m_nbins; };
  size_t getNBoundaries() const override { return m_nbins + 1; };
  std::string toXMLString() const override {
    throw std::runtime_error("Not implemented");
  };
  coord_t getX(size_t) const override {
    throw std::runtime_error("Not implemented");
  };
  void setRange(size_t /*nBins*/, coord_t /*min*/, coord_t /*max*/) override{};
  ~FakeIMDDimension() override {}
};

GNU_DIAG_OFF_SUGGEST_OVERRIDE
//=================================================================================================
/// Concrete mocked implementation of IMDWorkspace for testing.
class MockIMDWorkspace : public Mantid::API::IMDWorkspace {
public:
  MOCK_CONST_METHOD0(id, const std::string());
  MOCK_CONST_METHOD0(getMemorySize, size_t());
  MOCK_CONST_METHOD0(getGeometryXML, std::string());
  MOCK_CONST_METHOD0(getNPoints, uint64_t());
  MOCK_CONST_METHOD0(getNEvents, uint64_t());
  MOCK_CONST_METHOD1(getSignalNormalizedAt, Mantid::signal_t(size_t index1));
  MOCK_CONST_METHOD2(getSignalNormalizedAt,
                     double(size_t index1, size_t index2));
  MOCK_CONST_METHOD3(getSignalNormalizedAt,
                     double(size_t index1, size_t index2, size_t index3));
  MOCK_CONST_METHOD4(getSignalNormalizedAt,
                     double(size_t index1, size_t index2, size_t index3,
                            size_t index4));
  MOCK_CONST_METHOD0(getNonIntegratedDimensions,
                     Mantid::Geometry::VecIMDDimension_const_sptr());
  MOCK_METHOD1(mockSetMDMasking,
               void(Mantid::Geometry::MDImplicitFunction *ptr));
  void setMDMasking(std::unique_ptr<Mantid::Geometry::MDImplicitFunction> ptr) {
    mockSetMDMasking(ptr.get());
  }
  MOCK_METHOD0(clearMDMasking, void());
  MOCK_CONST_METHOD0(getSpecialCoordinateSystem,
                     Mantid::Kernel::SpecialCoordinateSystem());
  MOCK_CONST_METHOD0(hasOrientedLattice, bool());

  Mantid::API::IMDWorkspace::LinePlot
  getLinePlot(const Mantid::Kernel::VMD &, const Mantid::Kernel::VMD &,
              Mantid::API::MDNormalization) const override {
    LinePlot line;
    return line;
  }

  std::vector<std::unique_ptr<Mantid::API::IMDIterator>> createIterators(
      size_t = 1,
      Mantid::Geometry::MDImplicitFunction * = NULL) const override {
    throw std::runtime_error("Not Implemented");
  }

  Mantid::signal_t
  getSignalAtCoord(const Mantid::coord_t *,
                   const Mantid::API::MDNormalization &) const override {
    return 0;
  }

  Mantid::signal_t getSignalWithMaskAtCoord(
      const Mantid::coord_t *,
      const Mantid::API::MDNormalization &) const override {
    return 0;
  }

  MockIMDWorkspace() : IMDWorkspace() {}

  ~MockIMDWorkspace() override {}

private:
  MockIMDWorkspace *doClone() const override {
    throw std::runtime_error("Cloning of MockIMDWorkspace is not implemented.");
  }
  MockIMDWorkspace *doCloneEmpty() const override {
    throw std::runtime_error("Cloning of MockIMDWorkspace is not implemented.");
  }
};

//=================================================================================================
/// Mock to allow the behaviour of the chain of responsibility to be tested.
class MockvtkDataSetFactory : public Mantid::VATES::vtkDataSetFactory {
public:
  /// This is necessary as Google Mock can't mock functions that take
  /// non-copyable params.
  virtual void SetSuccessor(std::unique_ptr<vtkDataSetFactory> successor) {
    setSuccessorProxy(successor.get());
  }
  MOCK_CONST_METHOD1(
      create, vtkSmartPointer<vtkDataSet>(Mantid::VATES::ProgressAction &));
  MOCK_CONST_METHOD0(createMeshOnly, vtkDataSet *());
  MOCK_CONST_METHOD0(createScalarArray, vtkFloatArray *());
  MOCK_METHOD1(initialize, void(const Mantid::API::Workspace_sptr &));
  MOCK_METHOD1(setSuccessorProxy, void(vtkDataSetFactory *));
  MOCK_CONST_METHOD0(hasSuccessor, bool());
  MOCK_CONST_METHOD0(validate, void());
  MOCK_CONST_METHOD0(getFactoryTypeName, std::string());
  MOCK_METHOD1(setRecursionDepth, void(size_t));
};

//=================================================================================================
/// Mock to allow the behaviour of MDLoadingPresenters to be tested.
class MockMDLoadingView : public Mantid::VATES::MDLoadingView {
public:
  MOCK_CONST_METHOD0(getTime, double());
  MOCK_CONST_METHOD0(getRecursionDepth, size_t());
  MOCK_CONST_METHOD0(getLoadInMemory, bool());
  MOCK_METHOD2(updateAlgorithmProgress, void(double, const std::string &));
  ~MockMDLoadingView() override {}
};

class MockWorkspaceProvider : public Mantid::VATES::WorkspaceProvider {
public:
  MOCK_CONST_METHOD1(canProvideWorkspace, bool(std::string));
  MOCK_CONST_METHOD1(fetchWorkspace, Mantid::API::Workspace_sptr(std::string));
  MOCK_CONST_METHOD1(disposeWorkspace, void(std::string));
  ~MockWorkspaceProvider() override {}
};

class MockProgressAction : public Mantid::VATES::ProgressAction {
public:
  MOCK_METHOD1(eventRaised, void(double));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class FakeProgressAction : public Mantid::VATES::ProgressAction {
  void eventRaised(double) override {}
};

/**
Create a field data entry containing (as contents) the argument text.
@param testData : Text to enter
@return new vtkFieldData object containing text.
*/
GNU_UNUSED_FUNCTION vtkFieldData *
createFieldDataWithCharArray(std::string testData) {
  vtkFieldData *fieldData = vtkFieldData::New();
  vtkCharArray *charArray = vtkCharArray::New();
  charArray->SetName(Mantid::VATES::XMLDefinitions::metaDataId().c_str());
  charArray->Allocate(100);
  for (unsigned int i = 0; i < testData.size(); i++) {
    char cNextVal = testData.at(i);
    if (int(cNextVal) > 1) {
      charArray->InsertNextValue(cNextVal);
    }
  }
  fieldData->AddArray(charArray);
  charArray->Delete();
  return fieldData;
}

/**
Construct an example Geometry section of the XML passed via field-data.

Note that this function doesn't give complete control over the geometry. For
example, the Upper and Lower bounds are hard-coded.

@param xDimensionIdMapping : Dimension name for dimension to be used as the
x-dimension in the view.
@param yDimensionIdMapping : Dimension name for dimension y-dimension in the
view.
@param zDimensionIdMapping : Dimension name for dimension z-dimension in the
view.
@param tDimensionIdMapping : Dimension name for dimension t-dimension in the
view.
@param xBins : number of bins in the x dimension
@param yBins : number of bins in the x dimension
@param zBins : number of bins in the x dimension
@param tBins : number of bins in the x dimension
@return xml snippet as string.
*/
std::string constrctGeometryOnlyXML(const std::string &xDimensionIdMapping,
                                    const std::string &yDimensionIdMapping,
                                    const std::string &zDimensionIdMapping,
                                    const std::string &tDimensionIdMapping,
                                    std::string xBins = "10",
                                    std::string yBins = "10",
                                    std::string zBins = "10",
                                    std::string tBins = "10") {
  std::string body =
      std::string("<DimensionSet>") + "<Dimension ID=\"en\">" +
      "<Name>Energy</Name>" + "<Units>None</Units>" +
      "<UpperBounds>150.0000</UpperBounds>" +
      "<LowerBounds>0.0000</LowerBounds>" + "<NumberOfBins>" + xBins +
      "</NumberOfBins>" + "</Dimension>" + "<Dimension ID=\"qx\">" +
      "<Name>Qx</Name>" + "<Units>None</Units>" +
      "<UpperBounds>5.0000</UpperBounds>" +
      "<LowerBounds>-1.5000</LowerBounds>" + "<NumberOfBins>" + yBins +
      "</NumberOfBins>" + "</Dimension>" + "<Dimension ID=\"qy\">" +
      "<Name>Qy</Name>" + "<Units>None</Units>" +
      "<UpperBounds>6.6000</UpperBounds>" +
      "<LowerBounds>-6.6000</LowerBounds>" + "<NumberOfBins>" + zBins +
      "</NumberOfBins>" + "</Dimension>" + "<Dimension ID=\"qz\">" +
      "<Name>Qz</Name>" + "<Units>None</Units>" +
      "<UpperBounds>6.6000</UpperBounds>" +
      "<LowerBounds>-6.6000</LowerBounds>" + "<NumberOfBins>" + tBins +
      "</NumberOfBins>" + "</Dimension>" + "<XDimension>" + "<RefDimensionId>" +
      xDimensionIdMapping + "</RefDimensionId>" + "</XDimension>" +
      "<YDimension>" + "<RefDimensionId>" + yDimensionIdMapping +
      "</RefDimensionId>" + "</YDimension>" + "<ZDimension>" +
      "<RefDimensionId>" + zDimensionIdMapping + "</RefDimensionId>" +
      "</ZDimension>" + "<TDimension>" + "<RefDimensionId>" +
      tDimensionIdMapping + "</RefDimensionId>" + "</TDimension>" +
      "</DimensionSet>";
  return body;
}

/**
Construct test xml describing the transformations and the inputs.
@param xDimensionIdMapping : Dimension name for dimension to be used as the
x-dimension in the view.
@param yDimensionIdMapping : Dimension name for dimension y-dimension in the
view.
@param zDimensionIdMapping : Dimension name for dimension z-dimension in the
view.
@param tDimensionIdMapping : Dimension name for dimension t-dimension in the
view.
@return full xml as string.
*/
GNU_UNUSED_FUNCTION std::string
constructXML(const std::string &xDimensionIdMapping,
             const std::string &yDimensionIdMapping,
             const std::string &zDimensionIdMapping,
             const std::string &tDimensionIdMapping) {
  return std::string("<?xml version=\"1.0\" encoding=\"utf-8\"?>") +
         "<MDInstruction>" + "<MDWorkspaceName>Input</MDWorkspaceName>" +
         "<MDWorkspaceLocation>test_horace_reader.sqw</MDWorkspaceLocation>" +
         constrctGeometryOnlyXML(xDimensionIdMapping, yDimensionIdMapping,
                                 zDimensionIdMapping, tDimensionIdMapping) +
         "</MDInstruction>";
}

/**
Construct an example Geometry section of the XML passed via field-data.

Note that this function doesn't give complete control over the geometry. For
example, the Upper and Lower bounds are hard-coded.

@param xDimensionIdMapping : Dimension name for dimension to be used as the
x-dimension in the view.
@param yDimensionIdMapping : Dimension name for dimension y-dimension in the
view.
@param zDimensionIdMapping : Dimension name for dimension z-dimension in the
view.
@param tDimensionIdMapping : Dimension name for dimension t-dimension in the
view.
@param xBins : number of bins in the x dimension
@param yBins : number of bins in the x dimension
@param zBins : number of bins in the x dimension
@param tBins : number of bins in the x dimension
@return xml snippet as string.
*/
std::string constructGeometryOnlyXMLForMDEvHelperData(
    const std::string &xDimensionIdMapping,
    const std::string &yDimensionIdMapping,
    const std::string &zDimensionIdMapping,
    const std::string &tDimensionIdMapping, std::string xBins = "10",
    std::string yBins = "10", std::string zBins = "10",
    std::string tBins = "10") {
  std::string cardDirSpec =
      std::string("<DimensionSet>") + "<Dimension ID=\"Axis0\">" +
      "<Name>Axis0</Name>" + "<Units>m</Units>" +
      "<UpperBounds>10.0000</UpperBounds>" +
      "<LowerBounds>0.0000</LowerBounds>" + "<NumberOfBins>" + xBins +
      "</NumberOfBins>" + "</Dimension>" + "<Dimension ID=\"Axis1\">" +
      "<Name>Axis1</Name>" + "<Units>m</Units>" +
      "<UpperBounds>10.0000</UpperBounds>" +
      "<LowerBounds>0.0000</LowerBounds>" + "<NumberOfBins>" + yBins +
      "</NumberOfBins>" + "</Dimension>" + "<Dimension ID=\"Axis2\">" +
      "<Name>Axis2</Name>" + "<Units>m</Units>" +
      "<UpperBounds>10.0000</UpperBounds>" +
      "<LowerBounds>0.0000</LowerBounds>" + "<NumberOfBins>" + zBins +
      "</NumberOfBins>" + "</Dimension>";
  std::string timeSpec;
  if (!tDimensionIdMapping.empty()) {
    timeSpec = std::string("<Dimension ID=\"Axis3\">") + "<Name>Axis3</Name>" +
               "<Units>s</Units>" + "<UpperBounds>10.0000</UpperBounds>" +
               "<LowerBounds>0.0000</LowerBounds>" + "<NumberOfBins>" + tBins +
               "</NumberOfBins>" + "</Dimension>";
  }
  std::string cardDirRef =
      std::string("<XDimension>") + "<RefDimensionId>" + xDimensionIdMapping +
      "</RefDimensionId>" + "</XDimension>" + "<YDimension>" +
      "<RefDimensionId>" + yDimensionIdMapping + "</RefDimensionId>" +
      "</YDimension>" + "<ZDimension>" + "<RefDimensionId>" +
      zDimensionIdMapping + "</RefDimensionId>" + "</ZDimension>";
  std::string timeRef;
  if (!tDimensionIdMapping.empty()) {
    timeRef = std::string("<TDimension>") + "<RefDimensionId>" +
              tDimensionIdMapping + "</RefDimensionId>" + "</TDimension>";
  } else {
    timeRef = std::string("<TDimension>") + "<RefDimensionId>" +
              "</RefDimensionId>" + "</TDimension>";
  }
  std::string endTag = "</DimensionSet>";
  std::string body = cardDirSpec;
  if (!timeSpec.empty()) {
    body += timeSpec;
  }
  body += cardDirRef;
  body += timeRef;
  body += endTag;
  return body;
}

/**
Construct test xml describing the transformations and the inputs.
@param xDimensionIdMapping : Dimension name for dimension to be used as the
x-dimension in the view.
@param yDimensionIdMapping : Dimension name for dimension y-dimension in the
view.
@param zDimensionIdMapping : Dimension name for dimension z-dimension in the
view.
@param tDimensionIdMapping : Dimension name for dimension t-dimension in the
view.
@return full xml as string.
*/
GNU_UNUSED_FUNCTION std::string
constructXMLForMDEvHelperData(const std::string &xDimensionIdMapping,
                              const std::string &yDimensionIdMapping,
                              const std::string &zDimensionIdMapping,
                              const std::string &tDimensionIdMapping) {
  return std::string("<?xml version=\"1.0\" encoding=\"utf-8\"?>") +
         "<MDInstruction>" + "<MDWorkspaceName>Input</MDWorkspaceName>" +
         "<MDWorkspaceLocation>test_horace_reader.sqw</MDWorkspaceLocation>" +
         constructGeometryOnlyXMLForMDEvHelperData(
             xDimensionIdMapping, yDimensionIdMapping, zDimensionIdMapping,
             tDimensionIdMapping) +
         "</MDInstruction>";
}

Mantid::API::Workspace_sptr createSimple3DWorkspace() {
  using namespace Mantid::API;

  IAlgorithm *create =
      FrameworkManager::Instance().createAlgorithm("CreateMDWorkspace");
  create->setChild(true);
  create->initialize();
  create->setProperty("Dimensions", 4);
  create->setPropertyValue("Extents", "0,5,0,5,0,5,0,5");
  create->setPropertyValue("Names", "A,B,C,D");
  create->setPropertyValue("Units", "A,A,A,A");
  create->setPropertyValue("OutputWorkspace", "dummy");
  create->execute();
  Workspace_sptr outWs = create->getProperty("OutputWorkspace");
  return outWs;
}

GNU_UNUSED_FUNCTION Mantid::API::Workspace_sptr
get3DWorkspace(bool integratedTDimension, bool sliceMD) {
  using namespace Mantid::API;
  using namespace Mantid::DataObjects;

  Mantid::API::Workspace_sptr inputWs = createSimple3DWorkspace();

  std::string binningAlgName;
  if (sliceMD) {
    binningAlgName = "SliceMD";
  } else {
    binningAlgName = "BinMD";
  }
  IAlgorithm_sptr binningAlg =
      AlgorithmManager::Instance().createUnmanaged(binningAlgName);
  binningAlg->setChild(true);
  binningAlg->initialize();
  binningAlg->setProperty("InputWorkspace", inputWs);
  binningAlg->setPropertyValue("AlignedDim0", "A,0,5,2");
  binningAlg->setPropertyValue("AlignedDim1", "B,0,5,2");
  binningAlg->setPropertyValue("AlignedDim2", "C,0,5,2");

  if (integratedTDimension) {
    binningAlg->setPropertyValue("AlignedDim3", "D,0,5,1");
  } else {
    binningAlg->setPropertyValue("AlignedDim3", "D,0,5,2");
  }
  binningAlg->setPropertyValue("OutputWorkspace", "dummy");
  binningAlg->execute();
  Workspace_sptr outWs = binningAlg->getProperty("OutputWorkspace");
  return outWs;
}

/**
 * Get a string array from a particular field data entry in a vtkDataSet.
 * @param ds : The dataset to retrieve the field data from
 * @param fieldName : The requested field data entry
 * @return The value of the requested field data entry
 */
GNU_UNUSED_FUNCTION std::string getStringFieldDataValue(vtkDataSet *ds,
                                                        std::string fieldName) {
  vtkAbstractArray *value =
      ds->GetFieldData()->GetAbstractArray(fieldName.c_str());
  vtkStringArray *array = vtkStringArray::SafeDownCast(value);
  return static_cast<std::string>(array->GetValue(0));
}

} // namespace

#endif
