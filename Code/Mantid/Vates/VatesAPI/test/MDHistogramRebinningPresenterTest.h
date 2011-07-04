#ifndef MD_HISTOGRAM_REBINNING_PRESENTER_TEST_H_
#define MD_HISTOGRAM_REBINNING_PRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h> 

#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidVatesAPI/MDHistogramRebinningPresenter.h"
#include "MantidVatesAPI/MDRebinningView.h"
#include "MantidVatesAPI/Clipper.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidVatesAPI/RebinningActionManager.h"
#include "MantidVatesAPI/Common.h"
#include "MantidVatesAPI/ProgressAction.h"

#include <vtkUnstructuredGrid.h>
#include <vtkFieldData.h>
#include <vtkCharArray.h>
#include <vtkFieldData.h>
#include <vtkPoints.h>
#include <vtkCellData.h>
#include "MantidAPI/Workspace.h"

using namespace Mantid::VATES;
using namespace testing;

class MDHistogramRebinningPresenterTest : public CxxTest::TestSuite
{

private:

  class MockMDRebinningView : public MDRebinningView 
  {
  public:
    MOCK_CONST_METHOD0(getImplicitFunction,
      vtkImplicitFunction*());
    MOCK_CONST_METHOD0(getMaxThreshold,
      double());
    MOCK_CONST_METHOD0(getMinThreshold,
      double());
    MOCK_CONST_METHOD0(getApplyClip,
      bool());
    MOCK_CONST_METHOD0(getTimeStep,
      double());
    MOCK_CONST_METHOD0(getAppliedGeometryXML,
      const char*());
    MOCK_METHOD1(updateAlgorithmProgress,
      void(double));
  };

  class MockClipper: public Mantid::VATES::Clipper
  {
  public:
    MOCK_METHOD1(SetInput, void(vtkDataSet* in_ds));
    MOCK_METHOD1(SetClipFunction, void(vtkImplicitFunction* func));
    MOCK_METHOD1(SetInsideOut, void(bool insideout));
    MOCK_METHOD1(SetRemoveWholeCells, void(bool removeWholeCells));
    MOCK_METHOD1(SetOutput, void(vtkUnstructuredGrid* out_ds));
    MOCK_METHOD0(Update, void());
    MOCK_METHOD0(Delete,void());
    MOCK_METHOD0(GetOutput, vtkDataSet*());
    MOCK_METHOD0(die, void());
    virtual ~MockClipper(){}
  };

  class MockvtkDataSetFactory : public Mantid::VATES::vtkDataSetFactory 
  {
  public:
    MOCK_CONST_METHOD0(create,
      vtkDataSet*());
    MOCK_CONST_METHOD0(createMeshOnly,
      vtkDataSet*());
    MOCK_CONST_METHOD0(createScalarArray,
      vtkFloatArray*());
    MOCK_METHOD1(initialize,
      void(boost::shared_ptr<Mantid::API::Workspace>));
    MOCK_CONST_METHOD0(validate,
      void());
    MOCK_CONST_METHOD0(getFactoryTypeName, std::string());
    virtual ~MockvtkDataSetFactory(){}
  };

  class MockRebinningActionManager : public Mantid::VATES::RebinningActionManager
  {
  public:
    MOCK_METHOD1(ask, void(Mantid::VATES::RebinningIterationAction));
    MOCK_CONST_METHOD0(action, RebinningIterationAction());
    MOCK_METHOD0(reset, void());
    virtual ~MockRebinningActionManager(){}
  };

  class FakeProgressAction : public ProgressAction
  {
    virtual void eventRaised(double e)
    {
      e = 1;
    }
  };

  static vtkFieldData* createFieldDataWithCharArray(std::string testData)
  {
    vtkFieldData* fieldData = vtkFieldData::New();
    vtkCharArray* charArray = vtkCharArray::New();
    charArray->SetName(Mantid::VATES::XMLDefinitions::metaDataId().c_str());
    charArray->Allocate(100);
    for(unsigned int i = 0; i < testData.size(); i++)
    {
      char cNextVal = testData.at(i);
      if(int(cNextVal) > 1)
      {
        charArray->InsertNextValue(cNextVal);

      }
    }
    fieldData->AddArray(charArray);
    charArray->Delete();
    return fieldData;
  }

  static std::string constrctGeometryOnlyXML(const std::string& xDimensionIdMapping, const std::string& yDimensionIdMapping, const std::string& zDimensionIdMapping, const std::string& tDimensionIdMapping
    ,std::string xBins = "10",
    std::string yBins = "10",
    std::string zBins = "10",
    std::string tBins = "10"
    )
  {
    std::string body = std::string("<DimensionSet>") +
      "<Dimension ID=\"en\">" +
      "<Name>Energy</Name>" +
      "<UpperBounds>150</UpperBounds>" +
      "<LowerBounds>0</LowerBounds>" +
      "<NumberOfBins>" + xBins + "</NumberOfBins>" +
      "</Dimension>" +
      "<Dimension ID=\"qx\">" +
      "<Name>Qx</Name>" +
      "<UpperBounds>5</UpperBounds>" +
      "<LowerBounds>-1.5</LowerBounds>" +
      "<NumberOfBins>" + yBins + "</NumberOfBins>" +
      "</Dimension>" +
      "<Dimension ID=\"qy\">" +
      "<Name>Qy</Name>" +
      "<UpperBounds>6.6</UpperBounds>" +
      "<LowerBounds>-6.6</LowerBounds>" +
      "<NumberOfBins>" + zBins  + "</NumberOfBins>" +
      "</Dimension>" +
      "<Dimension ID=\"qz\">" +
      "<Name>Qz</Name>" +
      "<UpperBounds>6.6</UpperBounds>" +
      "<LowerBounds>-6.6</LowerBounds>" +
      "<NumberOfBins>" + tBins + "</NumberOfBins>" +
      "</Dimension>" +
      "<XDimension>" +
      "<RefDimensionId>" +
      xDimensionIdMapping +
      "</RefDimensionId>" +
      "</XDimension>" +
      "<YDimension>" +
      "<RefDimensionId>" +
      yDimensionIdMapping +
      "</RefDimensionId>" +
      "</YDimension>" +
      "<ZDimension>" +
      "<RefDimensionId>" + 
      zDimensionIdMapping +
      "</RefDimensionId>" +
      "</ZDimension>" +
      "<TDimension>" +
      "<RefDimensionId>" +
      tDimensionIdMapping +
      "</RefDimensionId>" +
      "</TDimension>" +
      "</DimensionSet>";
    return body;
  }

  static std::string constructXML(const std::string& xDimensionIdMapping, const std::string& yDimensionIdMapping, const std::string& zDimensionIdMapping, const std::string& tDimensionIdMapping)
  {
    return std::string("<?xml version=\"1.0\" encoding=\"utf-8\"?>") +
      "<MDInstruction>" +
      "<MDWorkspaceName>Input</MDWorkspaceName>" +
      "<MDWorkspaceLocation>test_horace_reader.sqw</MDWorkspaceLocation>" +
      constrctGeometryOnlyXML(xDimensionIdMapping, yDimensionIdMapping, zDimensionIdMapping, tDimensionIdMapping) +
      "<Function>" +
      "<Type>CompositeImplicitFunction</Type>" +
      "<ParameterList/>" +
      "<Function>" +
      "<Type>BoxImplicitFunction</Type>" +
      "<ParameterList>" +
      "<Parameter>" +
      "<Type>HeightParameter</Type>" +
      "<Value>6</Value>" +
      "</Parameter>" +
      "<Parameter>" +
      "<Type>WidthParameter</Type>" +
      "<Value>1.5</Value>" +
      "</Parameter>" +
      "<Parameter>" +
      "<Type>DepthParameter</Type>" +
      "<Value>6</Value>" +
      "</Parameter>" +
      "<Parameter>" +
      "<Type>OriginParameter</Type>" +
      "<Value>0, 0, 0</Value>" +
      "</Parameter>" +
      "</ParameterList>" +
      "</Function>" +
      "<Function>" +
      "<Type>CompositeImplicitFunction</Type>" +
      "<ParameterList/>" +
      "<Function>" +
      "<Type>BoxImplicitFunction</Type>" +
      "<ParameterList>" +
      "<Parameter>" +
      "<Type>WidthParameter</Type>" +
      "<Value>4</Value>" +
      "</Parameter>" +
      "<Parameter>" +
      "<Type>HeightParameter</Type>" +
      "<Value>1.5</Value>" +
      "</Parameter>" +
      "<Parameter>" +
      "<Type>DepthParameter</Type>" +
      "<Value>6</Value>" +
      "</Parameter>" +
      "<Parameter>" +
      "<Type>OriginParameter</Type>" +
      "<Value>0, 0, 0</Value>" +
      "</Parameter>" +
      "</ParameterList>" +
      "</Function>" +
      "</Function>" +
      "</Function>" +
      "</MDInstruction>";
  }

public:
  

  void testConstruction()
  {
    MockMDRebinningView view;
    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    MockClipper* pClipper = new MockClipper;

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDHistogramRebinningPresenter<MockMDRebinningView> presenter(dataSet, pRequest, &view, pClipper);
    TSM_ASSERT("Geometry should be available immediately after construction.", !presenter.getAppliedGeometryXML().empty());
  }

  void testUpdateModelWithNoChanges()
  {
    MockMDRebinningView view;
    EXPECT_CALL(view, getTimeStep()).WillOnce(Return(0));
    EXPECT_CALL(view, getMaxThreshold()).WillOnce(Return(0));
    EXPECT_CALL(view, getMinThreshold()).WillOnce(Return(0));
    EXPECT_CALL(view, getApplyClip()).WillOnce(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en");
    EXPECT_CALL(view, getAppliedGeometryXML()).WillOnce(Return(viewXML.c_str()));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateAll)).Times(0); //Since nothing has changed, no requests should be made.
    
    MockClipper* pClipper = new MockClipper;

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDHistogramRebinningPresenter<MockMDRebinningView> presenter(dataSet, pRequest, &view, pClipper);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithDifferentMaxThreshold()
  {
    MockMDRebinningView view;
    EXPECT_CALL(view, getTimeStep()).WillOnce(Return(0));
    EXPECT_CALL(view, getMaxThreshold()).Times(2).WillRepeatedly(Return(1)); //Maxthreshold non-zero
    EXPECT_CALL(view, getMinThreshold()).WillOnce(Return(0));
    EXPECT_CALL(view, getApplyClip()).WillOnce(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en");
    EXPECT_CALL(view, getAppliedGeometryXML()).WillOnce(Return(viewXML.c_str()));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateVisualDataSetOnly)).Times(1); //Maxthreshold updated should reflect on request.
    
    MockClipper* pClipper = new MockClipper;

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDHistogramRebinningPresenter<MockMDRebinningView> presenter(dataSet, pRequest, &view, pClipper);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithDifferentMinThreshold()
  {
    MockMDRebinningView view;
    EXPECT_CALL(view, getTimeStep()).WillOnce(Return(0));
    EXPECT_CALL(view, getMaxThreshold()).WillOnce(Return(0)); 
    EXPECT_CALL(view, getMinThreshold()).Times(2).WillRepeatedly(Return(1)); //Minthreshold non-zero
    EXPECT_CALL(view, getApplyClip()).WillOnce(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en");
    EXPECT_CALL(view, getAppliedGeometryXML()).WillOnce(Return(viewXML.c_str()));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateVisualDataSetOnly)).Times(1); //Minthreshold updated should reflect on request.
    
    MockClipper* pClipper = new MockClipper;

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDHistogramRebinningPresenter<MockMDRebinningView> presenter(dataSet, pRequest, &view, pClipper);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithDifferentTimestep()
  {
    MockMDRebinningView view;
    EXPECT_CALL(view, getTimeStep()).Times(2).WillRepeatedly(Return(1)); //Timestep updated
    EXPECT_CALL(view, getMaxThreshold()).WillOnce(Return(0));
    EXPECT_CALL(view, getMinThreshold()).WillOnce(Return(0));
    EXPECT_CALL(view, getApplyClip()).WillOnce(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en");
    EXPECT_CALL(view, getAppliedGeometryXML()).WillOnce(Return(viewXML.c_str()));
   
    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateVisualDataSetOnly)).Times(1); //Timestep updated should reflect on request.

    MockClipper* pClipper = new MockClipper;

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDHistogramRebinningPresenter<MockMDRebinningView> presenter(dataSet, pRequest, &view, pClipper);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithSwapped4DGeometry()
  {
    MockMDRebinningView view;
    EXPECT_CALL(view, getTimeStep()).WillOnce(Return(0));
    EXPECT_CALL(view, getMaxThreshold()).WillOnce(Return(0));
    EXPECT_CALL(view, getMinThreshold()).WillOnce(Return(0));
    EXPECT_CALL(view, getApplyClip()).WillOnce(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "en", "qz");
    EXPECT_CALL(view, getAppliedGeometryXML()).Times(3).WillRepeatedly(Return(viewXML.c_str())); //Geometry (4D) should reflect on request.
   
    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateVisualDataSetOnly)).Times(1); //Swapping request should be made.

    MockClipper* pClipper = new MockClipper;

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDHistogramRebinningPresenter<MockMDRebinningView> presenter(dataSet, pRequest, &view, pClipper);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithMoreXBins()
  {
    MockMDRebinningView view;
    EXPECT_CALL(view, getTimeStep()).WillOnce(Return(0));
    EXPECT_CALL(view, getMaxThreshold()).WillOnce(Return(0));
    EXPECT_CALL(view, getMinThreshold()).WillOnce(Return(0));
    EXPECT_CALL(view, getApplyClip()).WillOnce(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en", "11");
    EXPECT_CALL(view, getAppliedGeometryXML()).Times(3).WillRepeatedly(Return(viewXML.c_str())); //Geometry (4D) should reflect on request.
   
    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateVisualDataSetOnly)).Times(1); //From standard 4D swapping
    EXPECT_CALL(*pRequest, ask(RecalculateAll)).Times(1); //Nxbins changed, requires rebin request.

    MockClipper* pClipper = new MockClipper;

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDHistogramRebinningPresenter<MockMDRebinningView> presenter(dataSet, pRequest, &view, pClipper);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithMoreXYBins()
  {
    MockMDRebinningView view;
    EXPECT_CALL(view, getTimeStep()).WillOnce(Return(0));
    EXPECT_CALL(view, getMaxThreshold()).WillOnce(Return(0));
    EXPECT_CALL(view, getMinThreshold()).WillOnce(Return(0));
    EXPECT_CALL(view, getApplyClip()).WillOnce(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en", "11", "11");
    EXPECT_CALL(view, getAppliedGeometryXML()).Times(3).WillRepeatedly(Return(viewXML.c_str()));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateVisualDataSetOnly)).Times(1); //From standard 4D swapping
    EXPECT_CALL(*pRequest, ask(RecalculateAll)).Times(2); //Nxbins & Nybins changed, requires rebin request.

    MockClipper* pClipper = new MockClipper;

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDHistogramRebinningPresenter<MockMDRebinningView> presenter(dataSet, pRequest, &view, pClipper);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  
  void testUpdateModelWithMoreXYZBins()
  {
    MockMDRebinningView view;
    EXPECT_CALL(view, getTimeStep()).WillOnce(Return(0));
    EXPECT_CALL(view, getMaxThreshold()).WillOnce(Return(0));
    EXPECT_CALL(view, getMinThreshold()).WillOnce(Return(0));
    EXPECT_CALL(view, getApplyClip()).WillRepeatedly(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en", "11", "11", "11");
    EXPECT_CALL(view, getAppliedGeometryXML()).Times(3).WillRepeatedly(Return(viewXML.c_str()));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateVisualDataSetOnly)).Times(1); //From standard 4D swapping
    EXPECT_CALL(*pRequest, ask(RecalculateAll)).Times(3); //Nxbins & Nybins & Nzbins changed, requires rebin request.

    MockClipper* pClipper = new MockClipper;

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDHistogramRebinningPresenter<MockMDRebinningView> presenter(dataSet, pRequest, &view, pClipper);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

      
  void testExecutionWithFullRebin()
  {
    /*****
     Test that it executes and fully rebins on first pass, then that it does not rebin again on the second pass.
    ****/

    MockMDRebinningView view;
    EXPECT_CALL(view, getTimeStep()).WillOnce(Return(0)); //NoChange
    EXPECT_CALL(view, getMaxThreshold()).WillOnce(Return(0)); //NoChange
    EXPECT_CALL(view, getMinThreshold()).WillOnce(Return(0)); //NoChange
    EXPECT_CALL(view, getApplyClip()).WillOnce(Return(false)); //NoChange
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en");
    EXPECT_CALL(view, getAppliedGeometryXML()).Times(2).WillRepeatedly(Return(viewXML.c_str())); // NoChange

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(_)).Times(0);
    EXPECT_CALL(*pRequest, action()).Times(2).WillRepeatedly(Return(RecalculateAll)); //Request is preset to RecalculateAll.
    EXPECT_CALL(*pRequest, reset()).Times(1);

    MockClipper* pClipper = new MockClipper;
    MockvtkDataSetFactory* pDataSetFactory = new MockvtkDataSetFactory;
    EXPECT_CALL(*pDataSetFactory, initialize(_)).Times(1);
    EXPECT_CALL(*pDataSetFactory, create()).WillOnce(Return(vtkUnstructuredGrid::New()));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    FakeProgressAction progressAction;

    MDHistogramRebinningPresenter<MockMDRebinningView> presenter(dataSet, pRequest, &view, pClipper);
    presenter.updateModel();
    vtkDataSet* product = presenter.execute(pDataSetFactory, progressAction);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pDataSetFactory));
    delete pDataSetFactory;
    product->Delete();
  }

  void testExecutionThrowsWhenNoFieldData()
  {
    MockMDRebinningView view;

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, action()).WillRepeatedly(Return(RecalculateAll)); //Request is preset to RecalculateAll.

    MockClipper* pClipper = new MockClipper;

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    //NO FIELD DATA ADDED TO DATASET!

    FakeProgressAction progressAction;

    MDHistogramRebinningPresenter<MockMDRebinningView>* presenter;
    TSM_ASSERT_THROWS("Should not process without field data. Should throw!",presenter = new MDHistogramRebinningPresenter<MockMDRebinningView>(dataSet, pRequest, &view, pClipper), std::logic_error);

  }


};

#endif
