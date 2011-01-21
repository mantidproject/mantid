#ifndef MDFILE_TESTDATAGENERATOR_TEST_H
#define MDFILE_TESTDATAGENERATOR_TEST_H
#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MDTestWorkspace.h"
#include "MDDataObjects/MD_FileTestDataGenerator.h"
#include "MDDataObjects/MD_FileFormatFactory.h"
#include "MDDataObjects/MDWorkspace.h"

using namespace Mantid;
using namespace MDDataObjects;

class MDFileTestDataGeneratorTest : public CxxTest::TestSuite
{

    // auxiliary variables extracted from workspace;
    IMD_FileFormat           * pReader;
    MDDataPoints             * pMDDPoints;
    MDImage                  * pMDImg;
  // workspace itself
    MDWorkspace_sptr spMDWs;

 
public:
    void testMDFTDConstructor(){
        std::auto_ptr<MDTestWorkspace> pMDWS;
		TSM_ASSERT_THROWS_NOTHING("Initiating test MD workspace should not throw",pMDWS=std::auto_ptr<MDTestWorkspace>(new MDTestWorkspace()));

        spMDWs                    = pMDWS->get_spWS();

        pReader                   = spMDWs->get_pFileReader();
        pMDImg                    = spMDWs->get_spMDImage().get();
        pMDDPoints                = spMDWs->get_spMDDPoints().get();

 
    }
    void testMDFTDReadData(){
  
        // read data;
        TSM_ASSERT_THROWS_NOTHING("Read MD image should not throw",pReader->read_MDImg_data(*pMDImg));
        TSM_ASSERT_THROWS_NOTHING("Init pix loactions should not throw",pMDDPoints->init_pix_locations());
    }
    void testWSSizes(){
        size_t nCells = pMDImg->getDataSize();
        uint64_t nPoints = pMDDPoints->getNumPixels();
        TSM_ASSERT_EQUALS("Test number of pixels shoule be equal to geometric progression of nCells",nCells*(nCells+1)/2,nPoints);
    }
    void testReadCell(){
        std::vector<size_t> SelectedCells(1);
        SelectedCells[0] =1000;

        unsigned int pix_size = pMDDPoints->sizeofMDDataPoint();
        // the buffer to place pixels
        std::vector<char> data_buffer((SelectedCells[0]+1)*pix_size);
        size_t n_pix_in_buffer;

        pReader->read_pix_subset(*pMDImg,SelectedCells,0,data_buffer,n_pix_in_buffer);

        TSM_ASSERT_EQUALS("Number of pixels for this datatype should be equal to the cell number",SelectedCells[0]+1,n_pix_in_buffer);

        TSM_ASSERT_EQUALS("data buffer should be reallocated properly",n_pix_in_buffer*pix_size,data_buffer.size());

        MD_image_point *pImgData = pMDImg->get_pData();

        TSM_ASSERT_EQUALS("Number of pixels in cell has to be equal to the number of pixels returned ",pImgData[SelectedCells[0]].npix,n_pix_in_buffer);
    }
   
    
};

#endif