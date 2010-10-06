#ifndef H_TEST_SQW
#define H_TEST_SQW


#include <cxxtest/TestSuite.h>
#include "MDPixels.h"

using namespace Mantid;
using namespace MDDataObjects;

class testMDPixels :    public CxxTest::TestSuite
{
    MDPixels  *pSQW;
    sqw_pixel *pix_buf;
    size_t     buf_size;
public:
    void testSQWConstructor(void){
        TS_ASSERT_THROWS_NOTHING(pSQW=new MDPixels(5));
    }
    void testSQWDNDread(void){
       
       // read DND object
             TS_ASSERT_THROWS_NOTHING(pSQW->read_mdd("../../../../Test/VATES/fe_demo.sqw"));
             // it can throw if the pixels do not fit into memory
             //std::cout<<"\n start reading all pixels in memory and this can be long\n";
             //TS_ASSERT_THROWS_NOTHING(sqw_data.read_pix());
    }
    void testSQWnPix(void){
             // are pixels in memory?
             //TS_ASSERT_EQUALS(sqw_data.isMemoryBased(),true);
             size_t nPixels;
             TS_ASSERT_THROWS_NOTHING(nPixels=pSQW->getNumPixels());

             // the dataset seems has wrong parameters -- this is question to HORACE hdf;
             //TS_ASSERT_EQUALS(nPixels,18287130);
             
    }
    void testSQWreadEmptySelection(void){
             std::vector<size_t> selected_cells(1,0);
             selected_cells[0]=987;
             pix_buf = new sqw_pixel[PIX_BUFFER_SIZE];
             buf_size= PIX_BUFFER_SIZE;
             size_t start_cell(0),n_pix_in_buffer(0);

            TS_ASSERT_THROWS_NOTHING(start_cell=pSQW->read_pix_selection(selected_cells,start_cell,pix_buf,buf_size,n_pix_in_buffer));
            TS_ASSERT_EQUALS(n_pix_in_buffer,0);
            TS_ASSERT_EQUALS(start_cell,1);
    }
    void testSQWreadDataSelection(void){
            size_t cells_nums[]={26904,26905,26906,26907,26908,26909,26910,26911,26912,26913};
            std::vector<size_t> selected_cells;
            selected_cells.assign(cells_nums,cells_nums+sizeof(cells_nums)/sizeof(size_t));
            // not forget to reset starting cell to 0 for independent reading operation (if it is consequent reading, it shold continue
             size_t start_cell(0),n_pix_in_buffer(0);

            TS_ASSERT_THROWS_NOTHING(start_cell=pSQW->read_pix_selection(selected_cells,start_cell,pix_buf,buf_size,n_pix_in_buffer));
            TS_ASSERT_EQUALS(n_pix_in_buffer,199);
            TS_ASSERT_EQUALS(start_cell,10);

    }
    testMDPixels(void):pSQW(NULL),pix_buf(NULL){};
    ~testMDPixels(void){
        if(pSQW)delete pSQW;
        if(pix_buf)delete [] pix_buf;
    }
};

#endif