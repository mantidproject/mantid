#ifndef CONVERT2_MDEVENTS_UNITS_CONVERSION_TEST_H_
#define CONVERT2_MDEVENTS_UNITS_CONVERSION_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDAlgorithms/ConvertToMDEventsUnitsConv.h"

class ConvertToMDEventsUnitsConvTest : public CxxTest::TestSuite
{

public:
static ConvertToMDEventsUnitsConvTest *createSuite() { return new ConvertToMDEventsUnitsConvTest(); }
static void destroySuite(ConvertToMDEventsUnitsConvTest  * suite) { delete suite; }    




ConvertToMDEventsUnitsConvTest (){

 //  int numHist=10;
 //  Mantid::API::MatrixWorkspace_sptr wsEv = boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::CreateRandomEventWorkspace(100, numHist, 0.1));
 //  wsEv->setInstrument( ComponentCreationHelper::createTestInstrumentCylindrical(numHist) );
 //  // any inelastic units or unit conversion using TOF needs Ei to be present among properties. 
 ////  wsEv->mutableRun().addProperty("Ei",13.,"meV",true);

}

#endif
