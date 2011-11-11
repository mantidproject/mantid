#ifndef MANTID_MD_CONVERT2_Q_NDANY_TEST_H_
#define MANTID_MD_CONVERT2_Q_NDANY_TEST_H_

#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDAlgorithms/ConvertToQNDany.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::MDEvents;
class Convert2AnyTestHelper: public ConvertToQNDany
{
public:
    Convert2AnyTestHelper(){};
   
    std::vector<std::string> get_dimension_names(MatrixWorkspace_const_sptr inMatrixWS){      
       std::vector<std::string> default_properties(1);
        default_properties[0]="DeltaE";
       return ConvertToQNDany::get_dimension_names(default_properties,inMatrixWS);
    }

   ConvertToQNDany::known_algorithms identify_requested_alg(const std::vector<std::string> &dim_names_availible, const std::string &QOption,const std::vector<std::string> &dim_selected,size_t &nDims)const
   { return   ConvertToQNDany::identify_requested_alg( dim_names_availible,QOption,dim_selected,nDims);
   }

   void run_algo(size_t n_mentod){       
       alg_selector[n_mentod](this);
      
   }
};
// helper function to provide list of names to test:
std::vector<std::string> dim_availible()
{
    std::string dns_ws[]={"DeltaE","T","alpha","beta","gamma"};
    std::vector<std::string> data_names_in_WS;
    for(size_t i=0;i<5;i++){
        data_names_in_WS.push_back(dns_ws[i]);
    }
    return data_names_in_WS;
}
//
class ConvertToQNDanyTest : public CxxTest::TestSuite
{
 std::auto_ptr<Convert2AnyTestHelper> pAlg;
public:
static ConvertToQNDanyTest *createSuite() { return new ConvertToQNDanyTest(); }
static void destroySuite(ConvertToQNDanyTest * suite) { delete suite; }    

void testInit(){

    TS_ASSERT_THROWS_NOTHING( pAlg->initialize() )
    TS_ASSERT( pAlg->isInitialized() )

    TSM_ASSERT_EQUALS("algortithm should have 6 propeties",6,(size_t)(pAlg->getProperties().size()));
}
void testGetDimNames(){
    // get ws from the DS    
    Mantid::API::MatrixWorkspace_sptr ws2D = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("testWSProcessed"));
    // check the privat function
    std::vector<std::string> dim_names = pAlg->get_dimension_names(ws2D);

   TSM_ASSERT_EQUALS("the algorithm for this workpace can choose from 4 properties",4,dim_names.size());

   std::vector<std::string> basic_properties(4);
   basic_properties[0]="DeltaE";
   basic_properties[1]="phi";
   basic_properties[2]="chi";
   basic_properties[3]="omega";
   for(size_t i=0;i<basic_properties.size();i++){
        TSM_ASSERT_EQUALS("The workspace property have to be specific",basic_properties[i],dim_names[i]);
   }
}

void testSetUpThrow()
{
    //TODO: check if wrong WS throws (should on validator)

     // get ws from the DS    
     Mantid::API::MatrixWorkspace_sptr ws2D = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("testWSProcessed"));
     // give it to algorithm
    TSM_ASSERT_THROWS_NOTHING("the inital ws is not in the units of energy transfer",pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    // target ws fine
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransferND"));
    // unknown Q-dimension trows
    TS_ASSERT_THROWS(pAlg->setPropertyValue("QDimensions", "unknownQ"),std::invalid_argument);
    // correct Q-dimension fine
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions", "|Q|"));
    // additional dimensions requested -- fine
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OtherDimensions", "DeltaE,omega"));

}
void testAlgoSelectorThrows()
{
    std::vector<std::string> data_names_in_WS = dim_availible();

    std::vector<std::string> dim_requested(1);
    dim_requested[0]="AA";
    size_t nDims;
    TSM_ASSERT_THROWS("AA property is unavalible among ws parameters ",pAlg->identify_requested_alg(data_names_in_WS,"|Q|",dim_requested,nDims),std::invalid_argument);
    
    dim_requested[0]="DeltaE";
    TSM_ASSERT_THROWS("Invalid Q argument ",pAlg->identify_requested_alg(data_names_in_WS,"wrong",dim_requested,nDims),std::invalid_argument);
}
     
void testAlgoSelector0()
{
    std::vector<std::string> data_names_in_WS = dim_availible();
    std::vector<std::string> dim_requested(1);
    size_t nDims;

    dim_requested[0]="DeltaE";
    TSM_ASSERT_EQUALS("NoQND: ",0,(int)pAlg->identify_requested_alg(data_names_in_WS,"",dim_requested,nDims));
    TS_ASSERT_EQUALS(1,nDims);
}
void testAlgoSelector1()
{
    std::vector<std::string> data_names_in_WS = dim_availible();
    std::vector<std::string> dim_requested(1);
    size_t nDims;

    dim_requested[0]="DeltaE";
    TSM_ASSERT_EQUALS("modQdE: ",1,(int)pAlg->identify_requested_alg(data_names_in_WS,"|Q|",dim_requested,nDims));
    TS_ASSERT_EQUALS(2,nDims);
}
void testAlgoSelector2()
{
    std::vector<std::string> data_names_in_WS = dim_availible();
    std::vector<std::string> dim_requested(2);
    size_t nDims;

    dim_requested[0]="alpha";
    dim_requested[1]="beta";
    TSM_ASSERT_EQUALS("modQND: ",2,(int)pAlg->identify_requested_alg(data_names_in_WS,"|Q|",dim_requested,nDims));
    TS_ASSERT_EQUALS(3,nDims);
}
void testAlgoSelector3()
{
    std::vector<std::string> data_names_in_WS = dim_availible();
    std::vector<std::string> dim_requested(3);
    size_t nDims;

    dim_requested[0]="alpha";
    dim_requested[1]="beta";
    dim_requested[2]="DeltaE";
    TSM_ASSERT_EQUALS("modQdEND: ",3,(int)pAlg->identify_requested_alg(data_names_in_WS,"|Q|",dim_requested,nDims));
    TS_ASSERT_EQUALS(4,nDims);
}
void testAlgoSelector4()
{
    std::vector<std::string> data_names_in_WS = dim_availible();
    std::vector<std::string> dim_requested;
    size_t nDims;

    TSM_ASSERT_EQUALS("Q3D: ",4,(int)pAlg->identify_requested_alg(data_names_in_WS,"QxQyQz",dim_requested,nDims));
    TS_ASSERT_EQUALS(3,nDims);
  
}
void testAlgoSelector5()
{
    std::vector<std::string> data_names_in_WS = dim_availible();
    std::vector<std::string> dim_requested(1);
    size_t nDims;
    dim_requested[0]="DeltaE";
    TSM_ASSERT_EQUALS("Q3DdE: ",5,(int)pAlg->identify_requested_alg(data_names_in_WS,"QxQyQz",dim_requested,nDims));
    TS_ASSERT_EQUALS(4,nDims);
  
}
void testAlgoSelector6()
{
    std::vector<std::string> data_names_in_WS = dim_availible();
    std::vector<std::string> dim_requested(2);
    size_t nDims;

    dim_requested[0]="alpha";
    dim_requested[1]="beta";
    TSM_ASSERT_EQUALS("Q3DND: ",6,(int)pAlg->identify_requested_alg(data_names_in_WS,"QxQyQz",dim_requested,nDims));
    TS_ASSERT_EQUALS(5,nDims);  
}
void testAlgoSelector7()
{
    std::vector<std::string> data_names_in_WS = dim_availible();
    std::vector<std::string> dim_requested(3);
    size_t nDims;

    dim_requested[0]="alpha";
    dim_requested[1]="beta";
    dim_requested[2]="DeltaE";
    TSM_ASSERT_EQUALS("Q3DdEND: ",7,(int)pAlg->identify_requested_alg(data_names_in_WS,"QxQyQz",dim_requested,nDims));
    TS_ASSERT_EQUALS(6,nDims);
}


void testFuncSelector()
{
    for(size_t i=0;i<8;i++){
        TSM_ASSERT_THROWS("f:"+boost::lexical_cast<std::string>(i),pAlg->run_algo(i),Kernel::Exception::NotImplementedError);
    }
}

void testExecSelection()
{
    
    pAlg->setRethrows(true);
    TS_ASSERT_THROWS(pAlg->execute(),Kernel::Exception::NotImplementedError);
}



ConvertToQNDanyTest(){
     pAlg = std::auto_ptr<Convert2AnyTestHelper>(new Convert2AnyTestHelper());
     Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);
     AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);
}

};


#endif /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_ */

