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
             //std::cout<<"\n start reading all pixels in memory and this can be long\n";
             //TS_ASSERT_THROWS_NOTHING(sqw_data.read_pix());

             // are pixels in memory?
             //TS_ASSERT_EQUALS(sqw_data.isMemoryBased(),true);
             size_t nPixels=sqw_data.genNumPixels();
             TS_ASSERT_EQUALS(nPixels,18287130);

             std::vector<size_t> selected_cells(1,0);
             selected_cells[0]=987;
             sqw_pixel *pix_buf = new sqw_pixel[PIX_BUFFER_SIZE];
             size_t buf_size = PIX_BUFFER_SIZE;
             size_t start_cell(0);

            TS_ASSERT_THROWS_NOTHING(start_cell=sqw_data.read_pix_selection(selected_cells,start_cell,pix_buf,buf_size));

        }catch(std::exception &err){ 
            std::cout<<" error of the SQW constructor "<<err.what()<<std::endl;
            TS_ASSERT_THROWS_NOTHING(throw(err));
        }

    }
};

#endif