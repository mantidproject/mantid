#ifndef MDEW_REBINNING_PRESENTER_TEST_H_
#define MDEW_REBINNING_PRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vtkUnstructuredGrid.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "MockObjects.h"

#include "MantidVatesAPI/MDEWRebinningPresenter.h"

using namespace testing;
using namespace Mantid::VATES;

//=====================================================================================
// Functional tests
//=====================================================================================
class MDEWRebinningPresenterTest : public CxxTest::TestSuite
{


public:

void testConstructorThrows()
{
  MockMDRebinningView view;
  MockRebinningActionManager actionManager;
  TSM_ASSERT_THROWS("Should throw if no FieldData", MDEWRebinningPresenter<MockMDRebinningView>(vtkUnstructuredGrid::New(), new MockRebinningActionManager, &view), std::logic_error);
}

  void testConstruction()
  {
    MockMDRebinningView view;
    MockRebinningActionManager* pRequest = new MockRebinningActionManager;

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDEWRebinningPresenter<MockMDRebinningView> presenter(dataSet, pRequest, &view);
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
    
    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDEWRebinningPresenter<MockMDRebinningView> presenter(dataSet, pRequest, &view);
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
    
    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDEWRebinningPresenter<MockMDRebinningView> presenter(dataSet, pRequest, &view);
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
    
    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDEWRebinningPresenter<MockMDRebinningView> presenter(dataSet, pRequest, &view);
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

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDEWRebinningPresenter<MockMDRebinningView> presenter(dataSet, pRequest, &view);
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
    EXPECT_CALL(view, getAppliedGeometryXML()).Times(2).WillRepeatedly(Return(viewXML.c_str())); //Geometry (4D) should reflect on request.
   
    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateAll)).Times(1); //Nxbins changed, requires rebin request.

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDEWRebinningPresenter<MockMDRebinningView> presenter(dataSet, pRequest, &view);
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
    EXPECT_CALL(view, getAppliedGeometryXML()).Times(2).WillRepeatedly(Return(viewXML.c_str()));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateAll)).Times(1); //Nxbins & Nybins changed, requires rebin request.

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDEWRebinningPresenter<MockMDRebinningView> presenter(dataSet, pRequest, &view);
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
    EXPECT_CALL(view, getAppliedGeometryXML()).Times(2).WillRepeatedly(Return(viewXML.c_str()));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateAll)).Times(1); //Nxbins & Nybins & Nzbins changed, requires rebin request.

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(createFieldDataWithCharArray(constructXML("qx", "qy", "qz", "en")));

    MDEWRebinningPresenter<MockMDRebinningView> presenter(dataSet, pRequest, &view);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }


};

#endif