#ifndef TESTTABLEWORKSPACEPROPERTY_
#define TESTTABLEWORKSPACEPROPERTY_

#include <vector> 
#include <algorithm> 
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/TableWorkspace.h" 
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Property.h"
#include "MantidAPI/TableRow.h" 
#include "MantidAPI/ColumnFactory.h" 

using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace std;

class TableWorkspaceAlgorithm : public Algorithm
{
public:
  ///no arg constructor
  TableWorkspaceAlgorithm() : Algorithm() {}
  ///virtual destructor
  virtual ~TableWorkspaceAlgorithm() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "TableWorkspaceAlgorithm";}
  /// Algorithm's version for identification overriding a virtual method
  virtual int version()const { return (1);}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Examples";}

private:
  ///Initialisation code
  void init();
  ///Execution code
  void exec();

  /// Static reference to the logger class
  static Mantid::Kernel::Logger& g_log;
};

void TableWorkspaceAlgorithm::init()
{
    declareProperty(new WorkspaceProperty<Workspace>("Table","",Direction::Input));
}

void TableWorkspaceAlgorithm::exec()
{
    Workspace_sptr b = getProperty("Table");
    TableWorkspace_sptr t = boost::dynamic_pointer_cast<TableWorkspace>(b);
    TableRow r = t->getFirstRow();
    r<<"FIRST"<<11;  r.next();
    r<<"SECOND"<<22;
}

class TableWorkspacePropertyTest : public CxxTest::TestSuite
{
public:
    TableWorkspacePropertyTest()
    {
        t.reset(new TableWorkspace(10));
        t->addColumn("str","Name");
        t->addColumn("int","Nunber");
        AnalysisDataService::Instance().add("tst",boost::dynamic_pointer_cast<Workspace>(t));
    }

    void testProperty()
    {
        TableWorkspaceAlgorithm alg;
        alg.initialize();
        alg.setPropertyValue("Table","tst");
        alg.execute();
        TableWorkspace_sptr table;
        TS_ASSERT_THROWS_NOTHING(table = boost::dynamic_pointer_cast<TableWorkspace>(AnalysisDataService::Instance().retrieve("tst")));
        TS_ASSERT( table );
        TS_ASSERT_EQUALS( table->rowCount(), 10 );
        TableRow r = table->getFirstRow();
        std::string s;
        int n;
        r >> s >> n;
        TS_ASSERT_EQUALS( s, "FIRST" );
        TS_ASSERT_EQUALS( n, 11 );
        r.next();
        r >> s >> n;
        TS_ASSERT_EQUALS( s, "SECOND" );
        TS_ASSERT_EQUALS( n, 22 );
    }
private:
    boost::shared_ptr<TableWorkspace> t;

};
#endif /*TESTTABLEWORKSPACE_*/
