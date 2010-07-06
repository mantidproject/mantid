#ifndef WORKSPACETEST_H_
#define WORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectraAxis.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid { namespace DataObjects {
class WorkspaceTester : public MatrixWorkspace
{
public:
  WorkspaceTester() : MatrixWorkspace() {}
  virtual ~WorkspaceTester() {}

  // Empty overrides of virtual methods
  virtual const int getNumberHistograms() const { return 1;}
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
  MantidVec& dataX(int const index) {return vec;}
  MantidVec& dataY(int const index) {return vec;}
  MantidVec& dataE(int const index) {return vec;}
  const MantidVec& dataX(int const index) const {return vec;}
  const MantidVec& dataY(int const index) const {return vec;}
  const MantidVec& dataE(int const index) const {return vec;}
  Kernel::cow_ptr<MantidVec> refX(const int index) const {return Kernel::cow_ptr<MantidVec>();}
  void setX(const int index, const Kernel::cow_ptr<MantidVec>& X) {}

private:
  MantidVec vec;
  int spec;
};
}} // namespace

DECLARE_WORKSPACE(WorkspaceTester)

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
    TS_ASSERT_THROWS_NOTHING( const WorkspaceHistory& hh = wsc.getHistory() );
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
    TS_ASSERT_EQUALS( ws->YUnit(), "" )
    TS_ASSERT_THROWS_NOTHING( ws->setYUnit("something") )
    TS_ASSERT_EQUALS( ws->YUnit(), "something" )
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
  
private:
  boost::shared_ptr<MatrixWorkspace> ws;

};

#endif /*WORKSPACETEST_H_*/
