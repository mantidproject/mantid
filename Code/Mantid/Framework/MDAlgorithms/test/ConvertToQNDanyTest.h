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
};

class ConvertToQNDanyTest : public CxxTest::TestSuite
{
 std::auto_ptr<Convert2AnyTestHelper> pAlg;
public:
static ConvertToQNDanyTest *createSuite() { return new ConvertToQNDanyTest(); }
static void destroySuite(ConvertToQNDanyTest * suite) { delete suite; }    

void testInit(){

    TS_ASSERT_THROWS_NOTHING( pAlg->initialize() )
    TS_ASSERT( pAlg->isInitialized() )

//    TSM_ASSERT_EQUALS("algortithm should have 3 propeties",3,(size_t)(pAlg->getProperties().size()));
}
void testGetDimNames(){
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);

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




ConvertToQNDanyTest(){
    pAlg = std::auto_ptr<Convert2AnyTestHelper>(new Convert2AnyTestHelper());
}

};


#endif /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_ */

