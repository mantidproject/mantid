#ifndef WORKSPACETEST_H_
#define WORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/Instrument/OneToOneSpectraDetectorMap.h"
#include <boost/scoped_ptr.hpp>

using std::size_t;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid;

namespace Mantid 
{

namespace DataObjects 
{

class WorkspaceTester : public MatrixWorkspace
{
public:
  WorkspaceTester() : MatrixWorkspace() {}
  virtual ~WorkspaceTester() {}

  // Empty overrides of virtual methods
  virtual size_t getNumberHistograms() const { return 1;}
  const std::string id() const {return "WorkspaceTester";}
  void init(const size_t&, const size_t& j, const size_t&)
  {
    vec.resize(j,1.0);
    // Put an 'empty' axis in to test the getAxis method
    m_axes.resize(1);
    m_axes[0] = new NumericAxis(1);
  }
  size_t size() const {return vec.size();}
  size_t blocksize() const {return vec.size();}
  MantidVec& dataX(size_t const ) {return vec;}
  MantidVec& dataY(size_t const ) {return vec;}
  MantidVec& dataE(size_t const ) {return vec;}
  MantidVec& dataDx(size_t const ) {return vec;}
  const MantidVec& dataX(size_t const) const {return vec;}
  const MantidVec& dataY(size_t const) const {return vec;}
  const MantidVec& dataE(size_t const) const {return vec;}
  const MantidVec& dataDx(size_t const) const {return vec;}
  Kernel::cow_ptr<MantidVec> refX(const size_t) const {return Kernel::cow_ptr<MantidVec>();}
  void setX(const size_t, const Kernel::cow_ptr<MantidVec>&) {}

private:
  MantidVec vec;
  int spec;
};

class WorkspaceTesterWithMaps : public MatrixWorkspace
{
public:
  WorkspaceTesterWithMaps() : MatrixWorkspace() {}
  virtual ~WorkspaceTesterWithMaps() {}

  // Empty overrides of virtual methods
  virtual size_t getNumberHistograms() const { return m_NVectors;}
  const std::string id() const {return "WorkspaceTester";}
  void init(const size_t& NVectors, const size_t& j, const size_t&)
  {
    m_NVectors = NVectors;
    vec.resize(j,1.0);
    // Put an 'empty' axis in to test the getAxis method
    m_axes.resize(2);
    m_axes[0] = new NumericAxis(1);

    //Spectrum # = 20 + workspace index.
    SpectraAxis * ax =  new SpectraAxis(m_NVectors);
    for (size_t i=0; i<m_NVectors; i++)
      ax->setValue(i, static_cast<double>(i)+20);
    m_axes[1] = ax;

    SpectraDetectorMap *newMap = new SpectraDetectorMap;
    this->replaceSpectraMap(newMap);
    //Detector id is 100 + workspace index = 80 + spectrum #
    for (size_t i=20; i<20+m_NVectors; i++)
    {
      std::vector<detid_t> vec;
      vec.push_back(static_cast<detid_t>(i)+80);
      newMap->addSpectrumEntries(static_cast<specid_t>(i), vec);
    }
  }
  size_t size() const {return vec.size();}
  size_t blocksize() const {return vec.size();}
  MantidVec& dataX(size_t const ) {return vec;}
  MantidVec& dataY(size_t const ) {return vec;}
  MantidVec& dataE(size_t const ) {return vec;}
  MantidVec& dataDx(size_t const ) {return vec;}
  const MantidVec& dataX(size_t const) const {return vec;}
  const MantidVec& dataY(size_t const) const {return vec;}
  const MantidVec& dataE(size_t const) const {return vec;}
  const MantidVec& dataDx(size_t const) const {return vec;}
  Kernel::cow_ptr<MantidVec> refX(const size_t) const {return Kernel::cow_ptr<MantidVec>();}
  void setX(const size_t, const Kernel::cow_ptr<MantidVec>&) {}

private:
  MantidVec vec;
  Mantid::specid_t spec;
  size_t m_NVectors;
};

DECLARE_WORKSPACE(WorkspaceTester)

class WorkspaceTesterWithDetectors : public MatrixWorkspace
{
public:
  WorkspaceTesterWithDetectors() : MatrixWorkspace() {}
  virtual ~WorkspaceTesterWithDetectors() {}

  // Empty overrides of virtual methods
  virtual size_t getNumberHistograms() const { return m_NVectors;}
  const std::string id() const {return "WorkspaceTester";}
  void init(const size_t& NVectors, const size_t&, const size_t&)
  {
    m_NVectors = NVectors;
    vec.resize(NVectors, MantidVec(1, 1.0));
    // Put an 'empty' axis in to test the getAxis method
    m_axes.resize(2);
    m_axes[0] = new NumericAxis(1);

    m_axes[1] =  new SpectraAxis(m_NVectors);
    for (size_t i=0; i<m_NVectors; i++)
    {
      this->getAxis(1)->spectraNo(i) = static_cast<specid_t>(i);
    }

    setInstrument(Instrument_sptr(new Instrument("TestInstrument")));
    Instrument_sptr inst = getBaseInstrument();
    
    for( size_t i = 0; i < NVectors; ++i )
    {
      // Create a detector for each spectra
      Detector * det = new Detector("pixel", static_cast<int>(i), inst.get());
      inst->add(det);
      inst->markAsDetector(det);
    }

  }
  size_t size() const {return vec.size();}
  size_t blocksize() const {return vec[0].size();}
  MantidVec& dataX(size_t const i) {return vec[i];}
  MantidVec& dataY(size_t const i) {return vec[i];}
  MantidVec& dataE(size_t const i) {return vec[i];}
  MantidVec& dataDx(size_t const i) {return vec[i];}
  const MantidVec& dataX(size_t const i) const {return vec[i];}
  const MantidVec& dataY(size_t const i) const {return vec[i];}
  const MantidVec& dataE(size_t const i) const {return vec[i];}
  const MantidVec& dataDx(size_t const i) const {return vec[i];}
  Kernel::cow_ptr<MantidVec> refX(const size_t) const {return Kernel::cow_ptr<MantidVec>();}
  void setX(const size_t, const Kernel::cow_ptr<MantidVec>&) {}

private:
  std::vector<MantidVec> vec;
  specid_t spec;
  size_t m_NVectors;
};

}
}// namespace



class MatrixWorkspaceTest : public CxxTest::TestSuite
{
public:
  MatrixWorkspaceTest() : ws(new Mantid::DataObjects::WorkspaceTester)
  {
    ws->initialize(1,1,1);
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

  void testGetInstrument()
  {
    boost::shared_ptr<IInstrument> i = ws->getInstrument();
    TS_ASSERT_EQUALS( ws->getInstrument()->type(), "Instrument" );
  }

  void test_That_A_Workspace_Gets_SpectraMap_When_Initialized_With_NVector_Elements()
  {
    MatrixWorkspace_sptr testWS(new Mantid::DataObjects::WorkspaceTester);
    // Starts with an empty one
    TS_ASSERT_EQUALS(testWS->spectraMap().nElements(), 0);
    const size_t nhist(10);
    testWS->initialize(nhist,1,1);
    TS_ASSERT_EQUALS(testWS->spectraMap().nElements(), nhist);    
  }

  void testSpectraMap()
  {
    MatrixWorkspace_sptr ws2 = WorkspaceFactory::Instance().create(ws,1,1,1);
    const Geometry::ISpectraDetectorMap &specs = ws2->spectraMap();
    TS_ASSERT_EQUALS( &(ws->spectraMap()), &specs );
  }

  void testReplacingSpectraMap()
  {
    boost::scoped_ptr<MatrixWorkspace> testWS(new Mantid::DataObjects::WorkspaceTester);
    testWS->initialize(1,1,1);
    const Geometry::ISpectraDetectorMap &specs = testWS->spectraMap();
    // Default one
    TS_ASSERT_EQUALS(specs.nElements(), 1);

    testWS->replaceSpectraMap(new OneToOneSpectraDetectorMap(1,10));
    // Has it been replaced
    TS_ASSERT_EQUALS(testWS->spectraMap().nElements(), 10);
  }
  
  void testSpectraMapCopiedWhenAWorkspaceIsCopied()
  {
    boost::shared_ptr<MatrixWorkspace> parent(new Mantid::DataObjects::WorkspaceTester);
    parent->initialize(1,1,1);
    parent->replaceSpectraMap(new OneToOneSpectraDetectorMap(1,10));
    TS_ASSERT_EQUALS(parent->spectraMap().nElements(), 10);
    MatrixWorkspace_sptr copied = WorkspaceFactory::Instance().create(parent,1,1,1);
    TS_ASSERT_EQUALS(copied->spectraMap().nElements(), 10);
  }

  void testGetSetSample()
  {
    TS_ASSERT( &ws->sample() );

    ws->mutableSample().setName("test");
    TS_ASSERT_EQUALS( ws->sample().getName(), "test" );
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
    TS_ASSERT_EQUALS( ws->axes(), 1 );
  }

  void testGetAxis()
  {
    TS_ASSERT_THROWS( ws->getAxis(-1), Exception::IndexError );
    TS_ASSERT_THROWS_NOTHING( ws->getAxis(0) );
    TS_ASSERT( ws->getAxis(0) );
    TS_ASSERT( ws->getAxis(0)->isNumeric() );
    TS_ASSERT_THROWS( ws->getAxis(1), Exception::IndexError );
  }

  void testReplaceAxis()
  {
    Axis* axBad = new SpectraAxis(5);
    TS_ASSERT_THROWS( ws->replaceAxis(0,axBad), std::runtime_error );
    delete axBad;
    Axis* ax = new SpectraAxis(1);
    TS_ASSERT_THROWS( ws->replaceAxis(1,ax), Exception::IndexError );
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

  void testWholeSpectraMasking()
  {
    boost::shared_ptr<MatrixWorkspace> workspace(new Mantid::DataObjects::WorkspaceTesterWithDetectors);
    // Workspace has 3 spectra, each 1 in length
    const int numHist(3);
    workspace->initialize(numHist,1,1);

    // Initially un masked
    for( int i = 0; i < numHist; ++i )
    {
      TS_ASSERT_EQUALS(workspace->readY(i)[0], 1.0);
      TS_ASSERT_EQUALS(workspace->readE(i)[0], 1.0);

      IDetector_sptr det;
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
    const double maskValue(-10.0);
    workspace->maskWorkspaceIndex(1,maskValue);
    workspace->maskWorkspaceIndex(2,maskValue);

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
	expectedValue = maskValue;
	expectedMasked = true;
      }
      TS_ASSERT_EQUALS(workspace->readY(i)[0], expectedValue);
      TS_ASSERT_EQUALS(workspace->readE(i)[0], expectedValue);

      IDetector_sptr det;
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

  void testMasking()
  {
    MatrixWorkspace *ws2 = new Mantid::DataObjects::WorkspaceTester;
    ws2->initialize(1,2,2);
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
    // This will be 0.25 (1*0.5*0.5) because in the test class the same vector is used for both E & Y
    TS_ASSERT_EQUALS( ws2->dataY(0)[1], 0.25 );

    // Now mask a bin earlier than above and check it's sorting properly
    TS_ASSERT_THROWS_NOTHING( ws2->maskBin(0,0) );
    TS_ASSERT( ws2->hasMaskedBins(0) );
    TS_ASSERT_EQUALS( ws2->maskedBins(0).size(), 2 );
    TS_ASSERT_EQUALS( ws2->maskedBins(0).begin()->first, 0 );
    TS_ASSERT_EQUALS( ws2->maskedBins(0).begin()->second, 1.0 );
    // This will be 0.25 (1*0.5*0.5) because in the test class the same vector is used for both E & Y
    TS_ASSERT_EQUALS( ws2->dataY(0)[0], 0.0 );
    // Check the previous masking is still OK
    TS_ASSERT_EQUALS( ws2->maskedBins(0).rbegin()->first, 1 );
    TS_ASSERT_EQUALS( ws2->maskedBins(0).rbegin()->second, 0.5 );
    TS_ASSERT_EQUALS( ws2->dataY(0)[1], 0.25 );

    delete ws2;
  }

  void testSize()
  {
    MatrixWorkspace *wkspace = new Mantid::DataObjects::WorkspaceTester;
    //Test workspace takes the middle value here as the size of each vector
    wkspace->initialize(1,4,3);

    TS_ASSERT_EQUALS(wkspace->blocksize(), 4);
    TS_ASSERT_EQUALS(wkspace->size(), 4);

  }

  void testBinIndexOf()
  {
    MatrixWorkspace *wkspace = new Mantid::DataObjects::WorkspaceTester;
    wkspace->initialize(1,4,2);
    //Data is all 1.0s
    wkspace->dataX(0)[1] = 2.0;
    wkspace->dataX(0)[2] = 3.0;
    wkspace->dataX(0)[3] = 4.0;

    TS_ASSERT_EQUALS(wkspace->getNumberHistograms(), 1);

    //First bin
    TS_ASSERT_EQUALS(wkspace->binIndexOf(1.3), 0);
    // Bin boundary
    TS_ASSERT_EQUALS(wkspace->binIndexOf(2.0), 0);
    // Mid range
    TS_ASSERT_EQUALS(wkspace->binIndexOf(2.5), 1);
    // Still second bin
    TS_ASSERT_EQUALS(wkspace->binIndexOf(2.001), 1);
    // Last bin
    TS_ASSERT_EQUALS(wkspace->binIndexOf(3.1), 2);
    // Last value
    TS_ASSERT_EQUALS(wkspace->binIndexOf(4.0), 2);

    // Error handling

    // Bad index value
    TS_ASSERT_THROWS(wkspace->binIndexOf(2.5, 1), std::out_of_range);
    TS_ASSERT_THROWS(wkspace->binIndexOf(2.5, -1), std::out_of_range);

    // Bad X values
    TS_ASSERT_THROWS(wkspace->binIndexOf(5.), std::out_of_range);
    TS_ASSERT_THROWS(wkspace->binIndexOf(0.), std::out_of_range);
  }

  void testMappingFunctions()
  {
    MatrixWorkspace * wsm = new Mantid::DataObjects::WorkspaceTesterWithMaps();
    //WS index = 0 to 9
    //Spectrum = WS + 20
    //Detector ID = WS + 100
    wsm->initialize(10, 4, 2);

    {
      index2spec_map * m = wsm->getWorkspaceIndexToSpectrumMap();
      for (int i=0; i < 10; i++)
        TS_ASSERT_EQUALS((*m)[i], 20+i);
      delete m;
    }
    {
      spec2index_map * m = wsm->getSpectrumToWorkspaceIndexMap();
      for (int i=0; i < 10; i++)
        TS_ASSERT_EQUALS((*m)[i+20], i);
      delete m;
    }
    {
      index2detid_map * m = wsm->getWorkspaceIndexToDetectorIDMap();
      for (int i=0; i < 10; i++)
        TS_ASSERT_EQUALS((*m)[i], i+100);
      delete m;
    }
    {
      detid2index_map * m = wsm->getDetectorIDToWorkspaceIndexMap(true);
      for (int i=0; i < 10; i++)
        TS_ASSERT_EQUALS((*m)[i+100], i);
      delete m;
    }
  }

  void testGetNonIntegratedDimensionsThrows()
  {
    //No implementation yet. 
    MatrixWorkspace *ws = new Mantid::DataObjects::WorkspaceTester;
    TSM_ASSERT_THROWS("Characterisation tests fail", ws->getNonIntegratedDimensions(), std::runtime_error);
  }

private:
  boost::shared_ptr<MatrixWorkspace> ws;

};

#endif /*WORKSPACETEST_H_*/
