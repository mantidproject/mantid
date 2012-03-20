#ifndef MANTID_MD_CONVERT2_MDEV_SUBALGFACTORY_TEST_H_
#define MANTID_MD_CONVERT2_MDEV_SUBALGFACTORY_TEST_H_

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

#include "MantidMDAlgorithms/ConvertToMDEventsSubalgFactory.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;


//
class ConvertToMDEventsSubalgFactoryTest : public CxxTest::TestSuite
{
 std::auto_ptr<ConvertToMDEventsSubalgFactory> pFact;
 std::auto_ptr<ConvertToMDEventsParams> pParams;
public:
static ConvertToMDEventsSubalgFactoryTest *createSuite() { return new ConvertToMDEventsSubalgFactoryTest(); }
static void destroySuite(ConvertToMDEventsSubalgFactoryTest * suite) { delete suite; }    

void testInit()
{
    TS_ASSERT_THROWS_NOTHING(pFact->init(*pParams));
}

void testWrongAlgThrows()
{
    TS_ASSERT_THROWS(pFact->getAlg("Non_existing_subalgorithm"),std::invalid_argument);
}

void testGetAlg()
{
    for(int iq=0;iq<NoQ;iq++)
    {
        Q_state q = (Q_state)iq;
        for(int im=0;im<ANY_Mode;im++){

            AnalMode m = (AnalMode)im;
            for(int ic=0;ic<NConvUintsStates;ic++){
                CnvrtUnits c= (CnvrtUnits)ic;

                for(int iw=0;iw<NInWSTypes;iw++){
                    InputWSType w =InputWSType(iw);

                    for(int is=0;is<NSampleTypes;is++){
                        SampleType s = SampleType(is);
                        std::string alg_id = pParams->getAlgoID(q,m,c,w,s);
                        TSM_ASSERT_THROWS_NOTHING("Q-type subalgorithm with id: "+alg_id+" has not been initated properly",pFact->getAlg(alg_id));
                    }
                }
            }
        }
    }

// NoQ mode is special, it has less options        
   for(int ic=0;ic<NConvUintsStates;ic++)
   {
       CnvrtUnits c= (CnvrtUnits)ic;
       for(int iw=0;iw<NInWSTypes;iw++)
       {
           InputWSType w =InputWSType(iw);
           std::string alg_id = pParams->getAlgoID(NoQ,ANY_Mode,c,w,NSampleTypes);
           TSM_ASSERT_THROWS_NOTHING("NoQ-type subalgorithm with id: "+alg_id+" has not been initated properly",pFact->getAlg(alg_id));
       }
   }
  

}

//
ConvertToMDEventsSubalgFactoryTest()
{
        pFact = std::auto_ptr<ConvertToMDEventsSubalgFactory>(new ConvertToMDEventsSubalgFactory());
        pParams=std::auto_ptr<ConvertToMDEventsParams>(new ConvertToMDEventsParams());
}

};


#endif /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_ */

