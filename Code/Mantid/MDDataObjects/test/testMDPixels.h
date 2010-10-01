#ifndef H_TEST_SQW
#define H_TEST_SQW


#include <cxxtest/TestSuite.h>
#include "MDPixels.h"

using namespace Mantid;
using namespace MDDataObjects;

class testMDPixels :    public CxxTest::TestSuite
{
public:
    void testSQW(void){
       
        try{
            MDPixels sqw_data(5);
        // read DND object
             TS_ASSERT_THROWS_NOTHING(sqw_data.read_mdd("../../../../Test/VATES/fe_demo.sqw"));
             // it can throw if the pixels do not fit into memory
             std::cout<<"\n start reading all pixels in memory and this can be long\n";
             TS_ASSERT_THROWS_NOTHING(sqw_data.read_pix());

             // are pixels in memory?
             TS_ASSERT_EQUALS(sqw_data.isMemoryBased(),true);
             

        }catch(std::exception &err){ 
            std::cout<<" error of the SQW constructor "<<err.what()<<std::endl;
            TS_ASSERT_THROWS_NOTHING(throw(err));
        }

    }
};

#endif