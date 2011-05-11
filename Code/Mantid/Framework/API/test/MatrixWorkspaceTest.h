#ifndef WORKSPACETEST_H_
#define WORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidGeometry/Instrument/Instrument.h"


using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

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
  virtual int getNumberHistograms() const { return 1;}
  const std::string id() const {return "WorkspaceTester";}
  void init(const int& i, const int& j, const int& k)
  {
    vec.resize(j,1.0);
    // Put an 'empty' axis in to test the getAxis method
    m_axes.resize(1);
    m_axes[0] = new NumericAxis(1);
  }
  int size() const {return vec.size();}
  int blocksize() const {return vec.size();}
  MantidVec& dataX(int const ) {return vec;}
  MantidVec& dataY(int const ) {return vec;}
  MantidVec& dataE(int const ) {return vec;}
  MantidVec& dataDx(int const ) {return vec;}
  const MantidVec& dataX(int const) const {return vec;}
  const MantidVec& dataY(int const) const {return vec;}
  const MantidVec& dataE(int const) const {return vec;}
  const MantidVec& dataDx(int const) const {return vec;}
  Kernel::cow_ptr<MantidVec> refX(const int) const {return Kernel::cow_ptr<MantidVec>();}
  void setX(const int, const Kernel::cow_ptr<MantidVec>&) {}

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
  virtual int getNumberHistograms() const { return m_NVectors;}
  const std::string id() const {return "WorkspaceTester";}
  void init(const int& NVectors, const int& j, const int& k)
  {
    m_NVectors = NVectors;
    vec.resize(j,1.0);
    // Put an 'empty' axis in to test the getAxis method
    m_axes.resize(2);
    m_axes[0] = new NumericAxis(1);

    //Spectrum # = 20 + workspace index.
    SpectraAxis * ax =  new SpectraAxis(m_NVectors);
    for (int i=0; i<m_NVectors; i++)
      ax->setValue(i, i+20);
    m_axes[1] = ax;

    //Detector id is 100 + workspace index = 80 + spectrum #
    for (int i=20; i<20+m_NVectors; i++)
    {
      std::vector<int> vec;
      vec.push_back(i+80);
      this->mutableSpectraMap().addSpectrumEntries(i, vec);
    }

  }
  int size() const {return vec.size();}
  int blocksize() const {return vec.size();}
  MantidVec& dataX(int const ) {return vec;}
  MantidVec& dataY(int const ) {return vec;}
  MantidVec& dataE(int const ) {return vec;}
  MantidVec& dataDx(int const ) {return vec;}
  const MantidVec& dataX(int const) const {return vec;}
  const MantidVec& dataY(int const) const {return vec;}
  const MantidVec& dataE(int const) const {return vec;}
  const MantidVec& dataDx(int const) const {return vec;}
  Kernel::cow_ptr<MantidVec> refX(const int) const {return Kernel::cow_ptr<MantidVec>();}
  void setX(const int, const Kernel::cow_ptr<MantidVec>&) {}

private:
  MantidVec vec;
  int spec;
  int m_NVectors;
};

DECLARE_WORKSPACE(WorkspaceTester)

class WorkspaceTesterWithDetectors : public MatrixWorkspace
{
public:
  WorkspaceTesterWithDetectors() : MatrixWorkspace() {}
  virtual ~WorkspaceTesterWithDetectors() {}

  // Empty overrides of virtual methods
  virtual int getNumberHistograms() const { return m_NVectors;}
  const std::string id() const {return "WorkspaceTester";}
  void init(const int& NVectors, const int& j, const int& k)
  {
    m_NVectors = NVectors;
    vec.resize(NVectors, MantidVec(1, 1.0));
    // Put an 'empty' axis in to test the getAxis method
    m_axes.resize(2);
    m_axes[0] = new NumericAxis(1);

    m_axes[1] =  new SpectraAxis(m_NVectors);
    for (int i=0; i<m_NVectors; i++)
    {
      this->getAxis(1)->spectraNo(i) = i;
    }
    this->mutableSpectraMap().populateSimple(0, NVectors);

    setInstrument(Instrument_sptr(new Instrument("TestInstrument")));
    Instrument_sptr inst = getBaseInstrument();
    
    for( int i = 0; i < NVectors; ++i )
    {
      // Create a detector for each spectra
      Detector * det = new Detector("pixel", i, inst.get());
      inst->add(det);
      inst->markAsDetector(det);
    }

  }
  int size() const {return vec.size();}
  int blocksize() const {return vec[0].size();}
  MantidVec& dataX(int const i) {return vec[i];}
  MantidVec& dataY(int const i) {return vec[i];}
  MantidVec& dataE(int const i) {return vec[i];}
  MantidVec& dataDx(int const i) {return vec[i];}
  const MantidVec& dataX(int const i) const {return vec[i];}
  const MantidVec& dataY(int const i) const {return vec[i];}
  const MantidVec& dataE(int const i) const {return vec[i];}
  const MantidVec& dataDx(int const i) const {return vec[i];}
  Kernel::cow_ptr<MantidVec> refX(const int) const {return Kernel::cow_ptr<MantidVec>();}
  void setX(const int, const Kernel::cow_ptr<MantidVec>&) {}

private:
  std::vector<MantidVec> vec;
  int spec;
  int m_NVectors;
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

  void testSpectraMap()
  {
    MatrixWorkspace_sptr ws2 = WorkspaceFactory::Instance().create(ws,1,1,1);
    const SpectraDetectorMap &specs = ws2->spectraMap();
    TS_ASSERT_EQUALS( &(ws->spectraMap()), &specs );
    SpectraDetectorMap &specs2 = ws2->mutableSpectraMap();
    TS_ASSERT_DIFFERS( &(ws->spectraMap()), &specs2 );
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
    TS_ASSERT_THROWS_NOTHING( WorkspaceHistory& h = ws->history() );
    const Mantid::DataObjects::WorkspaceTester wsc;
    const WorkspaceHistory& hh = wsc.getHistory();
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
    IndexToIndexMap * m;

    m = wsm->getWorkspaceIndexToSpectrumMap();
    for (int i=0; i < 10; i++)
      TS_ASSERT_EQUALS((*m)[i], 20+i);
    delete m;

    m = wsm->getSpectrumToWorkspaceIndexMap();
    for (int i=0; i < 10; i++)
      TS_ASSERT_EQUALS((*m)[i+20], i);
    delete m;

    m = wsm->getWorkspaceIndexToDetectorIDMap();
    for (int i=0; i < 10; i++)
      TS_ASSERT_EQUALS((*m)[i], i+100);
    delete m;

    m = wsm->getDetectorIDToWorkspaceIndexMap(true);
    for (int i=0; i < 10; i++)
      TS_ASSERT_EQUALS((*m)[i+100], i);
    delete m;

  }



private:
  boost::shared_ptr<MatrixWorkspace> ws;

};

#endif /*WORKSPACETEST_H_*/
