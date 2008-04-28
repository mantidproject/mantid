#ifndef WORKSPACETEST_H_
#define WORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Workspace.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

class WorkspaceTester : public Workspace
{
public:
  WorkspaceTester() : Workspace() {}
  virtual ~WorkspaceTester() {}
  
  // Empty overrides of virtual methods
  virtual const int getHistogramNumber() const { return 1;}
  const std::string id() const {return "WorkspaceTester";}
  void init(const int&, const int&, const int&) {}
  int size() const {return 0;}
  int blocksize() const {return 0;}
  std::vector<double>& dataX(int const index) {return vec;}
  std::vector<double>& dataY(int const index) {return vec;}
  std::vector<double>& dataE(int const index) {return vec;}
  std::vector<double>& dataE2(int const index) {return vec;}
  const std::vector<double>& dataX(int const index) const {return vec;}
  const std::vector<double>& dataY(int const index) const {return vec;}
  const std::vector<double>& dataE(int const index) const {return vec;}
  const std::vector<double>& dataE2(int const index) const {return vec;}
  const IErrorHelper* errorHelper(int const index) const {return NULL;}
  void setErrorHelper(int const,IErrorHelper*) {}
  void setErrorHelper(int const,const IErrorHelper*) {}
  int spectraNo(int const) const {return spec;}
  int& spectraNo(int const) {return spec;}
  
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

	void testGetSample()
	{
	  Sample& s = ws.getSample();
		ws.getSample().setName("test");
		TS_ASSERT_EQUALS( ws.getSample().getName(), "test" )
	}

	void testGetMemorySize()
	{
		TS_ASSERT_EQUALS( ws.getMemorySize(), 0 )
	}

	void testGetWorkspaceHistory()
	{
	  TS_ASSERT_THROWS_NOTHING( WorkspaceHistory& h = ws.getWorkspaceHistory() )
	  const WorkspaceTester wsc;
	  const WorkspaceHistory& hh = wsc.getWorkspaceHistory();
	  TS_ASSERT_THROWS_NOTHING( ws.getWorkspaceHistory() = hh )
	}

	void test_global_Mantid_API_Workspace_XUnit()
	{
	  TS_ASSERT( ! ws.XUnit().get() )
	  boost::shared_ptr<Unit> u;
	  TS_ASSERT_THROWS_NOTHING( ws.XUnit() = u )
	}

	void test_global_Mantid_API_Workspace_isDistribution()
	{
	  TS_ASSERT( ! ws.isDistribution() )
	  TS_ASSERT( ws.isDistribution(true) )
    TS_ASSERT( ws.isDistribution() )	  
	}

private:
  WorkspaceTester ws;
	
};

#endif /*WORKSPACETEST_H_*/
