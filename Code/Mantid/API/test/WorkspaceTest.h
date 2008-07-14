#ifndef WORKSPACETEST_H_
#define WORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Workspace.h"
#include "MantidAPI/SpectraDetectorMap.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

class WorkspaceTester : public Workspace
{
public:
  WorkspaceTester() : Workspace() {}
  virtual ~WorkspaceTester() {}

  // Empty overrides of virtual methods
  virtual const int getNumberHistograms() const { return 1;}
  const std::string id() const {return "WorkspaceTester";}
  void init(const int&, const int&, const int&)
  {
    // Put an 'empty' axis in to test the getAxis method
    m_axes.resize(1);
    m_axes[0] = new Axis(AxisType::Numeric,1);
  }
  int size() const {return 0;}
  int blocksize() const {return 0;}
  std::vector<double>& dataX(int const index) {return vec;}
  std::vector<double>& dataY(int const index) {return vec;}
  std::vector<double>& dataE(int const index) {return vec;}
  const std::vector<double>& dataX(int const index) const {return vec;}
  const std::vector<double>& dataY(int const index) const {return vec;}
  const std::vector<double>& dataE(int const index) const {return vec;}
  const IErrorHelper* errorHelper(int const index) const {return NULL;}
  void setErrorHelper(int const,IErrorHelper*) {}
  void setErrorHelper(int const,const IErrorHelper*) {}

private:
  std::vector<double> vec;
  int spec;
};

class WorkspaceTest : public CxxTest::TestSuite
{
public:

  void testGetSetTitle()
  {
    TS_ASSERT_EQUALS( ws.getTitle(), "" )
    ws.setTitle("something");
    TS_ASSERT_EQUALS( ws.getTitle(), "something" )
    ws.setTitle("");
  }

  void testGetSetComment()
  {
    TS_ASSERT_EQUALS( ws.getComment(), "" )
    ws.setComment("commenting");
    TS_ASSERT_EQUALS( ws.getComment(), "commenting" )
    ws.setComment("");
  }

  void testGetInstrument()
  {
    boost::shared_ptr<Instrument> i = ws.getInstrument();
    TS_ASSERT_EQUALS( ws.getInstrument()->type(), "Instrument" )
  }

  void testGetSetSpectraMap()
  {
    TS_ASSERT( ws.getSpectraMap() )
    boost::shared_ptr<SpectraDetectorMap> s(new SpectraDetectorMap);
    TS_ASSERT_THROWS_NOTHING( ws.setSpectraMap(s) )
    TS_ASSERT_EQUALS( ws.getSpectraMap(), s )
  }

  void testGetSetSample()
  {
    TS_ASSERT( ws.getSample() )
    boost::shared_ptr<Sample> s(new Sample);
    TS_ASSERT_THROWS_NOTHING( ws.setSample(s) )
    TS_ASSERT_EQUALS( ws.getSample(), s )
    ws.getSample()->setName("test");
    TS_ASSERT_EQUALS( ws.getSample()->getName(), "test" )
  }

  void testGetMemorySize()
  {
    TS_ASSERT_THROWS( ws.getMemorySize(), Exception::NotImplementedError )
  }

  void testHistory()
  {
    TS_ASSERT_THROWS_NOTHING( WorkspaceHistory& h = ws.history() )
    const WorkspaceTester wsc;
    const WorkspaceHistory& hh = wsc.getHistory();
    TS_ASSERT_THROWS_NOTHING( ws.history() = hh )
  }

  void testGetAxis()
  {
    ws.init(0,0,0);
    TS_ASSERT_THROWS( ws.getAxis(-1), Exception::IndexError )
    TS_ASSERT_THROWS_NOTHING( ws.getAxis(0) )
    TS_ASSERT( ws.getAxis(0) )
    TS_ASSERT_THROWS( ws.getAxis(1), Exception::IndexError )
  }

  void testIsDistribution()
  {
    TS_ASSERT( ! ws.isDistribution() )
    TS_ASSERT( ws.isDistribution(true) )
    TS_ASSERT( ws.isDistribution() )
  }

private:
  WorkspaceTester ws;

};

#endif /*WORKSPACETEST_H_*/
