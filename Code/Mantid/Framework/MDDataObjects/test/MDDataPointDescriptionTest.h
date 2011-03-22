#ifndef MDDATA_POINT_DESCRIPTION_TEST_H
#define MDDATA_POINT_DESCRIPTION_TEST_H
#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MDDataPoint.h"

#include <memory>

using namespace Mantid;
using namespace MDDataObjects;

class MDDataPointDescriptionTest : public CxxTest::TestSuite
{

public:
    void testMDDPointDescrConstructorDefault(){
        std::auto_ptr<MDPointDescription> pDescr;
        MDPointStructure default1;
        TSM_ASSERT_THROWS_NOTHING("Nothing should be throwing in description",pDescr = std::auto_ptr<MDPointDescription>(new MDPointDescription(default1)));

        TSM_ASSERT_EQUALS("Default pixel size has (4Dxfloat,2xdouble, 3x16-bit indexes )to be equal to 38 ",38,pDescr->sizeofMDDPoint());

        char buf[] = {'a','b','c'};
        typedef  MDDataPoint<> mdp;

        std::auto_ptr<mdp > pDP;
        TSM_ASSERT_THROWS_NOTHING("Default MDData Point should not throw with default descpr",pDP = std::auto_ptr<mdp>(new mdp(buf,*pDescr)));

        TSM_ASSERT_EQUALS("Actual and described pixel length have to be the same",pDescr->sizeofMDDPoint(),pDP->sizeofMDDataPoint());
    }
   void test1DMDDPointThrows(){
      // number of dimensions is lower then the number of reciprocal dimensions
        MDPointStructure info;
        info.NumDimensions = 1;
        info.NumDimIDs     = 1;
        TSM_ASSERT_THROWS("Nothing should be throwing in description",new MDPointDescription(info),std::invalid_argument);

    }
    void test1DMDDPointDesct(){
        std::auto_ptr<MDPointDescription> pDescr;
        MDPointStructure info ;
        info.NumDimensions    = 1;
        info.NumRecDimensions = 1;
        info.NumDimIDs        = 1;
        TSM_ASSERT_THROWS_NOTHING("Nothing should be throwing in description",pDescr = std::auto_ptr<MDPointDescription>(new MDPointDescription(info)));
	
        TSM_ASSERT_EQUALS("Default 1D pixel size has to be (1Dxfloat,2xdouble, 1x16-bit indexes )to be equal to 22 ",22,pDescr->sizeofMDDPoint());

        char buf[] = {'a','b','c'};
        typedef  MDDataPoint<> mdp;

        std::auto_ptr<mdp > pDP;
        TSM_ASSERT_THROWS_NOTHING("Default MDData Point should not throw with default descpr",pDP = std::auto_ptr<mdp >(new mdp(buf,*pDescr)));
	// but singleDimID is always casted to at least 32 bit integer (or bigger if specified)
        TSM_ASSERT_EQUALS("Actual and described pixel length have to be the same",pDescr->sizeofMDDPoint()+2,pDP->sizeofMDDataPoint());
    }
   void testMDEqualPointDesct(){
        std::auto_ptr<MDPointDescription> pDescr;
        MDPointStructure info ;
        info.SignalLength = 4;
        info.DimIDlength  = 4;
        info.NumPixCompressionBits=0;
        TSM_ASSERT_THROWS_NOTHING("Nothing should be throwing in description",pDescr = std::auto_ptr<MDPointDescription>(new MDPointDescription(info)));

        TSM_ASSERT_EQUALS("Default 1D pixel size has to be (4Dxfloat,2xfloat, 3xint-bit indexes )to be equal to 36 ",36,pDescr->sizeofMDDPoint());


        char buf[] = {'a','b','c'};
        typedef MDDataPointEqual<float,uint32_t,float> mdpe;
        std::auto_ptr<mdpe > pDP;
        TSM_ASSERT_THROWS_NOTHING("Equal MDData Point should not throw when build with such descption", pDP = (std::auto_ptr<mdpe >)(new mdpe(buf,*pDescr)));

        TSM_ASSERT_EQUALS("Actual and described pixel length have to be the same",pDescr->sizeofMDDPoint(),pDP->sizeofMDDataPoint());
    }
};
#endif
