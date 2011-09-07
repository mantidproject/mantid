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
#include <vtkPoints.h>
#include <vtkCellData.h>
#include "MantidAPI/Workspace.h"
#include "MockObjects.h"

using namespace Mantid::VATES;
using namespace testing;

class MDHistogramRebinningPresenterTest : public CxxTest::TestSuite
{

public:
  

  void testConstruction()
  {
    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    MockClipper* pClipper = new MockClipper;

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDHistogramRebinningPresenter presenter(dataSet, pRequest, new MockMDRebinningView, pClipper, wsProvider);
    TSM_ASSERT("Geometry should be available immediately after construction.", !presenter.getAppliedGeometryXML().empty());
  }

  void testConstructionThrowsWhenNoFieldData()
  {
    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, action()).WillRepeatedly(Return(RecalculateAll)); //Request is preset to RecalculateAll.

    MockClipper* pClipper = new MockClipper;

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).Times(0);

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    //NO FIELD DATA ADDED TO DATASET!

    FakeProgressAction progressAction;

    MDHistogramRebinningPresenter* presenter;
    TSM_ASSERT_THROWS("Should not process without field data. Should throw!",presenter = new MDHistogramRebinningPresenter(dataSet, pRequest, new MockMDRebinningView, pClipper, wsProvider), std::logic_error);

  }

  void testConstructionThrowsWhenCannotProvideWorkspace()
  {
    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, action()).WillRepeatedly(Return(RecalculateAll)); //Request is preset to RecalculateAll.

    MockClipper* pClipper = new MockClipper;

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillOnce(Return(false)); //Not yielding a workspace.

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    FakeProgressAction progressAction;

    MDHistogramRebinningPresenter* presenter;
    TSM_ASSERT_THROWS("No workspace provided. Should throw!",presenter = new MDHistogramRebinningPresenter(dataSet, pRequest, new MockMDRebinningView, pClipper, wsProvider), std::invalid_argument);

  }

  void testUpdateModelWithNoChanges()
  {
    MockMDRebinningView* view = new MockMDRebinningView;
    EXPECT_CALL(*view, getTimeStep()).WillOnce(Return(0));
    EXPECT_CALL(*view, getMaxThreshold()).WillOnce(Return(0));
    EXPECT_CALL(*view, getMinThreshold()).WillOnce(Return(0));
    EXPECT_CALL(*view, getApplyClip()).WillOnce(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en");
    EXPECT_CALL(*view, getAppliedGeometryXML()).WillOnce(Return(viewXML.c_str()));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateAll)).Times(0); //Since nothing has changed, no requests should be made.
    
    MockClipper* pClipper = new MockClipper;

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDHistogramRebinningPresenter presenter(dataSet, pRequest, view, pClipper, wsProvider);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithDifferentMaxThreshold()
  {
    MockMDRebinningView* view = new MockMDRebinningView;
    EXPECT_CALL(*view, getTimeStep()).WillOnce(Return(0));
    EXPECT_CALL(*view, getMaxThreshold()).Times(2).WillRepeatedly(Return(1)); //Maxthreshold non-zero
    EXPECT_CALL(*view, getMinThreshold()).WillOnce(Return(0));
    EXPECT_CALL(*view, getApplyClip()).WillOnce(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en");
    EXPECT_CALL(*view, getAppliedGeometryXML()).WillOnce(Return(viewXML.c_str()));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateVisualDataSetOnly)).Times(1); //Maxthreshold updated should reflect on request.
    
    MockClipper* pClipper = new MockClipper;

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDHistogramRebinningPresenter presenter(dataSet, pRequest, view, pClipper, wsProvider);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithDifferentMinThreshold()
  {
    MockMDRebinningView* view =new  MockMDRebinningView;
    EXPECT_CALL(*view, getTimeStep()).WillOnce(Return(0));
    EXPECT_CALL(*view, getMaxThreshold()).WillOnce(Return(0)); 
    EXPECT_CALL(*view, getMinThreshold()).Times(2).WillRepeatedly(Return(1)); //Minthreshold non-zero
    EXPECT_CALL(*view, getApplyClip()).WillOnce(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en");
    EXPECT_CALL(*view, getAppliedGeometryXML()).WillOnce(Return(viewXML.c_str()));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateVisualDataSetOnly)).Times(1); //Minthreshold updated should reflect on request.
    
    MockClipper* pClipper = new MockClipper;

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDHistogramRebinningPresenter presenter(dataSet, pRequest, view, pClipper, wsProvider);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithDifferentTimestep()
  {
    MockMDRebinningView* view =new  MockMDRebinningView;
    EXPECT_CALL(*view, getTimeStep()).Times(2).WillRepeatedly(Return(1)); //Timestep updated
    EXPECT_CALL(*view, getMaxThreshold()).WillOnce(Return(0));
    EXPECT_CALL(*view, getMinThreshold()).WillOnce(Return(0));
    EXPECT_CALL(*view, getApplyClip()).WillOnce(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en");
    EXPECT_CALL(*view, getAppliedGeometryXML()).WillOnce(Return(viewXML.c_str()));
   
    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateVisualDataSetOnly)).Times(1); //Timestep updated should reflect on request.

    MockClipper* pClipper = new MockClipper;
    
    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDHistogramRebinningPresenter presenter(dataSet, pRequest, view, pClipper, wsProvider);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithSwapped4DGeometry()
  {
     MockMDRebinningView* view =new  MockMDRebinningView;
    EXPECT_CALL(*view, getTimeStep()).WillOnce(Return(0));
    EXPECT_CALL(*view, getMaxThreshold()).WillOnce(Return(0));
    EXPECT_CALL(*view, getMinThreshold()).WillOnce(Return(0));
    EXPECT_CALL(*view, getApplyClip()).WillOnce(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "en", "qz");
    EXPECT_CALL(*view, getAppliedGeometryXML()).Times(3).WillRepeatedly(Return(viewXML.c_str())); //Geometry (4D) should reflect on request.
   
    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateVisualDataSetOnly)).Times(1); //Swapping request should be made.

    MockClipper* pClipper = new MockClipper;

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDHistogramRebinningPresenter presenter(dataSet, pRequest, view, pClipper, wsProvider);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithMoreXBins()
  {
     MockMDRebinningView* view =new  MockMDRebinningView;
    EXPECT_CALL(*view, getTimeStep()).WillOnce(Return(0));
    EXPECT_CALL(*view, getMaxThreshold()).WillOnce(Return(0));
    EXPECT_CALL(*view, getMinThreshold()).WillOnce(Return(0));
    EXPECT_CALL(*view, getApplyClip()).WillOnce(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en", "11");
    EXPECT_CALL(*view, getAppliedGeometryXML()).Times(3).WillRepeatedly(Return(viewXML.c_str())); //Geometry (4D) should reflect on request.
   
    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateVisualDataSetOnly)).Times(1); //From standard 4D swapping
    EXPECT_CALL(*pRequest, ask(RecalculateAll)).Times(1); //Nxbins changed, requires rebin request.

    MockClipper* pClipper = new MockClipper;

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDHistogramRebinningPresenter presenter(dataSet, pRequest, view, pClipper, wsProvider);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithMoreXYBins()
  {
    MockMDRebinningView* view = new  MockMDRebinningView;
    EXPECT_CALL(*view, getTimeStep()).WillOnce(Return(0));
    EXPECT_CALL(*view, getMaxThreshold()).WillOnce(Return(0));
    EXPECT_CALL(*view, getMinThreshold()).WillOnce(Return(0));
    EXPECT_CALL(*view, getApplyClip()).WillOnce(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en", "11", "11");
    EXPECT_CALL(*view, getAppliedGeometryXML()).Times(3).WillRepeatedly(Return(viewXML.c_str()));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateVisualDataSetOnly)).Times(1); //From standard 4D swapping
    EXPECT_CALL(*pRequest, ask(RecalculateAll)).Times(2); //Nxbins & Nybins changed, requires rebin request.

    MockClipper* pClipper = new MockClipper;

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDHistogramRebinningPresenter presenter(dataSet, pRequest, view, pClipper, wsProvider);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  
  void testUpdateModelWithMoreXYZBins()
  {
    MockMDRebinningView* view = new  MockMDRebinningView;
    EXPECT_CALL(*view, getTimeStep()).WillOnce(Return(0));
    EXPECT_CALL(*view, getMaxThreshold()).WillOnce(Return(0));
    EXPECT_CALL(*view, getMinThreshold()).WillOnce(Return(0));
    EXPECT_CALL(*view, getApplyClip()).WillRepeatedly(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en", "11", "11", "11");
    EXPECT_CALL(*view, getAppliedGeometryXML()).Times(3).WillRepeatedly(Return(viewXML.c_str()));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateVisualDataSetOnly)).Times(1); //From standard 4D swapping
    EXPECT_CALL(*pRequest, ask(RecalculateAll)).Times(3); //Nxbins & Nybins & Nzbins changed, requires rebin request.

    MockClipper* pClipper = new MockClipper;

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDHistogramRebinningPresenter presenter(dataSet, pRequest, view, pClipper, wsProvider);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

      
  void testExecutionWithFullRebin()
  {
    /*****
     Test that it executes and fully rebins on first pass, then that it does not rebin again on the second pass.
    ****/

    MockMDRebinningView* view = new  MockMDRebinningView;
    EXPECT_CALL(*view, getTimeStep()).WillOnce(Return(0)); //NoChange
    EXPECT_CALL(*view, getMaxThreshold()).WillOnce(Return(0)); //NoChange
    EXPECT_CALL(*view, getMinThreshold()).WillOnce(Return(0)); //NoChange
    EXPECT_CALL(*view, getApplyClip()).WillOnce(Return(false)); //NoChange
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en");
    EXPECT_CALL(*view, getAppliedGeometryXML()).Times(2).WillRepeatedly(Return(viewXML.c_str())); // NoChange

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(_)).Times(0);
    EXPECT_CALL(*pRequest, action()).Times(2).WillRepeatedly(Return(RecalculateAll)); //Request is preset to RecalculateAll.
    EXPECT_CALL(*pRequest, reset()).Times(1);

    MockClipper* pClipper = new MockClipper;
    MockvtkDataSetFactory* pDataSetFactory = new MockvtkDataSetFactory;
    EXPECT_CALL(*pDataSetFactory, initialize(_)).Times(1);
    EXPECT_CALL(*pDataSetFactory, create()).WillOnce(Return(vtkUnstructuredGrid::New()));

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    FakeProgressAction progressAction;

    MDHistogramRebinningPresenter presenter(dataSet, pRequest, view, pClipper, wsProvider);
    presenter.updateModel();
    vtkDataSet* product = presenter.execute(pDataSetFactory, progressAction);

    TS_ASSERT(Mock::VerifyAndClearExpectations(view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pDataSetFactory));
    delete pDataSetFactory;
    product->Delete();
  }

};

#endif
