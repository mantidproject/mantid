#include <iostream>
#include <iomanip>
#include "Benchmark.h"
#include "UserAlgorithmTest.h"
#include "MantidAPI/FrameworkManager.h"
//#include "MantidAPI/Workspace.h"
//#include "MantidDataObjects/Workspace1D.h" 
//#include "MantidDataObjects/Workspace2D.h" 

#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/TableRow.h"
#include "MantidDataObjects/ColumnFactory.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Instrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Component.h"
#include "MantidGeometry/ParametrizedComponent.h"
#include "MantidGeometry/ParObjComponent.h"
#include "MantidGeometry/ParameterMap.h"

#include <boost/timer.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

void test();

int main()
{

  FrameworkManagerImpl& fm = FrameworkManager::Instance();

//  UserAlgorithmTest userTest;
 // userTest.RunAllTests();
  
 // Benchmark b;
 // b.RunPlusTest();
/*
#if defined _DEBUG
  //NOTE:  Any code in here is temporary for debugging purposes only, nothing is safe!
  //load a raw file
    IAlgorithm* loader = fm.createAlgorithm("LoadRaw");
    loader->setPropertyValue("Filename", "../../../Test/Data/HET15869.RAW");

    std::string outputSpace = "outer";
    loader->setPropertyValue("OutputWorkspace", outputSpace);    

    loader->execute();

    IAlgorithm* focus = fm.createAlgorithm("DiffractionFocussing");
    focus->setPropertyValue("GroupingFileName", "../../../Test/Data/offsets_2006_cycle064.cal");

    std::string resultSpace = "result";
    focus->setPropertyValue("InputWorkspace", outputSpace); 
    focus->setPropertyValue("OutputWorkspace", resultSpace);    

    focus->execute();


#endif


  FrameworkManager::Instance().clear();
  exit(0);//*/


   test();

}

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
  virtual const int version()const { return (1);}
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
    //std::cerr<<"OK "<<t->rowCount()<<"\n";
    TableRow r = t->getFirstRow();
    do
    {
        std::cerr<<r<<'\n';
    }while(r.next());
}

void test()
{
    boost::shared_ptr<TableWorkspace> t(new TableWorkspace(10));
    t->createColumn("str","Name");
    t->createColumn("int","Nunber");
    TableRow row = t->getFirstRow();
    row<<"Abracadabra"<<13;
    AnalysisDataService::Instance().add("tst",boost::dynamic_pointer_cast<Workspace>(t));
    TableWorkspaceAlgorithm alg;
    alg.initialize();
    alg.setPropertyValue("Table","tst");
    alg.execute();

    ParameterDouble pd("d");
    pd = 12.;
    Parameter* p = &pd;
    try
    {
    std::cerr<<"p="<<p->value<double>()<<'\n';
    }
    catch(std::exception& e)
    {
        std::cerr<<e.what()<<'\n';
    }
}