#ifndef WORKSPACEHISTORYTEST_H_
#define WORKSPACEHISTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Property.h"
#include "MantidAPI/WorkspaceProperty.h"


using namespace Mantid::DataObjects;
using namespace Mantid::Kernel; 
using namespace Mantid::API;

class fill2d : public Algorithm
{
public:
  fill2d() : Algorithm() {}
  virtual ~fill2d() {}
  const std::string name() const { return "fill2d";} 
  const int version() const { return 1;} 

  void init()
  { 
    declareProperty("signal",5.0);
    declareProperty("error",3.0);
    declareProperty(new WorkspaceProperty<Workspace2D>("OutWS","",Direction::Output));
  }
  void exec() 
  {
    int xlen = 100;
    int ylen = 100;
    double signal = getProperty("signal"); 
    double error = getProperty("error");

    std::vector<double> x1(xlen,1), y1(xlen,signal), e1(xlen,error);
    Workspace2D_sptr outWS(new Workspace2D);
    outWS->init(ylen,xlen,xlen);
    for (int i=0; i< ylen; i++)
    {
      outWS->setX(i,x1);     
      outWS->setData(i,y1,e1);
    }
    setProperty("OutWS", outWS );
  }
};

class add2d : public Algorithm
{
public:
  add2d() : Algorithm() {}
  virtual ~add2d() {}
  const std::string name() const { return "add2d";} 
  const int version() const { return 1;} 

  void init()
  { 
    declareProperty(new WorkspaceProperty<Workspace>("InWS_1","",Direction::Input));
    declareProperty(new WorkspaceProperty<Workspace>("InWS_2","",Direction::Input));
    declareProperty(new WorkspaceProperty<Workspace>("InoutWS","",Direction::InOut));
  }
  void exec() 
  {
    Workspace_sptr in_work1 = getProperty("InWS_1");
    Workspace_sptr in_work2 = getProperty("InWS_2");
    Workspace_sptr out = getProperty("InoutWS");


    for (int i=0;i<100;i++)
    {
      std::vector<double>& io1_Y = in_work1->dataY(i);
      std::vector<double>& io2_Y = in_work2->dataY(i);
      std::vector<double>& out_Y = out->dataY(i);
      std::vector<double>& io1_e = in_work1->dataE(i);
      std::vector<double>& io2_e = in_work2->dataE(i);
      std::vector<double>& out_e = out->dataE(i);

      for(int j=0; j < io1_Y.size(); j++)
      {
        out_Y[j]=io1_Y[j]+ io2_Y[j];
        out_e[j]= std::sqrt((io1_e[j]*io1_e[j])+(io2_e[j]* io2_e[j]));
        //don't need to touch the xdata
      }
    }
    //setProperty("InoutWS", out );
    // don't need to setProperty() as it already existed and
    // is an inout and we have changed it in situ, it WILL be stored
  }
};


/*
this test has to reside in the DataObjects project since even though Workspace History
is a property of the Workspace base class, you cannot have an instantiation of a 
properly filled Workspace objhect and a history can only really properly be created 
for a Workspace1D or Workspace2D, at the moment...
*/


class AlgorithmTest : public CxxTest::TestSuite
{
public: 

  void testExecute()
  {

    fill2d myAlg1,myAlg2, myAlg3;
    add2d  manip;
    // create workspace to hold manipulate
    myAlg1.initialize();
    myAlg1.setPropertyValue("OutWS","A");
    myAlg1.execute();

    Workspace_sptr A = AnalysisDataService::Instance().retrieve("A");
    TS_ASSERT_THROWS_NOTHING(WorkspaceHistory& A_WH = A->getWorkspaceHistory());
    WorkspaceHistory& A_WH = A->getWorkspaceHistory();
    TS_ASSERT_THROWS_NOTHING(const std::vector<AlgorithmHistory>& A_AH = A_WH.getAlgorithms());
    const std::vector<AlgorithmHistory>& A_AH = A_WH.getAlgorithms();
    TS_ASSERT_EQUALS( A_AH.size(), 1);
    TS_ASSERT_EQUALS( "fill2d", A_AH[0].name());
    TS_ASSERT_EQUALS( 1, A_AH[0].version());
    TS_ASSERT_THROWS_NOTHING(const std::vector<AlgorithmParameter>& A_AP = A_AH[0].getParameters());    
    const std::vector<AlgorithmParameter>& A_AP = A_AH[0].getParameters();
    TS_ASSERT_EQUALS(A_AP.size(), 3);
    TS_ASSERT_EQUALS(A_AP[0].name(),"signal");
    TS_ASSERT_EQUALS(A_AP[0].value(),"5");
    TS_ASSERT_EQUALS(A_AP[0].isDefault(),true);
    TS_ASSERT_EQUALS(A_AP[0].direction(),3);

    TS_ASSERT_EQUALS(A_AP[1].name(),"error");
    TS_ASSERT_EQUALS(A_AP[1].value(),"3");
    TS_ASSERT_EQUALS(A_AP[1].isDefault(),true);
    TS_ASSERT_EQUALS(A_AP[1].direction(),3);

    TS_ASSERT_EQUALS(A_AP[2].name(),"OutWS");
    TS_ASSERT_EQUALS(A_AP[2].value(),"A");
    TS_ASSERT_EQUALS(A_AP[2].isDefault(),false);
    TS_ASSERT_EQUALS(A_AP[2].direction(),1);

    // create workspace to hold manipulate
    myAlg2.initialize();
    myAlg2.setPropertyValue("OutWS","B");
    myAlg2.setProperty("signal",32.0);
    myAlg2.setProperty("error",4.0);
    myAlg2.execute();

    Workspace_sptr B = AnalysisDataService::Instance().retrieve("B");
    TS_ASSERT_THROWS_NOTHING(WorkspaceHistory& B_WH = B->getWorkspaceHistory());
    WorkspaceHistory& B_WH = B->getWorkspaceHistory();
    TS_ASSERT_THROWS_NOTHING(const std::vector<AlgorithmHistory>& B_AH = B_WH.getAlgorithms());
    const std::vector<AlgorithmHistory>& B_AH = B_WH.getAlgorithms();
    TS_ASSERT_EQUALS( B_AH.size(), 1);
    TS_ASSERT_EQUALS( "fill2d", B_AH[0].name());
    TS_ASSERT_EQUALS( 1, B_AH[0].version());
    TS_ASSERT_THROWS_NOTHING(const std::vector<AlgorithmParameter>& B_AP = B_AH[0].getParameters());
    const std::vector<AlgorithmParameter>& B_AP = B_AH[0].getParameters();
    TS_ASSERT_EQUALS(B_AP.size(), 3);
    TS_ASSERT_EQUALS(B_AP[0].name(),"signal");
    TS_ASSERT_EQUALS(B_AP[0].value(),"32");
    TS_ASSERT_EQUALS(B_AP[0].direction(),3);
    TS_ASSERT_EQUALS(B_AP[0].isDefault(),false);
    
    TS_ASSERT_EQUALS(B_AP[1].name(),"error");
    TS_ASSERT_EQUALS(B_AP[1].value(),"4");
    TS_ASSERT_EQUALS(B_AP[1].isDefault(),false);
    TS_ASSERT_EQUALS(B_AP[1].direction(),3);
    
    TS_ASSERT_EQUALS(B_AP[2].name(),"OutWS");
    TS_ASSERT_EQUALS(B_AP[2].value(),"B");
    TS_ASSERT_EQUALS(B_AP[2].isDefault(),false);
    TS_ASSERT_EQUALS(B_AP[2].direction(),1);
    
    // create workspace to hold the result in
    myAlg3.initialize();
    myAlg3.setPropertyValue("OutWS","C");
    myAlg3.setProperty("signal",0.0);
    myAlg3.setProperty("error",0.0);
    myAlg3.execute();

    // do the manipulation, the result being held in the inoutWS
    manip.initialize();

    manip.setPropertyValue("InWS_1","A");
    manip.setPropertyValue("InWS_2","B");
    manip.setPropertyValue("InoutWS","C");    
    manip.execute();
    Workspace_sptr C = AnalysisDataService::Instance().retrieve("C");
    TS_ASSERT_THROWS_NOTHING(WorkspaceHistory& C_WH = C->getWorkspaceHistory());
    WorkspaceHistory& C_WH = C->getWorkspaceHistory();
    TS_ASSERT_THROWS_NOTHING(std::vector<AlgorithmHistory>& C_AH = C_WH.getAlgorithms());
    std::vector<AlgorithmHistory>& C_AH = C_WH.getAlgorithms();
    TS_ASSERT_EQUALS( C_AH.size(), 4);
    TS_ASSERT_EQUALS( "add2d", C_AH[3].name());
    TS_ASSERT_EQUALS( 1, C_AH[3].version());
    TS_ASSERT_THROWS_NOTHING(const std::vector<AlgorithmParameter>& C_AP = C_AH[0].getParameters());
    const std::vector<AlgorithmParameter>& C_AP = C_AH[3].getParameters();
    TS_ASSERT_EQUALS(C_AP.size(), 3);
    // isdefault is true for all parameters because a setProperty has not been called on them
    TS_ASSERT_EQUALS(C_AP[0].name(),"InWS_1");
    TS_ASSERT_EQUALS(C_AP[0].value(),"A");
    TS_ASSERT_EQUALS(C_AP[0].isDefault(),true);
    TS_ASSERT_EQUALS(C_AP[0].direction(),0);
    TS_ASSERT_EQUALS(C_AP[1].name(),"InWS_2");
    TS_ASSERT_EQUALS(C_AP[1].value(),"B");
    TS_ASSERT_EQUALS(C_AP[1].isDefault(),true);
    TS_ASSERT_EQUALS(C_AP[1].direction(),0);
    TS_ASSERT_EQUALS(C_AP[2].name(),"InoutWS");
    TS_ASSERT_EQUALS(C_AP[2].value(),"C");
    TS_ASSERT_EQUALS(C_AP[2].isDefault(),true);
    TS_ASSERT_EQUALS(C_AP[2].direction(),2);
    
    // Test streamed output.
    std::stringstream s;
    s.exceptions( std::ios::failbit | std::ios::badbit );
    TS_ASSERT_THROWS_NOTHING( s << C_WH )
    // Check size (in bytes) of output
    int i = s.tellp();
    TS_ASSERT_LESS_THAN( 1800, i )
    // Check first line
    s.seekp(0);
    char c[21];
    s.getline(c,21);
    std::string ss(c);
    TS_ASSERT( ! ss.compare("Framework Version : ") )
  }
};



#endif /*WORKSPACEHISTORYTEST_H_*/
