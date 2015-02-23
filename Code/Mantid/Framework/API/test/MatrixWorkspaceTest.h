#ifndef WORKSPACETEST_H_
#define WORKSPACETEST_H_

#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/VMD.h"
#include "MantidTestHelpers/FakeGmockObjects.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "MantidTestHelpers/NexusTestHelper.h"

#include <cxxtest/TestSuite.h>
#include <boost/make_shared.hpp>

using std::size_t;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace testing;


// Declare into the factory.
DECLARE_WORKSPACE(WorkspaceTester);

/** Create a workspace with numSpectra, with
 * each spectrum having one detector, at id = workspace index.
 * @param numSpectra
 * @return
 */
boost::shared_ptr<MatrixWorkspace> makeWorkspaceWithDetectors(size_t numSpectra, size_t numBins)
{
  boost::shared_ptr<MatrixWorkspace> ws2 = boost::make_shared<WorkspaceTester>();
  ws2->initialize(numSpectra,numBins,numBins);

  Instrument_sptr inst(new Instrument("TestInstrument"));
  ws2->setInstrument(inst);
  // We get a 1:1 map by default so the detector ID should match the spectrum number
  for( size_t i = 0; i < ws2->getNumberHistograms(); ++i )
  {
    // Create a detector for each spectra
    Detector * det = new Detector("pixel", static_cast<detid_t>(i), inst.get());
    inst->add(det);
    inst->markAsDetector(det);
    ws2->getSpectrum(i)->addDetectorID(static_cast<detid_t>(i));
  }
  return ws2;
}



class MatrixWorkspaceTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MatrixWorkspaceTest *createSuite() { return new MatrixWorkspaceTest(); }
  static void destroySuite( MatrixWorkspaceTest *suite ) { delete suite; }

  MatrixWorkspaceTest() : ws(new WorkspaceTester)
  {
    ws->initialize(1,1,1);
  }
  
  void test_toString_Produces_Expected_Contents()
  {
    auto testWS = boost::make_shared<WorkspaceTester>();
    testWS->initialize(1,2,1);
    testWS->setTitle("A test run");
    testWS->getAxis(0)->setUnit("TOF");
    testWS->setYUnitLabel("Counts");

    std::string expected = \
      "WorkspaceTester\n"
      "Title: A test run\n"
      "Histograms: 1\n"
      "Bins: 1\n"
      "Histogram\n"
      "X axis: Time-of-flight / microsecond\n"
      "Y axis: Counts\n"
      "Distribution: False\n"
      "Instrument:  (1990-Jan-01 to 1990-Jan-01)\n"
      "Run start: not available\n"
      "Run end:  not available\n"
      ;

    TS_ASSERT_EQUALS(expected, testWS->toString());
  }

  void testGetSetTitle()
  {
    TS_ASSERT_EQUALS( ws->getTitle(), "" );
    ws->setTitle("something");
    TS_ASSERT_EQUALS( ws->getTitle(), "something" );
    ws->setTitle("");
  }

  void testGetSetComment()
  {
    TS_ASSERT_EQUALS( ws->getComment(), "" );
    ws->setComment("commenting");
    TS_ASSERT_EQUALS( ws->getComment(), "commenting" );
    ws->setComment("");
  }

  void test_getIndicesFromDetectorIDs()
  {
    WorkspaceTester ws;
    ws.initialize(10, 1,1);
    for (size_t i=0; i<10; i++)
      ws.getSpectrum(i)->setDetectorID(detid_t(i*10));
    std::vector<detid_t> dets;
    dets.push_back(60);
    dets.push_back(20);
    dets.push_back(90);
    std::vector<size_t> indices;
    ws.getIndicesFromDetectorIDs(dets, indices);
    TS_ASSERT_EQUALS( indices.size(), 3);
    TS_ASSERT_EQUALS( indices[0], 6);
    TS_ASSERT_EQUALS( indices[1], 2);
    TS_ASSERT_EQUALS( indices[2], 9);
  }

  void test_That_A_Workspace_Gets_SpectraMap_When_Initialized_With_NVector_Elements()
  {
    WorkspaceTester testWS;
    const size_t nhist(10);
    testWS.initialize(nhist,1,1);
    for (size_t i=0; i<testWS.getNumberHistograms(); i++)
    {
      TS_ASSERT_EQUALS(testWS.getSpectrum(i)->getSpectrumNo(), specid_t(i+1));
      TS_ASSERT(testWS.getSpectrum(i)->hasDetectorID(detid_t(i)));
    }
  }

  void test_updateSpectraUsing()
  {
    WorkspaceTester testWS;
    testWS.initialize(3,1,1);

    specid_t specs[] = {1,2,2,3};
    detid_t detids[] = {10,99,20,30};
    TS_ASSERT_THROWS_NOTHING( testWS.updateSpectraUsing(SpectrumDetectorMapping(specs, detids, 4)) );

    TS_ASSERT( testWS.getSpectrum(0)->hasDetectorID(10) );
    TS_ASSERT( testWS.getSpectrum(1)->hasDetectorID(20) );
    TS_ASSERT( testWS.getSpectrum(1)->hasDetectorID(99) );
    TS_ASSERT( testWS.getSpectrum(2)->hasDetectorID(30) );
  }

  void testDetectorMappingCopiedWhenAWorkspaceIsCopied()
  {
    boost::shared_ptr<MatrixWorkspace> parent(new WorkspaceTester);
    parent->initialize(1,1,1);
    parent->getSpectrum(0)->setSpectrumNo(99);
    parent->getSpectrum(0)->setDetectorID(999);

    MatrixWorkspace_sptr copied = WorkspaceFactory::Instance().create(parent);
    // Has it been copied?
    TS_ASSERT_EQUALS(copied->getSpectrum(0)->getSpectrumNo(), 99);
    TS_ASSERT(copied->getSpectrum(0)->hasDetectorID(999));
  }

  void testGetMemorySize()
  {
    TS_ASSERT_THROWS_NOTHING( ws->getMemorySize() );
  }

  void testHistory()
  {
    TS_ASSERT_THROWS_NOTHING( ws->history() );
  }

  void testAxes()
  {
    TS_ASSERT_EQUALS( ws->axes(), 2 );
  }

  void testGetAxis()
  {
    TS_ASSERT_THROWS( ws->getAxis(-1), Exception::IndexError );
    TS_ASSERT_THROWS_NOTHING( ws->getAxis(0) );
    TS_ASSERT( ws->getAxis(0) );
    TS_ASSERT( ws->getAxis(0)->isNumeric() );
    TS_ASSERT_THROWS( ws->getAxis(2), Exception::IndexError );
  }

  void testReplaceAxis()
  {
    Axis* ax = new SpectraAxis(ws.get());
    TS_ASSERT_THROWS( ws->replaceAxis(2,ax), Exception::IndexError );
    TS_ASSERT_THROWS_NOTHING( ws->replaceAxis(0,ax) );
    TS_ASSERT( ws->getAxis(0)->isSpectra() );
  }

  void testIsDistribution()
  {
    TS_ASSERT( ! ws->isDistribution() );
    TS_ASSERT( ws->isDistribution(true) );
    TS_ASSERT( ws->isDistribution() );
  }

  void testGetSetYUnit()
  {
    TS_ASSERT_EQUALS( ws->YUnit(), "" );
    TS_ASSERT_THROWS_NOTHING( ws->setYUnit("something") );
    TS_ASSERT_EQUALS( ws->YUnit(), "something" );
  }


  void testGetSpectrum()
  {
    WorkspaceTester ws;
    ws.initialize(4,1,1);
    ISpectrum * spec = NULL;
    TS_ASSERT_THROWS_NOTHING( spec = ws.getSpectrum(0) );
    TS_ASSERT(spec);
    TS_ASSERT_THROWS_NOTHING( spec = ws.getSpectrum(3) );
    TS_ASSERT(spec);
  }

  /** Get a detector sptr for each spectrum */
  void testGetDetector()
  {
    // Workspace has 3 spectra, each 1 in length
    const int numHist(3);
    boost::shared_ptr<MatrixWorkspace> workspace(makeWorkspaceWithDetectors(3,1));

    // Initially un masked
    for( int i = 0; i < numHist; ++i )
    {
      IDetector_const_sptr det;
      TS_ASSERT_THROWS_NOTHING(det = workspace->getDetector(i));
      if( det )
      {
        TS_ASSERT_EQUALS(det->getID(), i);
      }
      else
      {
        TS_FAIL("No detector defined");
      }
    }

    // Now a detector group
    ISpectrum * spec = workspace->getSpectrum(0);
    spec->addDetectorID(1);
    spec->addDetectorID(2);
    IDetector_const_sptr det;
    TS_ASSERT_THROWS_NOTHING(det = workspace->getDetector(0));
    TS_ASSERT(det);

    // Now an empty (no detector) pixel
    spec = workspace->getSpectrum(1);
    spec->clearDetectorIDs();
    IDetector_const_sptr det2;
    TS_ASSERT_THROWS_ANYTHING(det2 = workspace->getDetector(1));
    TS_ASSERT(!det2);
  }



  void testWholeSpectraMasking()
  {
    // Workspace has 3 spectra, each 1 in length
    const int numHist(3);
    boost::shared_ptr<MatrixWorkspace> workspace(makeWorkspaceWithDetectors(3,1));

    // Initially un masked
    for( int i = 0; i < numHist; ++i )
    {
      TS_ASSERT_EQUALS(workspace->readY(i)[0], 1.0);
      TS_ASSERT_EQUALS(workspace->readE(i)[0], 1.0);

      IDetector_const_sptr det;
      TS_ASSERT_THROWS_NOTHING(det = workspace->getDetector(i));
      if( det )
      {
        TS_ASSERT_EQUALS(det->isMasked(), false);
      }
      else
      {
        TS_FAIL("No detector defined");
      }
    }

    // Mask a spectra
    workspace->maskWorkspaceIndex(1);
    workspace->maskWorkspaceIndex(2);

    for( int i = 0; i < numHist; ++i )
    {
      double expectedValue(0.0);
      bool expectedMasked(false);
      if( i == 0 )
      {
        expectedValue = 1.0;
        expectedMasked = false;
      }
      else
      {
        expectedMasked = true;
      }
      TS_ASSERT_EQUALS(workspace->readY(i)[0], expectedValue);
      TS_ASSERT_EQUALS(workspace->readE(i)[0], expectedValue);

      IDetector_const_sptr det;
      TS_ASSERT_THROWS_NOTHING(det = workspace->getDetector(i));
      if( det )
      {
        TS_ASSERT_EQUALS(det->isMasked(), expectedMasked);
      }
      else
      {
        TS_FAIL("No detector defined");
      }
    }
        
  }
  
  void testFlagMasked()
  {
    auto ws = makeWorkspaceWithDetectors(2,2);
    // Now do a valid masking
    TS_ASSERT_THROWS_NOTHING( ws->flagMasked(0,1,0.75) );
    TS_ASSERT( ws->hasMaskedBins(0) );
    TS_ASSERT_EQUALS( ws->maskedBins(0).size(), 1 );
    TS_ASSERT_EQUALS( ws->maskedBins(0).begin()->first, 1 );
    TS_ASSERT_EQUALS( ws->maskedBins(0).begin()->second, 0.75 );
    //flagMasked() shouldn't change the y-value maskBins() tested below does that
    TS_ASSERT_EQUALS( ws->dataY(0)[1], 1.0 );

    // Now mask a bin earlier than above and check it's sorting properly
    TS_ASSERT_THROWS_NOTHING( ws->flagMasked(1,1) )
    TS_ASSERT_EQUALS( ws->maskedBins(1).size(), 1 )
    TS_ASSERT_EQUALS( ws->maskedBins(1).begin()->first, 1 )
    TS_ASSERT_EQUALS( ws->maskedBins(1).begin()->second, 1.0 )
    // Check the previous masking is still OK
    TS_ASSERT_EQUALS( ws->maskedBins(0).rbegin()->first, 1 )
    TS_ASSERT_EQUALS( ws->maskedBins(0).rbegin()->second, 0.75 )
  }

  void testMasking()
  {
    auto ws2 = makeWorkspaceWithDetectors(1,2);

    TS_ASSERT( !ws2->hasMaskedBins(0) );
    // Doesn't throw on invalid spectrum index, just returns false
    TS_ASSERT( !ws2->hasMaskedBins(1) );
    TS_ASSERT( !ws2->hasMaskedBins(-1) );

    // Will throw if nothing masked for spectrum
    TS_ASSERT_THROWS( ws2->maskedBins(0), Mantid::Kernel::Exception::IndexError );
    // Will throw if attempting to mask invalid spectrum
    TS_ASSERT_THROWS( ws2->maskBin(-1,1), Mantid::Kernel::Exception::IndexError );
    TS_ASSERT_THROWS( ws2->maskBin(1,1), Mantid::Kernel::Exception::IndexError );
    // ...or an invalid bin
    TS_ASSERT_THROWS( ws2->maskBin(0,-1), Mantid::Kernel::Exception::IndexError );
    TS_ASSERT_THROWS( ws2->maskBin(0,2), Mantid::Kernel::Exception::IndexError );

    // Now do a valid masking
    TS_ASSERT_THROWS_NOTHING( ws2->maskBin(0,1,0.5) );
    TS_ASSERT( ws2->hasMaskedBins(0) );
    TS_ASSERT_EQUALS( ws2->maskedBins(0).size(), 1 );
    TS_ASSERT_EQUALS( ws2->maskedBins(0).begin()->first, 1 );
    TS_ASSERT_EQUALS( ws2->maskedBins(0).begin()->second, 0.5 );
    TS_ASSERT_EQUALS( ws2->dataY(0)[1], 0.5 );

    // Now mask a bin earlier than above and check it's sorting properly
    TS_ASSERT_THROWS_NOTHING( ws2->maskBin(0,0) );
    TS_ASSERT_EQUALS( ws2->maskedBins(0).begin()->first, 0 );
    TS_ASSERT_EQUALS( ws2->maskedBins(0).begin()->second, 1.0 );
    TS_ASSERT_EQUALS( ws2->dataY(0)[0], 0.0 );
    // Check the previous masking is still OK
    TS_ASSERT_EQUALS( ws2->maskedBins(0).rbegin()->first, 1 );
    TS_ASSERT_EQUALS( ws2->maskedBins(0).rbegin()->second, 0.5 );
    TS_ASSERT_EQUALS( ws2->dataY(0)[1], 0.5 );
  }

  void testSize()
  {
    WorkspaceTester wkspace;
    wkspace.initialize(1,4,3);
    TS_ASSERT_EQUALS(wkspace.blocksize(), 3);
    TS_ASSERT_EQUALS(wkspace.size(), 3);
  }

  void testBinIndexOf()
  {
    WorkspaceTester wkspace;
    wkspace.initialize(1,4,2);
    //Data is all 1.0s
    wkspace.dataX(0)[1] = 2.0;
    wkspace.dataX(0)[2] = 3.0;
    wkspace.dataX(0)[3] = 4.0;

    TS_ASSERT_EQUALS(wkspace.getNumberHistograms(), 1);

    //First bin
    TS_ASSERT_EQUALS(wkspace.binIndexOf(1.3), 0);
    // Bin boundary
    TS_ASSERT_EQUALS(wkspace.binIndexOf(2.0), 0);
    // Mid range
    TS_ASSERT_EQUALS(wkspace.binIndexOf(2.5), 1);
    // Still second bin
    TS_ASSERT_EQUALS(wkspace.binIndexOf(2.001), 1);
    // Last bin
    TS_ASSERT_EQUALS(wkspace.binIndexOf(3.1), 2);
    // Last value
    TS_ASSERT_EQUALS(wkspace.binIndexOf(4.0), 2);

    // Error handling

    // Bad index value
    TS_ASSERT_THROWS(wkspace.binIndexOf(2.5, 1), std::out_of_range);
    TS_ASSERT_THROWS(wkspace.binIndexOf(2.5, -1), std::out_of_range);

    // Bad X values
    TS_ASSERT_THROWS(wkspace.binIndexOf(5.), std::out_of_range);
    TS_ASSERT_THROWS(wkspace.binIndexOf(0.), std::out_of_range);
  }

  void test_nexus_spectraMap()
  {
    NexusTestHelper th(true);
    th.createFile("MatrixWorkspaceTest.nxs");
    auto ws = makeWorkspaceWithDetectors(100, 50);
    std::vector<int> spec;
    for (int i=0; i<100; i++)
    {
      // Give some funny numbers, so it is not the default
      ws->getSpectrum(size_t(i))->setSpectrumNo( i * 11 );
      ws->getSpectrum(size_t(i))->setDetectorID(99-i);
      spec.push_back(i);
    }
    // Save that to the NXS file
    TS_ASSERT_THROWS_NOTHING( ws->saveSpectraMapNexus(th.file, spec); );

  }

  void test_get_neighbours_exact()
  {
    //Create a nearest neighbours product, which can be returned.
    SpectrumDistanceMap map;
    MockNearestNeighbours* product = new MockNearestNeighbours;
    EXPECT_CALL(*product, neighbours(_)).WillRepeatedly(Return(map));
    EXPECT_CALL(*product, die()).Times(1); //Created once and destroyed once!

    //Create a factory, for generating the nearest neighbour products
    MockNearestNeighboursFactory* factory = new MockNearestNeighboursFactory;
    EXPECT_CALL(*factory, create(_,_,_,_)).Times(1).WillOnce(Return(product));

    WorkspaceTester wkspace(factory);
    wkspace.initialize(1,4,3);
    wkspace.getNeighboursExact(0, 1); //First call should construct nearest neighbours before calling ::neighbours
    wkspace.getNeighboursExact(0, 1); //Second call should not construct nearest neighbours before calling ::neighbours
  }

  void test_get_neighbours_radius()
  {
    //Create a nearest neighbours product, which can be returned.
    SpectrumDistanceMap map;
    MockNearestNeighbours* product = new MockNearestNeighbours;
    EXPECT_CALL(*product, neighboursInRadius(_,_)).WillRepeatedly(Return(map));
    EXPECT_CALL(*product, die()).Times(1); //Created once and destroyed once!

    //Create a factory, for generating the nearest neighbour products
    MockNearestNeighboursFactory* factory = new MockNearestNeighboursFactory;
    EXPECT_CALL(*factory, create(_,_,_)).Times(1).WillOnce(Return(product));

    WorkspaceTester wkspace(factory);
    wkspace.initialize(1,4,3);
    wkspace.getNeighbours(0, 1); //First call should construct nearest neighbours before calling ::neighbours
    wkspace.getNeighbours(0, 1); //Second call should not construct nearest neighbours before calling ::neighbours
  }

  void test_reset_neighbours()
  {
    //Create a nearest neighbours product, which can be returned.
    SpectrumDistanceMap map;
    MockNearestNeighbours* product = new MockNearestNeighbours;
    EXPECT_CALL(*product, neighboursInRadius(_,_)).WillRepeatedly(Return(map));
    EXPECT_CALL(*product, die()).Times(1); //Should be explicitly called upon reset.

    //Create a factory, for generating the nearest neighbour products
    MockNearestNeighboursFactory* factory = new MockNearestNeighboursFactory;
    EXPECT_CALL(*factory, create(_,_,_)).Times(1).WillOnce(Return(product));

    WorkspaceTester wkspace(factory);
    wkspace.initialize(1,4,3);
    wkspace.getNeighbours(0, 1); //First call should construct nearest neighbours before calling ::neighbours
    wkspace.rebuildNearestNeighbours(); //should cause die.

    TSM_ASSERT("Nearest neigbhbours Factory has not been used as expected", Mock::VerifyAndClearExpectations(factory));
    TSM_ASSERT("Nearest neigbhbours Product has not been used as expected", Mock::VerifyAndClearExpectations(product));
  }

  void test_rebuild_after_reset_neighbours()
  {
    SpectrumDistanceMap mapA, mapB, mapC;

    MockNearestNeighbours* productA = new MockNearestNeighbours;
    EXPECT_CALL(*productA, neighboursInRadius(_,_)).WillRepeatedly(Return(mapA));
    EXPECT_CALL(*productA, die()).Times(1); 

    MockNearestNeighbours* productB = new MockNearestNeighbours;
    EXPECT_CALL(*productB, neighboursInRadius(_,_)).WillRepeatedly(Return(mapB));
    EXPECT_CALL(*productB, die()).Times(1); 

    MockNearestNeighbours* productC = new MockNearestNeighbours;
    EXPECT_CALL(*productC, neighboursInRadius(_,_)).WillRepeatedly(Return(mapC));
    EXPECT_CALL(*productC, die()).Times(1); 

    //Create a factory, for generating the nearest neighbour products
    MockNearestNeighboursFactory* factory = new MockNearestNeighboursFactory;
    EXPECT_CALL(*factory, create(_,_,_)).Times(3)
    .WillOnce(Return(productA))
    .WillOnce(Return(productB))
    .WillOnce(Return(productC));

    WorkspaceTester wkspace(factory);
    wkspace.initialize(1,4,3);
    wkspace.getNeighbours(0, 1); //First call should construct nearest neighbours before calling ::neighbours
    wkspace.rebuildNearestNeighbours(); //should cause die.
    wkspace.getNeighbours(0, 1); //should cause creation for radius type call
    wkspace.rebuildNearestNeighbours(); //should cause die.
    wkspace.getNeighbours(0, 1); //should cause creation for number of neighbours type call
    wkspace.rebuildNearestNeighbours(); //should cause die. allows expectations to be checked, otherwise die called on nn destruction!

    TSM_ASSERT("Nearest neigbhbours Factory has not been used as expected", Mock::VerifyAndClearExpectations(factory));
    TSM_ASSERT("Nearest neigbhbours ProductA has not been used as expected", Mock::VerifyAndClearExpectations(productA));
    TSM_ASSERT("Nearest neigbhbours ProductB has not been used as expected", Mock::VerifyAndClearExpectations(productB));
    TSM_ASSERT("Nearest neigbhbours ProductC has not been used as expected", Mock::VerifyAndClearExpectations(productC));
  }


  /** Properly, this tests a method on Instrument, not MatrixWorkspace, but they
   * are related.
   */
  void test_isDetectorMasked()
  {
    auto ws = makeWorkspaceWithDetectors(100, 10);
    Instrument_const_sptr inst = ws->getInstrument();
    // Make sure the instrument is parametrized so that the test is thorough
    TS_ASSERT( inst->isParametrized() );
    TS_ASSERT( !inst->isDetectorMasked(1) );
    TS_ASSERT( !inst->isDetectorMasked(19) );
    // Mask then check that it returns as masked
    TS_ASSERT( ws->getSpectrum(19)->hasDetectorID(19) );
    ws->maskWorkspaceIndex(19);
    TS_ASSERT( inst->isDetectorMasked(19) );
  }

  /** Check if any of a list of detectors are masked */
  void test_isDetectorMasked_onASet()
  {
    auto ws = makeWorkspaceWithDetectors(100, 10);
    Instrument_const_sptr inst = ws->getInstrument();
    // Make sure the instrument is parametrized so that the test is thorough
    TS_ASSERT( inst->isParametrized() );

    // Mask detector IDs 8 and 9
    ws->maskWorkspaceIndex(8);
    ws->maskWorkspaceIndex(9);

    std::set<detid_t> dets;
    TSM_ASSERT("No detector IDs = not masked", !inst->isDetectorMasked(dets) );
    dets.insert(6);
    TSM_ASSERT("Detector is not masked", !inst->isDetectorMasked(dets) );
    dets.insert(7);
    TSM_ASSERT("Detectors are not masked", !inst->isDetectorMasked(dets) );
    dets.insert(8);
    TSM_ASSERT("If any detector is not masked, return false", !inst->isDetectorMasked(dets) );
    // Start again
    dets.clear();
    dets.insert(8);
    TSM_ASSERT("If all detectors are not masked, return true", inst->isDetectorMasked(dets) );
    dets.insert(9);
    TSM_ASSERT("If all detectors are not masked, return true", inst->isDetectorMasked(dets) );
    dets.insert(10);
    TSM_ASSERT("If any detector is not masked, return false", !inst->isDetectorMasked(dets) );
  }

  void test_hasGroupedDetectors()
  {
    auto ws = makeWorkspaceWithDetectors(5, 1);
    TS_ASSERT_EQUALS( ws->hasGroupedDetectors(), false);

	  ws->getSpectrum(0)->addDetectorID(3);
    TS_ASSERT_EQUALS( ws->hasGroupedDetectors(), true);
 
  }

  void test_getSpectrumToWorkspaceIndexMap()
  {
    WorkspaceTester ws;
    ws.initialize(2,1,1);
    const auto map = ws.getSpectrumToWorkspaceIndexMap();
    TS_ASSERT_EQUALS( map.size(), 2 );
    TS_ASSERT_EQUALS( map.begin()->first, 1 );
    TS_ASSERT_EQUALS( map.begin()->second, 0 );
    TS_ASSERT_EQUALS( map.rbegin()->first, 2 );
    TS_ASSERT_EQUALS( map.rbegin()->second, 1 );

    // Check it throws for non-spectra axis
    ws.replaceAxis(1,new NumericAxis(1));
    TS_ASSERT_THROWS( ws.getSpectrumToWorkspaceIndexMap(), std::runtime_error);
  }

  void test_getDetectorIDToWorkspaceIndexMap()
  {
    auto ws = makeWorkspaceWithDetectors(5, 1);
    detid2index_map idmap = ws->getDetectorIDToWorkspaceIndexMap(true);

    TS_ASSERT_EQUALS( idmap.size(), 5 );
    int i = 0;
    for ( auto it = idmap.begin(); it != idmap.end(); ++it, ++i )
    {
      TS_ASSERT_EQUALS( idmap.count(i), 1 );
      TS_ASSERT_EQUALS( idmap[i], i );
    }

    ws->getSpectrum(2)->addDetectorID(99); // Set a second ID on one spectrum
    TS_ASSERT_THROWS( ws->getDetectorIDToWorkspaceIndexMap(true), std::runtime_error );
    detid2index_map idmap2 = ws->getDetectorIDToWorkspaceIndexMap();
    TS_ASSERT_EQUALS( idmap2.size(), 6 );
  }

  void test_getDetectorIDToWorkspaceIndexVector()
  {
    auto ws = makeWorkspaceWithDetectors(100, 10);
    std::vector<size_t> out;
    detid_t offset = -1234;
    TS_ASSERT_THROWS_NOTHING( ws->getDetectorIDToWorkspaceIndexVector(out, offset) );
    TS_ASSERT_EQUALS( offset, 0);
    TS_ASSERT_EQUALS( out.size(), 100);
    TS_ASSERT_EQUALS( out[0], 0);
    TS_ASSERT_EQUALS( out[1], 1);
    TS_ASSERT_EQUALS( out[99], 99);

    // Create some discontinuities and check that the default value is there
    // Have to create a whole new instrument to keep things consistent, since the detector ID
    // is stored in at least 3 places
    auto inst = boost::make_shared<Instrument>("TestInstrument");
    ws->setInstrument(inst);
    // We get a 1:1 map by default so the detector ID should match the spectrum number
    for( size_t i = 0; i < ws->getNumberHistograms(); ++i )
    {
      detid_t detid = static_cast<detid_t>(i);
      // Create a detector for each spectra
      if ( i == 0 ) detid = -1;
      if ( i == 99 ) detid = 110;
      Detector * det = new Detector("pixel", detid, inst.get());
      inst->add(det);
      inst->markAsDetector(det);
      ws->getSpectrum(i)->addDetectorID(detid);
    }
    ws->getSpectrum(66)->clearDetectorIDs();

    TS_ASSERT_THROWS_NOTHING( ws->getDetectorIDToWorkspaceIndexVector(out, offset) );
    TS_ASSERT_EQUALS( offset, 1 );
    TS_ASSERT_EQUALS( out.size(), 112 );
    TS_ASSERT_EQUALS( out[66+offset], std::numeric_limits<size_t>::max() );
    TS_ASSERT_EQUALS( out[99+offset], 99 );
    TS_ASSERT_EQUALS( out[105+offset], std::numeric_limits<size_t>::max() );
    TS_ASSERT_EQUALS( out[110+offset], 99 );
  }

  void test_getSpectrumToWorkspaceIndexVector()
  {
    auto ws = makeWorkspaceWithDetectors(100, 10);
    std::vector<size_t> out;
    detid_t offset = -1234;
    TS_ASSERT_THROWS_NOTHING( ws->getSpectrumToWorkspaceIndexVector(out, offset) );
    TS_ASSERT_EQUALS( offset, -1);
    TS_ASSERT_EQUALS( out.size(), 100);
    TS_ASSERT_EQUALS( out[0], 0);
    TS_ASSERT_EQUALS( out[1], 1);
    TS_ASSERT_EQUALS( out[99], 99);
  }

  void test_getSignalAtCoord()
  {
    WorkspaceTester ws;
    // Matrix with 4 spectra, 5 bins each
    ws.initialize(4,6,5);
    for (size_t wi=0; wi<4; wi++)
      for (size_t x=0; x<6; x++)
      {
        ws.dataX(wi)[x] = double(x);
        if (x<5)
        {
          ws.dataY(wi)[x] = double(wi*10 + x);
          ws.dataE(wi)[x] = double((wi*10 + x)*2);
        }
      }
    coord_t coords[2] = {0.5, 1.0};
    TS_ASSERT_DELTA(ws.getSignalAtCoord(coords, Mantid::API::NoNormalization), 0.0, 1e-5);
    coords[0] = 1.5;
    TS_ASSERT_DELTA(ws.getSignalAtCoord(coords, Mantid::API::NoNormalization), 1.0, 1e-5);
  }

  void test_getCoordAtSignal_regression()
  {
    /*
    Having more spectrum numbers (acutally vertical axis increments) than x bins in VolumeNormalisation mode
    should not cause any issues.
    */
    WorkspaceTester ws;
    const int nVertical = 4;

    const int nBins = 2;
    const int nYValues = 1;
    ws.initialize(nVertical, nBins, nYValues);
    NumericAxis* verticalAxis = new NumericAxis(nVertical);
    for(int i = 0; i < nVertical; ++i)
    {
      for(int j = 0; j < nBins; ++j)
      {
        if( j < nYValues )
        {
          ws.dataY(i)[j] = 1.0; // All y values are 1.
          ws.dataE(i)[j] = j;
        }
        ws.dataX(i)[j] = j; // x increments by 1
      }
      verticalAxis->setValue(i, double(i)); // Vertical axis increments by 1.
    }
    ws.replaceAxis(1, verticalAxis);
    // Signal is always 1 and volume of each box is 1. Therefore normalized signal values by volume should always be 1.

    // Test at the top right.
    coord_t coord_top_right[2] = {static_cast<float>(ws.readX(0).back()),  float(0)};
    signal_t value = 0;
    TS_ASSERT_THROWS_NOTHING(value = ws.getSignalAtCoord(coord_top_right, VolumeNormalization));
    TS_ASSERT_EQUALS(1.0, value);

    // Test at another location just to be sure.
    coord_t coord_bottom_left[2] = {static_cast<float>(ws.readX(nVertical-1)[1]),  float(nVertical-1) };
    TS_ASSERT_THROWS_NOTHING(value = ws.getSignalAtCoord(coord_bottom_left, VolumeNormalization));
    TS_ASSERT_EQUALS(1.0, value);
  }

  void test_setMDMasking()
  {
    WorkspaceTester ws;
    TSM_ASSERT_THROWS("Characterisation test. This is not implemented.", ws.setMDMasking(NULL), std::runtime_error);
  }

  void test_clearMDMasking()
  {
    WorkspaceTester ws;
    TSM_ASSERT_THROWS("Characterisation test. This is not implemented.", ws.clearMDMasking(), std::runtime_error);
  }

  void test_getSpecialCoordinateSystem_default()
  {
    WorkspaceTester ws;
    TSM_ASSERT_EQUALS("Should default to no special coordinate system.", Mantid::Kernel::None, ws.getSpecialCoordinateSystem());
  }

  void test_getFirstPulseTime_getLastPulseTime()
  {
    WorkspaceTester ws;
    auto proton_charge = new TimeSeriesProperty<double>("proton_charge");
    DateAndTime startTime("2013-04-21T10:40:00");
    proton_charge->addValue( startTime, 1.0E-7 );
    proton_charge->addValue( startTime+1.0, 2.0E-7 );
    proton_charge->addValue( startTime+2.0, 3.0E-7 );
    proton_charge->addValue( startTime+3.0, 4.0E-7 );
    ws.mutableRun().addLogData(proton_charge);

    TS_ASSERT_EQUALS( ws.getFirstPulseTime(), startTime );
    TS_ASSERT_EQUALS( ws.getLastPulseTime(), startTime+3.0 );
  }

  void test_getFirstPulseTime_getLastPulseTime_SNS1990bug()
  {
    WorkspaceTester ws;
    auto proton_charge = new TimeSeriesProperty<double>("proton_charge");
    DateAndTime startTime("1990-12-31T23:59:00");
    proton_charge->addValue( startTime, 1.0E-7 );
    proton_charge->addValue( startTime+1.0, 2.0E-7 );
    ws.mutableRun().addLogData(proton_charge);

    // If fewer than 100 entries (unlikely to happen in reality), you just get back the last one
    TS_ASSERT_EQUALS( ws.getFirstPulseTime(), startTime+1.0 );

    for ( int i = 2; i < 62; ++i )
    {
      proton_charge->addValue( startTime+static_cast<double>(i), 1.0E-7 );
    }
    TS_ASSERT_EQUALS( ws.getFirstPulseTime(), DateAndTime("1991-01-01T00:00:00") );
  }

  void test_getFirstPulseTime_getLastPulseTime_throws_if_protoncharge_missing_or_empty()
  {
    WorkspaceTester ws;
    TS_ASSERT_THROWS( ws.getFirstPulseTime(), std::runtime_error );
    TS_ASSERT_THROWS( ws.getLastPulseTime(), std::runtime_error );
    ws.mutableRun().addLogData(new TimeSeriesProperty<double>("proton_charge"));
    TS_ASSERT_THROWS( ws.getFirstPulseTime(), std::runtime_error );
    TS_ASSERT_THROWS( ws.getLastPulseTime(), std::runtime_error );
  }

  void test_getFirstPulseTime_getLastPulseTime_throws_if_protoncharge_wrong_type()
  {
    WorkspaceTester ws;
    auto proton_charge = new TimeSeriesProperty<int>("proton_charge");
    proton_charge->addValue("2013-04-21T10:19:10",1);
    proton_charge->addValue("2013-04-21T10:19:12",2);
    ws.mutableRun().addLogData(proton_charge);
    TS_ASSERT_THROWS( ws.getFirstPulseTime(), std::invalid_argument );
    TS_ASSERT_THROWS( ws.getLastPulseTime(), std::invalid_argument );

    ws.mutableRun().addProperty(new PropertyWithValue<double>("proton_charge",99.0),true);
    TS_ASSERT_THROWS( ws.getFirstPulseTime(), std::invalid_argument );
    TS_ASSERT_THROWS( ws.getLastPulseTime(), std::invalid_argument );
  }

  void test_getXMinMax()
  {
    double xmin, xmax;
    ws->getXMinMax(xmin,xmax);
    TS_ASSERT_EQUALS(xmin, 1.0);
    TS_ASSERT_EQUALS(xmax, 1.0);
    TS_ASSERT_EQUALS(ws->getXMin(), 1.0);
    TS_ASSERT_EQUALS(ws->getXMax(), 1.0);
  }

  void test_monitorWorkspace()
  {
    auto ws = boost::make_shared<WorkspaceTester>();
    TSM_ASSERT( "There should be no monitor workspace by default", ! ws->monitorWorkspace() )

    auto ws2 = boost::make_shared<WorkspaceTester>();
    ws->setMonitorWorkspace(ws2);
    TSM_ASSERT_EQUALS( "Monitor workspace not successfully set", ws->monitorWorkspace(), ws2 )

    ws->setMonitorWorkspace(boost::shared_ptr<MatrixWorkspace>());
    TSM_ASSERT( "Monitor workspace not successfully reset", ! ws->monitorWorkspace() )
  }

  void test_getXIndex()
  {
    WorkspaceTester ws;
    ws.init(1,4,3);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    X[1] = 2.0;
    X[2] = 3.0;
    X[3] = 4.0;

    auto ip = ws.getXIndex( 0, 0.0, true );
    TS_ASSERT_EQUALS( ip.first, 0 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 0.0, false );
    TS_ASSERT_EQUALS( ip.first, 4 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 1.0, true );
    TS_ASSERT_EQUALS( ip.first, 0 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 1.0, false );
    TS_ASSERT_EQUALS( ip.first, 4 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 5.0, true );
    TS_ASSERT_EQUALS( ip.first, 4 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 5.0, false );
    TS_ASSERT_EQUALS( ip.first, 3 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 4.0, true );
    TS_ASSERT_EQUALS( ip.first, 4 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 4.0, false );
    TS_ASSERT_EQUALS( ip.first, 3 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 5.0, true, 5 );
    TS_ASSERT_EQUALS( ip.first, 4 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 5.0, false, 5 );
    TS_ASSERT_EQUALS( ip.first, 4 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 3.0, true, 5 );
    TS_ASSERT_EQUALS( ip.first, 4 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 3.0, false, 5 );
    TS_ASSERT_EQUALS( ip.first, 4 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 4.0, true, 5 );
    TS_ASSERT_EQUALS( ip.first, 4 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 4.0, false, 5 );
    TS_ASSERT_EQUALS( ip.first, 4 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 4.0, true, 4 );
    TS_ASSERT_EQUALS( ip.first, 4 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 4.0, false, 4 );
    TS_ASSERT_EQUALS( ip.first, 4 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 4.0, true, 3 );
    TS_ASSERT_EQUALS( ip.first, 4 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 4.0, false, 3 );
    TS_ASSERT_EQUALS( ip.first, 3 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 4.0, true );
    TS_ASSERT_EQUALS( ip.first, 4 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 4.0, false );
    TS_ASSERT_EQUALS( ip.first, 3 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 2.0, true, 3 );
    TS_ASSERT_EQUALS( ip.first, 4 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 2.0, false, 3 );
    TS_ASSERT_EQUALS( ip.first, 3 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 1.0, true, 3 );
    TS_ASSERT_EQUALS( ip.first, 4 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 1.0, false, 3 );
    TS_ASSERT_EQUALS( ip.first, 3 );
    TS_ASSERT_DELTA( ip.second, 0.0, 1e-15 );

    ip = ws.getXIndex( 0, 2.1, true );
    TS_ASSERT_EQUALS( ip.first, 1 );
    TS_ASSERT_DELTA( ip.second, 0.1, 1e-15 );

    ip = ws.getXIndex( 0, 2.1, false );
    TS_ASSERT_EQUALS( ip.first, 2 );
    TS_ASSERT_DELTA( ip.second, 0.9, 1e-15 );
  }

  void test_getImage_0_width()
  {
    WorkspaceTester ws;
    ws.init(9,2,1);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    X[1] = 2.0;
    const size_t start = 0;
    const size_t stop  = 8;
    size_t width = 0;
    TS_ASSERT_THROWS( ws.getImageY(start,stop,width), std::runtime_error );
    width = 3;
    TS_ASSERT_THROWS_NOTHING( ws.getImageY(start,stop,width) );
  }

  void test_getImage_wrong_start()
  {
    WorkspaceTester ws;
    ws.init(9,2,1);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    X[1] = 2.0;
    size_t start = 10;
    size_t stop  = 8;
    size_t width = 3;
    TS_ASSERT_THROWS( ws.getImageY(start,stop,width), std::runtime_error );
    start = 9;
    TS_ASSERT_THROWS( ws.getImageY(start,stop,width), std::runtime_error );
    start = 0;
    TS_ASSERT_THROWS_NOTHING( ws.getImageY(start,stop,width) );
  }

  void test_getImage_wrong_stop()
  {
    WorkspaceTester ws;
    ws.init(9,2,1);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    X[1] = 2.0;
    size_t start = 0;
    size_t stop  = 18;
    size_t width = 3;
    TS_ASSERT_THROWS( ws.getImageY(start,stop,width), std::runtime_error );
    stop = 9;
    TS_ASSERT_THROWS( ws.getImageY(start,stop,width), std::runtime_error );
    stop = 8;
    TS_ASSERT_THROWS_NOTHING( ws.getImageY(start,stop,width) );
  }

  void test_getImage_empty_set()
  {
    WorkspaceTester ws;
    ws.init(9,2,1);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    X[1] = 2.0;
    size_t start = 1;
    size_t stop  = 0;
    size_t width = 1;
    TS_ASSERT_THROWS( ws.getImageY(start,stop,width), std::runtime_error );
    stop = 1;
    TS_ASSERT_THROWS_NOTHING( ws.getImageY(start,stop,width) );
  }

  void test_getImage_non_rectangular()
  {
    WorkspaceTester ws;
    ws.init(9,2,1);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    X[1] = 2.0;
    size_t start = 0;
    size_t stop  = 7;
    size_t width = 3;
    TS_ASSERT_THROWS( ws.getImageY(start,stop,width), std::runtime_error );
  }

  void test_getImage_wrong_indexStart()
  {
    WorkspaceTester ws;
    ws.init(9,2,1);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    X[1] = 2.0;
    const size_t start = 0;
    const size_t stop  = 8;
    const size_t width = 3;
    double startX = 3;
    double endX = 4;
    TS_ASSERT_THROWS( ws.getImageY(start,stop,width,startX,endX), std::runtime_error );

    WorkspaceTester wsh;
    wsh.init(9,1,1);
    startX = 2;
    endX = 2;
    TS_ASSERT_THROWS( wsh.getImageY(start,stop,width,startX,endX), std::runtime_error );
  }

  void test_getImage_wrong_indexEnd()
  {
    WorkspaceTester ws;
    ws.init(9,2,1);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    X[1] = 2.0;
    const size_t start = 0;
    const size_t stop  = 8;
    const size_t width = 3;
    double startX = 1.0;
    double endX = 0.0;
    TS_ASSERT_THROWS( ws.getImageY(start,stop,width,startX,endX), std::runtime_error );

    WorkspaceTester wsh;
    wsh.init(9,2,2);
    auto &X1 = ws.dataX(0);
    X1[0] = 1.0;
    X1[1] = 2.0;
    startX = 1.0;
    endX = 0.0;
    TS_ASSERT_THROWS( wsh.getImageY(start,stop,width,startX,endX), std::runtime_error );
  }

  void test_getImage_single_bin_histo()
  {
    WorkspaceTester ws;
    ws.init(9,2,1);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    X[1] = 2.0;
    for(size_t i = 0; i < ws.getNumberHistograms(); ++i)
    {
      ws.dataY(i)[0] = static_cast<double>( i + 1 );
    }
    const size_t start = 0;
    const size_t stop  = 8;
    const size_t width = 3;
    double startX = 0;
    double endX = 3;
    Mantid::API::MantidImage_sptr image;
    TS_ASSERT_THROWS_NOTHING( image = ws.getImageY(start,stop,width,startX,endX) );
    if ( !image ) return;
    TS_ASSERT_EQUALS( image->size(), 3 );
    TS_ASSERT_EQUALS( (*image)[0].size(), 3 );
    TS_ASSERT_EQUALS( (*image)[1].size(), 3 );
    TS_ASSERT_EQUALS( (*image)[2].size(), 3 );

    TS_ASSERT_EQUALS( (*image)[0][0], 1 );
    TS_ASSERT_EQUALS( (*image)[0][1], 2 );
    TS_ASSERT_EQUALS( (*image)[0][2], 3 );
    TS_ASSERT_EQUALS( (*image)[1][0], 4 );
    TS_ASSERT_EQUALS( (*image)[1][1], 5 );
    TS_ASSERT_EQUALS( (*image)[1][2], 6 );
    TS_ASSERT_EQUALS( (*image)[2][0], 7 );
    TS_ASSERT_EQUALS( (*image)[2][1], 8 );
    TS_ASSERT_EQUALS( (*image)[2][2], 9 );

  }

  void test_getImage_single_bin_points()
  {
    WorkspaceTester ws;
    ws.init(9,1,1);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    for(size_t i = 0; i < ws.getNumberHistograms(); ++i)
    {
      ws.dataY(i)[0] = static_cast<double>( i + 1 );
    }
    const size_t start = 0;
    const size_t stop  = 8;
    const size_t width = 3;
    double startX = 1;
    double endX = 1;
    Mantid::API::MantidImage_sptr image;
    TS_ASSERT_THROWS_NOTHING( image = ws.getImageY(start,stop,width,startX,endX) );
    if ( !image ) return;
    TS_ASSERT_EQUALS( image->size(), 3 );
    TS_ASSERT_EQUALS( (*image)[0].size(), 3 );
    TS_ASSERT_EQUALS( (*image)[1].size(), 3 );
    TS_ASSERT_EQUALS( (*image)[2].size(), 3 );

    TS_ASSERT_EQUALS( (*image)[0][0], 1 );
    TS_ASSERT_EQUALS( (*image)[0][1], 2 );
    TS_ASSERT_EQUALS( (*image)[0][2], 3 );
    TS_ASSERT_EQUALS( (*image)[1][0], 4 );
    TS_ASSERT_EQUALS( (*image)[1][1], 5 );
    TS_ASSERT_EQUALS( (*image)[1][2], 6 );
    TS_ASSERT_EQUALS( (*image)[2][0], 7 );
    TS_ASSERT_EQUALS( (*image)[2][1], 8 );
    TS_ASSERT_EQUALS( (*image)[2][2], 9 );

  }

  void test_getImage_multi_bin_histo()
  {
    WorkspaceTester ws;
    ws.init(9,4,3);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    X[1] = 2.0;
    X[2] = 3.0;
    X[3] = 4.0;
    for(size_t i = 0; i < ws.getNumberHistograms(); ++i)
    {
      ws.dataY(i)[0] = static_cast<double>( i + 1 );
      ws.dataY(i)[1] = static_cast<double>( i + 2 );
      ws.dataY(i)[2] = static_cast<double>( i + 3 );
    }
    const size_t start = 0;
    const size_t stop  = 8;
    const size_t width = 3;
    Mantid::API::MantidImage_sptr image;
    TS_ASSERT_THROWS_NOTHING( image = ws.getImageY(start,stop,width) );
    if ( !image ) return;
    TS_ASSERT_EQUALS( image->size(), 3 );
    TS_ASSERT_EQUALS( (*image)[0].size(), 3 );
    TS_ASSERT_EQUALS( (*image)[1].size(), 3 );
    TS_ASSERT_EQUALS( (*image)[2].size(), 3 );

    TS_ASSERT_EQUALS( (*image)[0][0], 6 );
    TS_ASSERT_EQUALS( (*image)[0][1], 9 );
    TS_ASSERT_EQUALS( (*image)[0][2], 12 );
    TS_ASSERT_EQUALS( (*image)[1][0], 15 );
    TS_ASSERT_EQUALS( (*image)[1][1], 18 );
    TS_ASSERT_EQUALS( (*image)[1][2], 21 );
    TS_ASSERT_EQUALS( (*image)[2][0], 24 );
    TS_ASSERT_EQUALS( (*image)[2][1], 27 );
    TS_ASSERT_EQUALS( (*image)[2][2], 30 );

  }

  void test_getImage_multi_bin_points()
  {
    WorkspaceTester ws;
    ws.init(9,3,3);
    auto &X = ws.dataX(0);
    X[0] = 1.0;
    X[1] = 2.0;
    X[2] = 3.0;
    for(size_t i = 0; i < ws.getNumberHistograms(); ++i)
    {
      ws.dataY(i)[0] = static_cast<double>( i + 1 );
      ws.dataY(i)[1] = static_cast<double>( i + 2 );
      ws.dataY(i)[2] = static_cast<double>( i + 3 );
    }
    const size_t start = 0;
    const size_t stop  = 8;
    const size_t width = 3;
    Mantid::API::MantidImage_sptr image;
    TS_ASSERT_THROWS_NOTHING( image = ws.getImageY(start,stop,width) );
    if ( !image ) return;
    TS_ASSERT_EQUALS( image->size(), 3 );
    TS_ASSERT_EQUALS( (*image)[0].size(), 3 );
    TS_ASSERT_EQUALS( (*image)[1].size(), 3 );
    TS_ASSERT_EQUALS( (*image)[2].size(), 3 );

    TS_ASSERT_EQUALS( (*image)[0][0], 6 );
    TS_ASSERT_EQUALS( (*image)[0][1], 9 );
    TS_ASSERT_EQUALS( (*image)[0][2], 12 );
    TS_ASSERT_EQUALS( (*image)[1][0], 15 );
    TS_ASSERT_EQUALS( (*image)[1][1], 18 );
    TS_ASSERT_EQUALS( (*image)[1][2], 21 );
    TS_ASSERT_EQUALS( (*image)[2][0], 24 );
    TS_ASSERT_EQUALS( (*image)[2][1], 27 );
    TS_ASSERT_EQUALS( (*image)[2][2], 30 );

  }

  void test_setImage_too_large()
  {
    auto image = createImage(2,3);
    WorkspaceTester ws;
    ws.init(2,2,1);
    TS_ASSERT_THROWS( ws.setImageY( *image ), std::runtime_error );
  }

  void test_setImage_not_single_bin()
  {
    auto image = createImage(2,3);
    WorkspaceTester ws;
    ws.init(20,3,2);
    TS_ASSERT_THROWS( ws.setImageY( *image ), std::runtime_error );
  }

  void test_setImageY()
  {
    auto image = createImage(2,3);
    WorkspaceTester ws;
    ws.init(6,2,1);
    TS_ASSERT_THROWS_NOTHING( ws.setImageY( *image ) );
    TS_ASSERT_EQUALS( ws.readY(0)[0], 1 );
    TS_ASSERT_EQUALS( ws.readY(1)[0], 2 );
    TS_ASSERT_EQUALS( ws.readY(2)[0], 3 );
    TS_ASSERT_EQUALS( ws.readY(3)[0], 4 );
    TS_ASSERT_EQUALS( ws.readY(4)[0], 5 );
    TS_ASSERT_EQUALS( ws.readY(5)[0], 6 );
  }

  void test_setImageE()
  {
    auto image = createImage(2,3);
    WorkspaceTester ws;
    ws.init(6,2,1);
    TS_ASSERT_THROWS_NOTHING( ws.setImageE( *image ) );
    TS_ASSERT_EQUALS( ws.readE(0)[0], 1 );
    TS_ASSERT_EQUALS( ws.readE(1)[0], 2 );
    TS_ASSERT_EQUALS( ws.readE(2)[0], 3 );
    TS_ASSERT_EQUALS( ws.readE(3)[0], 4 );
    TS_ASSERT_EQUALS( ws.readE(4)[0], 5 );
    TS_ASSERT_EQUALS( ws.readE(5)[0], 6 );
  }

  void test_setImageY_start()
  {
    auto image = createImage(2,3);
    WorkspaceTester ws;
    ws.init(9,2,1);
    TS_ASSERT_THROWS_NOTHING( ws.setImageY( *image, 3 ) );
    TS_ASSERT_EQUALS( ws.readY(3)[0], 1 );
    TS_ASSERT_EQUALS( ws.readY(4)[0], 2 );
    TS_ASSERT_EQUALS( ws.readY(5)[0], 3 );
    TS_ASSERT_EQUALS( ws.readY(6)[0], 4 );
    TS_ASSERT_EQUALS( ws.readY(7)[0], 5 );
    TS_ASSERT_EQUALS( ws.readY(8)[0], 6 );
  }

  void test_setImageE_start()
  {
    auto image = createImage(2,3);
    WorkspaceTester ws;
    ws.init(9,2,1);
    TS_ASSERT_THROWS_NOTHING( ws.setImageE( *image, 2 ) );
    TS_ASSERT_EQUALS( ws.readE(2)[0], 1 );
    TS_ASSERT_EQUALS( ws.readE(3)[0], 2 );
    TS_ASSERT_EQUALS( ws.readE(4)[0], 3 );
    TS_ASSERT_EQUALS( ws.readE(5)[0], 4 );
    TS_ASSERT_EQUALS( ws.readE(6)[0], 5 );
    TS_ASSERT_EQUALS( ws.readE(7)[0], 6 );
  }

private:

  Mantid::API::MantidImage_sptr createImage(size_t width, size_t height)
  {
    auto image = new Mantid::API::MantidImage(height);
    double value = 1.0;
    for(auto row = image->begin(); row != image->end(); ++row)
    {
      row->resize( width );
      for(auto pixel = row->begin(); pixel != row->end(); ++pixel, value += 1.0)
      {
        *pixel = value;
      }
    }
    return Mantid::API::MantidImage_sptr( image );
  }

  boost::shared_ptr<MatrixWorkspace> ws;

};

#endif /*WORKSPACETEST_H_*/
