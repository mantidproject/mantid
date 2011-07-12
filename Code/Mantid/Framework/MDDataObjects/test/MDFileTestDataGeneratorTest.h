#ifndef MDFILE_TESTDATAGENERATOR_TEST_H
#define MDFILE_TESTDATAGENERATOR_TEST_H
#include <cxxtest/TestSuite.h>

#include "MDDataObjects/MD_FileFormatFactory.h"
#include "MDDataObjectsTestHelpers/MD_FileTestDataGenerator.h"
#include "MDDataObjectsTestHelpers/MDTestWorkspace.h"

#include "MDDataObjects/MDWorkspace.h"

using namespace Mantid;
using namespace MDDataObjects;

class MDFileTestDataGeneratorTest : public CxxTest::TestSuite
{

    // auxiliary variables extracted from workspace;
  
    MDDataPoints             * pMDDPoints;
    MDImage                  * pMDImg;
  // workspace itself
    MDWorkspace_sptr spMDWs;

  
public:
    void testMDFTDConstructor(){
        std::auto_ptr<MDDataTestHelper::MDTestWorkspace> pMDWS;
		TSM_ASSERT_THROWS_NOTHING("Initiating test MD workspace should not throw",pMDWS=std::auto_ptr<MDDataTestHelper::MDTestWorkspace>
            (new MDDataTestHelper::MDTestWorkspace()));

        spMDWs                    = pMDWS->get_spWS();

     
        pMDImg                    = spMDWs->get_spMDImage().get();
        pMDDPoints                = spMDWs->get_spMDDPoints().get();

 
    }
    void testMDFTDReadData(){

		IMD_FileFormat  & Reader  = spMDWs->get_const_FileReader();
        // read data;
        TSM_ASSERT_THROWS_NOTHING("Read MD image should not throw",Reader.read_MDImg_data(*pMDImg));
 //       TSM_ASSERT_THROWS_NOTHING("Init pix loactions should not throw",pMDDPoints->init_pix_locations());
    }
    void testWSSizes(){
        size_t nSample_Cells= 10*10*10*10;    // this value is specified in MDTestWorkspace default constructror
		uint64_t nTpoints   = 100*100*100*100; // this value is specified in MDTestWorkspace default constructror

        size_t nCells      = pMDImg->getDataSize();
        uint64_t nPoints   = pMDDPoints->getNumPixels();
        TSM_ASSERT_EQUALS("Test number of pixels shoule be equal to the initial one",nTpoints,nPoints);
        TSM_ASSERT_EQUALS("Test number of cells shoule be equal to the initial one",nSample_Cells,nCells);
    }
    void testReadCell(){
        std::vector<size_t> SelectedCells(1);
        SelectedCells[0] =1000;

		IMD_FileFormat  & Reader  = spMDWs->get_const_FileReader();

        unsigned int pix_size     = pMDDPoints->sizeofMDDataPoint();
        // the buffer (insufficient) to place pixels
        std::vector<char> data_buffer(100*pix_size);
        size_t n_pix_in_buffer(0);

        Reader.read_pix_subset(*pMDImg,SelectedCells,0,data_buffer,n_pix_in_buffer);

        TSM_ASSERT_EQUALS("Number of pixels sholud be approximately equal to 10^nDim ",10*10*10*10,n_pix_in_buffer);

        TSM_ASSERT_EQUALS("data buffer should be reallocated properly",n_pix_in_buffer*pix_size,data_buffer.size());
        // verify test data in buffer
        std::vector<int> ind;
        float S,Err;
        // we know the number of dims and number of reciprocal dims so n_ind defined

        size_t nRecDim = pMDImg->getGeometry().getNumReciprocalDims();
        size_t nDims   = spMDWs->getNumDims();
        size_t nInd    = nDims-nRecDim+2;

        // start test data correct;
        retrieveNDPix(&data_buffer[0],S,Err,ind,nDims,nInd);
        TSM_ASSERT_DELTA("Start signal incorrect",1.,S,1e-6);
        TSM_ASSERT_DELTA("Start error incorrect",0.5,Err,1e-6);
        for(size_t i=0;i<nInd;i++){
            TSM_ASSERT_EQUALS("Start index n-2 incorrect",2+i,ind[i]);
        }
       // end test data correct;
        retrieveNDPix(&data_buffer[(n_pix_in_buffer-1)*pix_size],S,Err,ind,nDims,nInd);
        TSM_ASSERT_DELTA("Start signal incorrect",1.,S,1e-6);
        TSM_ASSERT_DELTA("Start error incorrect",0.5,Err,1e-6);
        for(size_t i=0;i<nInd;i++){
            TSM_ASSERT_EQUALS("Start index n-2 incorrect",2+i,ind[i]);
        }

      // middle test data correct;
        retrieveNDPix(&data_buffer[(n_pix_in_buffer/2)*pix_size],S,Err,ind,nDims,nInd);
        TSM_ASSERT_DELTA("Start signal incorrect",1.,S,1e-6);
        TSM_ASSERT_DELTA("Start error incorrect",0.5,Err,1e-6);
        for(size_t i=0;i<nInd;i++){
            TSM_ASSERT_EQUALS("Start index n-2 incorrect",2+i,ind[i]);
        }


        MD_image_point *pImgData = pMDImg->get_pData();

        TSM_ASSERT_EQUALS("Number of pixels in cell has to be equal to the number of pixels returned ",pImgData[SelectedCells[0]].npix,n_pix_in_buffer);
    }
private:
  void retrieveNDPix(const char *pData,float &S,float &Err,std::vector<int> &ind, size_t nDims, size_t nInd){
  // the function is complementary to MDDensityTestHelper, as it 
      float *pPoint = (float *)(pData);

      S   = *(pPoint+nDims);
      Err = *(pPoint+nDims+1);
      ind.resize(nInd);
      for(unsigned int i=0;i<nInd;i++){
        ind[i]=(int)*(pPoint+nDims+2+i);
      }
  }

    
};

#endif
