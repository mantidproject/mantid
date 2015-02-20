#ifndef MDEW_REBINNING_PRESENTER_TEST_H_
#define MDEW_REBINNING_PRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vtkUnstructuredGrid.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "MockObjects.h"

#include "MantidVatesAPI/MDEWRebinningPresenter.h"
#include "MantidVatesAPI/MetadataToFieldData.h"
#include "MantidVatesAPI/RebinningKnowledgeSerializer.h"
#include "MantidVatesAPI/VatesConfigurations.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidVatesAPI/vtkMDHistoHexFactory.h"
#include "MantidVatesAPI/vtkMDHistoHex4DFactory.h"
#include "MantidVatesAPI/NoThresholdRange.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include "MantidVatesAPI/EscalatingRebinningActionManager.h"
#include "MantidVatesAPI/vtkDataSetToGeometry.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

using namespace testing;
using namespace Mantid::VATES;
using namespace Mantid::MDEvents;

//=====================================================================================
// Functional tests
//=====================================================================================
class MDEWRebinningPresenterTest : public CxxTest::TestSuite
{
private:

  vtkFieldData* generateFieldData(std::string testData)
  {
    // Create mock object
    vtkFieldData* fieldData = createFieldDataWithCharArray(testData);

    // Generate metadata
    MetadataJsonManager manager;
    manager.setInstrument("OSIRIS");

    VatesConfigurations config;

    std::string jsonString = manager.getSerializedJson();

    // Add additional json metadata to the field data
    MetadataToFieldData convert;
    convert(fieldData, jsonString, config.getMetadataIdJson().c_str());
    
    return fieldData;
  }

public:

  void testConstructorThrowsWithoutFieldData()
  {
    MockRebinningActionManager actionManager;

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).Times(0);

    TSM_ASSERT_THROWS("Should throw if no FieldData", MDEWRebinningPresenter(vtkUnstructuredGrid::New(), new MockRebinningActionManager, new MockMDRebinningView, wsProvider), std::logic_error);
  }

  void testConstructorThrowsWhenCannotProvideWorkspace()
  {
    MockRebinningActionManager actionManager;

    vtkUnstructuredGrid* dataset = vtkUnstructuredGrid::New();
    dataset->SetFieldData(generateFieldData(constructXML("qx", "qy", "qz", "en")));

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillOnce(Return(false));

    TSM_ASSERT_THROWS("Should throw if cannot provide workspace", MDEWRebinningPresenter(dataset, new MockRebinningActionManager, new MockMDRebinningView, wsProvider), std::invalid_argument);
  }

  void testConstruction()
  {
    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(generateFieldData(constructXML("qx", "qy", "qz", "en")));

    MDEWRebinningPresenter presenter(dataSet, pRequest, new MockMDRebinningView, wsProvider);
    TSM_ASSERT("Geometry should be available immediately after construction.", !presenter.getAppliedGeometryXML().empty());
  }

  void testUpdateModelWithNoChanges()
  {
    MockMDRebinningView* view = new MockMDRebinningView;
    EXPECT_CALL(*view, getOutputHistogramWS()).WillRepeatedly(Return(true));
    EXPECT_CALL(*view, getTimeStep()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMaxThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMinThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getApplyClip()).WillRepeatedly(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en");
    EXPECT_CALL(*view, getAppliedGeometryXML()).WillRepeatedly(Return(viewXML.c_str()));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateAll)).Times(0); //Since nothing has changed, no requests should be made.

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(generateFieldData(constructXML("qx", "qy", "qz", "en")));

    MDEWRebinningPresenter presenter(dataSet, pRequest, view, wsProvider);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithDifferentMaxThreshold()
  {
    MockMDRebinningView* view = new MockMDRebinningView;
    EXPECT_CALL(*view, getOutputHistogramWS()).WillRepeatedly(Return(true));
    EXPECT_CALL(*view, getTimeStep()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMaxThreshold()).WillRepeatedly(Return(1)); //Maxthreshold non-zero
    EXPECT_CALL(*view, getMinThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getApplyClip()).WillRepeatedly(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en");
    EXPECT_CALL(*view, getAppliedGeometryXML()).WillRepeatedly(Return(viewXML.c_str()));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateVisualDataSetOnly)).Times(1); //Maxthreshold updated should reflect on request.

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(generateFieldData(constructXML("qx", "qy", "qz", "en")));

    MDEWRebinningPresenter presenter(dataSet, pRequest, view, wsProvider);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithDifferentMinThreshold()
  {
    MockMDRebinningView* view = new MockMDRebinningView;
    EXPECT_CALL(*view, getOutputHistogramWS()).WillRepeatedly(Return(true));
    EXPECT_CALL(*view, getTimeStep()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMaxThreshold()).WillRepeatedly(Return(0)); 
    EXPECT_CALL(*view, getMinThreshold()).WillRepeatedly(Return(1)); //Minthreshold non-zero
    EXPECT_CALL(*view, getApplyClip()).WillRepeatedly(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en");
    EXPECT_CALL(*view, getAppliedGeometryXML()).WillRepeatedly(Return(viewXML.c_str()));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateVisualDataSetOnly)).Times(1); //Minthreshold updated should reflect on request.

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(generateFieldData(constructXML("qx", "qy", "qz", "en")));

    MDEWRebinningPresenter presenter(dataSet, pRequest, view, wsProvider);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithDifferentTimestep()
  {
    MockMDRebinningView* view = new MockMDRebinningView;
    EXPECT_CALL(*view, getOutputHistogramWS()).WillRepeatedly(Return(true));
    EXPECT_CALL(*view, getTimeStep()).Times(2).WillRepeatedly(Return(1)); //Timestep updated
    EXPECT_CALL(*view, getMaxThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMinThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getApplyClip()).WillRepeatedly(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en");
    EXPECT_CALL(*view, getAppliedGeometryXML()).WillRepeatedly(Return(viewXML.c_str()));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateVisualDataSetOnly)).Times(1); //Timestep updated should reflect on request.

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(generateFieldData(constructXML("qx", "qy", "qz", "en")));

    MDEWRebinningPresenter presenter(dataSet, pRequest, view, wsProvider);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithMoreXBins()
  {
    MockMDRebinningView* view = new MockMDRebinningView;
    EXPECT_CALL(*view, getOutputHistogramWS()).WillRepeatedly(Return(true));
    EXPECT_CALL(*view, getTimeStep()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMaxThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMinThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getApplyClip()).WillRepeatedly(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en", "11");
    EXPECT_CALL(*view, getAppliedGeometryXML()).Times(2).WillRepeatedly(Return(viewXML.c_str())); //Geometry (4D) should reflect on request.

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateAll)).Times(1); //Nxbins changed, requires rebin request.

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(generateFieldData(constructXML("qx", "qy", "qz", "en")));

    MDEWRebinningPresenter presenter(dataSet, pRequest, view, wsProvider);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithMoreXYBins()
  {
    MockMDRebinningView* view = new MockMDRebinningView;
    EXPECT_CALL(*view, getOutputHistogramWS()).WillRepeatedly(Return(true));
    EXPECT_CALL(*view, getTimeStep()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMaxThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMinThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getApplyClip()).WillRepeatedly(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en", "11", "11");
    EXPECT_CALL(*view, getAppliedGeometryXML()).Times(2).WillRepeatedly(Return(viewXML.c_str()));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateAll)).Times(1); //Nxbins & Nybins changed, requires rebin request.

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(generateFieldData(constructXML("qx", "qy", "qz", "en")));

    MDEWRebinningPresenter presenter(dataSet, pRequest, view, wsProvider);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }


  void testUpdateModelWithMoreXYZBins()
  {
    MockMDRebinningView* view = new MockMDRebinningView;
    EXPECT_CALL(*view, getOutputHistogramWS()).WillRepeatedly(Return(true));
    EXPECT_CALL(*view, getTimeStep()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMaxThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMinThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getApplyClip()).WillRepeatedly(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en", "11", "11", "11");
    EXPECT_CALL(*view, getAppliedGeometryXML()).Times(2).WillRepeatedly(Return(viewXML.c_str()));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateAll)).Times(1); //Nxbins & Nybins & Nzbins changed, requires rebin request.

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(generateFieldData(constructXML("qx", "qy", "qz", "en")));

    MDEWRebinningPresenter presenter(dataSet, pRequest, view, wsProvider);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithDifferentOutputType()
  {
    MockMDRebinningView* view = new MockMDRebinningView;
    EXPECT_CALL(*view, getOutputHistogramWS()).WillRepeatedly(Return(false)); //Output a full MDEW workspace via SliceMD
    EXPECT_CALL(*view, getTimeStep()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMaxThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMinThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getApplyClip()).WillRepeatedly(Return(false));
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en");
    EXPECT_CALL(*view, getAppliedGeometryXML()).WillRepeatedly(Return(viewXML.c_str()));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateAll)).Times(1); //Output type changed. Requires re-execution.

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(generateFieldData(constructXML("qx", "qy", "qz", "en")));

    MDEWRebinningPresenter presenter(dataSet, pRequest, view, wsProvider);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithApplyClipping()
  {

    MockMDRebinningView* view = new MockMDRebinningView;
    EXPECT_CALL(*view, getOutputHistogramWS()).WillRepeatedly(Return(true));
    EXPECT_CALL(*view, getTimeStep()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMaxThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMinThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getApplyClip()).WillRepeatedly(Return(true)); // Clipping applied.
    EXPECT_CALL(*view, getOrigin()).WillRepeatedly(Return(Mantid::Kernel::V3D(0, 0, 0)));
    EXPECT_CALL(*view, getB1()).Times(AtLeast(1)).WillRepeatedly(Return(Mantid::Kernel::V3D(1, 0, 0)));
    EXPECT_CALL(*view, getB2()).Times(AtLeast(1)).WillRepeatedly(Return(Mantid::Kernel::V3D(0, 1, 0)));
    EXPECT_CALL(*view, getLengthB1()).Times(AtLeast(1)).WillRepeatedly(Return(1));
    EXPECT_CALL(*view, getLengthB2()).Times(AtLeast(1)).WillRepeatedly(Return(1));
    EXPECT_CALL(*view, getLengthB3()).Times(AtLeast(1)).WillRepeatedly(Return(1));
    EXPECT_CALL(*view, getForceOrthogonal()).WillRepeatedly(Return(false));

    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en");
    EXPECT_CALL(*view, getAppliedGeometryXML()).Times(AtLeast(1)).WillRepeatedly(Return(viewXML.c_str()));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    EXPECT_CALL(*pRequest, ask(RecalculateAll)).Times(AtLeast(1)); //Clipping changed so should rebin.

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(generateFieldData(constructXML("qx", "qy", "qz", "en")));

    MDEWRebinningPresenter presenter(dataSet, pRequest, view, wsProvider);
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithSameClipping()
  {
    MockMDRebinningView* view = new MockMDRebinningView;
    EXPECT_CALL(*view, getOutputHistogramWS()).WillRepeatedly(Return(true));
    EXPECT_CALL(*view, getTimeStep()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMaxThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMinThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getApplyClip()).WillRepeatedly(Return(true)); // Clipping applied.
    EXPECT_CALL(*view, getOrigin()).WillRepeatedly(Return(Mantid::Kernel::V3D(0, 0, 0)));
    EXPECT_CALL(*view, getB1()).Times(AtLeast(1)).WillRepeatedly(Return(Mantid::Kernel::V3D(1, 0, 0)));
    EXPECT_CALL(*view, getB2()).Times(AtLeast(1)).WillRepeatedly(Return(Mantid::Kernel::V3D(0, 1, 0)));
    EXPECT_CALL(*view, getLengthB1()).Times(AtLeast(1)).WillRepeatedly(Return(1));
    EXPECT_CALL(*view, getLengthB2()).Times(AtLeast(1)).WillRepeatedly(Return(1));
    EXPECT_CALL(*view, getLengthB3())
      .Times(AtLeast(1))
      .WillOnce(Return(1))
      .WillOnce(Return(1)); 
    EXPECT_CALL(*view, getForceOrthogonal()).WillRepeatedly(Return(false));
    
    //Should now need to fetch the implicit function
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en");
    EXPECT_CALL(*view, getAppliedGeometryXML()).Times(AtLeast(1)).WillRepeatedly(Return(viewXML.c_str()));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    
    EXPECT_CALL(*pRequest, ask(RecalculateAll)).Times(AtLeast(1)); //Should ask on first pass, but not for secondClipping as is identical to first.

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(generateFieldData(constructXML("qx", "qy", "qz", "en")));

    MDEWRebinningPresenter presenter(dataSet, pRequest, view, wsProvider);
    presenter.updateModel();
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testUpdateModelWithDifferentClipping()
  {
    MockMDRebinningView* view = new MockMDRebinningView;
    EXPECT_CALL(*view, getOutputHistogramWS()).WillRepeatedly(Return(true));
    EXPECT_CALL(*view, getTimeStep()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMaxThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMinThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getApplyClip()).WillRepeatedly(Return(true)); // Clipping applied.
    EXPECT_CALL(*view, getOrigin()).WillRepeatedly(Return(Mantid::Kernel::V3D(0, 0, 0)));
    EXPECT_CALL(*view, getB1()).Times(AtLeast(1)).WillRepeatedly(Return(Mantid::Kernel::V3D(1, 0, 0)));
    EXPECT_CALL(*view, getB2()).Times(AtLeast(1)).WillRepeatedly(Return(Mantid::Kernel::V3D(0, 1, 0)));
    EXPECT_CALL(*view, getLengthB1()).Times(AtLeast(1)).WillRepeatedly(Return(1));
    EXPECT_CALL(*view, getLengthB2()).Times(AtLeast(1)).WillRepeatedly(Return(1));
    EXPECT_CALL(*view, getLengthB3())
      .Times(AtLeast(1))
      .WillOnce(Return(1))
      .WillOnce(Return(2)); 
    EXPECT_CALL(*view, getForceOrthogonal()).WillRepeatedly(Return(false));
    
    //Should now need to fetch the implicit function
    std::string viewXML = constrctGeometryOnlyXML("qx", "qy", "qz", "en");
    EXPECT_CALL(*view, getAppliedGeometryXML()).Times(AtLeast(1)).WillRepeatedly(Return(viewXML.c_str()));

    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    
    EXPECT_CALL(*pRequest, ask(RecalculateAll)).Times(AtLeast(1)); //Should ask on first pass, but not for secondClipping as is identical to first.

    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(generateFieldData(constructXML("qx", "qy", "qz", "en")));

    MDEWRebinningPresenter presenter(dataSet, pRequest, view, wsProvider);
    presenter.updateModel();
    presenter.updateModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRequest));
  }

  void testExecute()
  {
    // Create a MDWorkspace and put it into the ADS.
    const std::string wsName = "TestMDEW";
    auto someMDEW = Mantid::MDEvents::MDEventsTestHelper::makeAnyMDEW<MDLeanEvent<3>,3>(10,0,10,0,wsName);

    // Generate xml relating to the dimensionality etc. by querying this workspace.
    RebinningKnowledgeSerializer serializer(LocationNotRequired);
    serializer.setWorkspace(someMDEW);
    std::string creationalXML = serializer.createXMLString();

    // Create an empty dataset and attach that xml as field data.
    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(generateFieldData(creationalXML));

    /*
    The vtkFilter is the view in our MVP set up.
    We can't actually create an instance of the vtkFilter for testing, which would otherwise make it impossible to test any of this functionality.

    However, we can mock a View, because both the real filter and our Mock inherite from MDRebinningView. Using this view we 'simulate' real user inputs.
    */
    MockMDRebinningView* view = new MockMDRebinningView;
    EXPECT_CALL(*view, getOutputHistogramWS()).WillRepeatedly(Return(true));
    EXPECT_CALL(*view, getTimeStep()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMaxThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMinThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getApplyClip()).WillRepeatedly(Return(false));
    /* 
      The following is the critical method. It returns the xml from the current state of the view.
      At the moment, the view is going to return the same xml as it was originally given. However, we can override this behaviour and provide 
      any xml we wish. So for example, simulating that the user has increased the number of bins.
    */
    std::string viewXML = serializer.getWorkspaceGeometry();
    EXPECT_CALL(*view, getAppliedGeometryXML()).WillRepeatedly(Return(viewXML.c_str())); 

    // The workspace provider is a proxy to the Analysis Data Service.
    ADSWorkspaceProvider<Mantid::API::IMDEventWorkspace> workspaceProvider;

    // The request is the way that the rebinner stores what action to take when the user hits 'Apply'/
    RebinningActionManager* pRequest = new EscalatingRebinningActionManager;

    // Create a presenter which binds the Model and the View together.
    MDEWRebinningPresenter presenter(dataSet, pRequest, view, workspaceProvider);
    pRequest->ask(Mantid::VATES::RecalculateAll); // Force it to rebin. Usually we call updateModel first, which figures out what action needs to be taken (rebin, just redraw e.t.c).

    // A progress action is the mechanism by which progress is reported.
    NiceMock<MockProgressAction> progressAction;
    // Create a factory for visualising the output workspace after rebinning. We only provide one here, because we know that it's going to be a 3D MDHistoWorkspace from BinMD.
    auto vtkFactory = new vtkMDHistoHexFactory(ThresholdRange_scptr(new NoThresholdRange), "signal");
    vtkDataSet* product = presenter.execute(vtkFactory,  progressAction, progressAction);

    // Now we can read the xml off the product data set. For example..
    vtkDataSetToGeometry parser(product);
    parser.execute();
    TS_ASSERT_EQUALS(3, parser.getAllDimensions().size()); // Simple test here. but there are loads of other properties on the parser to test.
  
    // NOTE. if you wanted to Extract the field data. You could do that here by fetching it off output product vtkDataSet.
  }

  void testTimeLabelAfterRebinFor4DData()
  {
    const std::string wsName = "TestMDEW";
    auto someMDEW = Mantid::MDEvents::MDEventsTestHelper::makeAnyMDEW<MDLeanEvent<4>,4>(10,0,10,0,wsName);

    RebinningKnowledgeSerializer serializer(LocationNotRequired);
    serializer.setWorkspace(someMDEW);
    std::string creationalXML = serializer.createXMLString();

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(generateFieldData(creationalXML));

    MockMDRebinningView* view = new MockMDRebinningView;
    EXPECT_CALL(*view, getOutputHistogramWS()).WillRepeatedly(Return(true));
    EXPECT_CALL(*view, getTimeStep()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMaxThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMinThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getApplyClip()).WillRepeatedly(Return(false));

    std::string viewXML = constructGeometryOnlyXMLForMDEvHelperData("Axis3",
                                                                    "Axis2",
                                                                    "Axis1",
                                                                    "Axis0");
    EXPECT_CALL(*view, getAppliedGeometryXML()).WillRepeatedly(Return(viewXML.c_str()));

    ADSWorkspaceProvider<Mantid::API::IMDEventWorkspace> workspaceProvider;

    RebinningActionManager* pRequest = new EscalatingRebinningActionManager;

    MDEWRebinningPresenter presenter(dataSet, pRequest, view, workspaceProvider);
    pRequest->ask(Mantid::VATES::RecalculateAll);

    TSM_ASSERT_EQUALS("Time label should be exact.",
                      presenter.getTimeStepLabel(), "Axis0 (m)")
  }

  void testAxisLabelsAfterRebinFor3DData()
  {
    const std::string wsName = "TestMDEW";
    auto someMDEW = Mantid::MDEvents::MDEventsTestHelper::makeAnyMDEW<MDLeanEvent<3>,3>(10,0,10,0,wsName);

    RebinningKnowledgeSerializer serializer(LocationNotRequired);
    serializer.setWorkspace(someMDEW);
    std::string creationalXML = serializer.createXMLString();

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(generateFieldData(creationalXML));

    MockMDRebinningView* view = new MockMDRebinningView;
    EXPECT_CALL(*view, getOutputHistogramWS()).WillRepeatedly(Return(true));
    EXPECT_CALL(*view, getTimeStep()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMaxThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMinThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getApplyClip()).WillRepeatedly(Return(false));

    std::string viewXML = constructGeometryOnlyXMLForMDEvHelperData("Axis2",
                                                                    "Axis0",
                                                                    "Axis1",
                                                                    "");
    EXPECT_CALL(*view, getAppliedGeometryXML()).WillRepeatedly(Return(viewXML.c_str()));

    ADSWorkspaceProvider<Mantid::API::IMDEventWorkspace> workspaceProvider;

    RebinningActionManager* pRequest = new EscalatingRebinningActionManager;

    MDEWRebinningPresenter presenter(dataSet, pRequest, view, workspaceProvider);
    pRequest->ask(Mantid::VATES::RecalculateAll);

    NiceMock<MockProgressAction> progressAction;

    auto vtkFactory = new vtkMDHistoHexFactory(ThresholdRange_scptr(new NoThresholdRange), "signal");
    vtkDataSet* product = presenter.execute(vtkFactory,  progressAction, progressAction);

    TSM_ASSERT_THROWS_NOTHING("Should pass", presenter.setAxisLabels(product));
    TSM_ASSERT_EQUALS("X Label should match exactly",
                      getStringFieldDataValue(product, "AxisTitleForX"),
                      "Axis2 (m)");
    TSM_ASSERT_EQUALS("Y Label should match exactly",
                      getStringFieldDataValue(product, "AxisTitleForY"),
                      "Axis0 (m)");
    TSM_ASSERT_EQUALS("Z Label should match exactly",
                      getStringFieldDataValue(product, "AxisTitleForZ"),
                      "Axis1 (m)");
  }

  void testAxisLabelsAfterRebinFor4DData()
  {
    const std::string wsName = "TestMDEW";
    auto someMDEW = Mantid::MDEvents::MDEventsTestHelper::makeAnyMDEW<MDLeanEvent<4>,4>(10,0,10);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(wsName, someMDEW);

    RebinningKnowledgeSerializer serializer(LocationNotRequired);
    serializer.setWorkspace(someMDEW);
    std::string creationalXML = serializer.createXMLString();

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(generateFieldData(creationalXML));

    MockMDRebinningView* view = new MockMDRebinningView;
    EXPECT_CALL(*view, getOutputHistogramWS()).WillRepeatedly(Return(true));
    EXPECT_CALL(*view, getTimeStep()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMaxThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getMinThreshold()).WillRepeatedly(Return(0));
    EXPECT_CALL(*view, getApplyClip()).WillRepeatedly(Return(false));

    std::string viewXML = constructGeometryOnlyXMLForMDEvHelperData("Axis3",
                                                                    "Axis2",
                                                                    "Axis1",
                                                                    "Axis0");
    EXPECT_CALL(*view, getAppliedGeometryXML()).WillRepeatedly(Return(viewXML.c_str()));

    ADSWorkspaceProvider<Mantid::API::IMDEventWorkspace> workspaceProvider;

    RebinningActionManager* pRequest = new EscalatingRebinningActionManager;

    MDEWRebinningPresenter presenter(dataSet, pRequest, view, workspaceProvider);
    pRequest->ask(Mantid::VATES::RecalculateAll);

    NiceMock<MockProgressAction> progressAction;

    auto vtkFactory = new vtkMDHistoHex4DFactory<TimeToTimeStep>(ThresholdRange_scptr(new NoThresholdRange), "signal", 0.0);
    vtkDataSet* product = presenter.execute(vtkFactory,  progressAction, progressAction);

    TSM_ASSERT_THROWS_NOTHING("Should pass", presenter.setAxisLabels(product));
    TSM_ASSERT_EQUALS("X Label should match exactly",
                      getStringFieldDataValue(product, "AxisTitleForX"),
                      "Axis3 (s)");
    TSM_ASSERT_EQUALS("Y Label should match exactly",
                      getStringFieldDataValue(product, "AxisTitleForY"),
                      "Axis2 (m)");
    TSM_ASSERT_EQUALS("Z Label should match exactly",
                      getStringFieldDataValue(product, "AxisTitleForZ"),
                      "Axis1 (m)");
  }

  void testJsonMetadataExtractionFromRebinnedDataSet()
  {
    MockRebinningActionManager* pRequest = new MockRebinningActionManager;
    MockWorkspaceProvider wsProvider;
    EXPECT_CALL(wsProvider, canProvideWorkspace(_)).WillRepeatedly(Return(true));

    vtkUnstructuredGrid* dataSet = vtkUnstructuredGrid::New();
    dataSet->SetFieldData(generateFieldData(constructXML("qx", "qy", "qz", "en")));

    MDEWRebinningPresenter presenter(dataSet, pRequest, new MockMDRebinningView, wsProvider);
    TSM_ASSERT("Instrument should be available immediately after construction.", !presenter.getInstrument().empty());
    TSM_ASSERT("Instrument should be read out correctly.", presenter.getInstrument() == "OSIRIS");
  }
};

#endif
