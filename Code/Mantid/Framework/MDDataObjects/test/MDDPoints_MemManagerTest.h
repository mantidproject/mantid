#ifndef MDDPOINTS_MEMMANAGER_TEST_H
#define MDDPOINTS_MEMMANAGER_TEST_H

#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MDDPoints_MemManager.h"
#include "MDDataObjects/MDImageDatatypes.h"

using namespace Mantid;
using namespace MDDataObjects;


// helper class to expose protected (actually private) functions of MDDPoints_MemManager
class MDDMemMan: public MDDPoints_MemManager
{
public:
  MDDMemMan(MD_img_data const &AllImgData,size_t nImageCells,unsigned int pix_size):
    MDDPoints_MemManager(AllImgData,nImageCells,pix_size)
  {	}
  //
  void add_pixels_in_memory(std::vector<char> &data_buffer,const std::vector<char> &all_pixels,const std::vector<bool> &pixels_selected,const std::vector<size_t> &cell_indexes,size_t n_selected_pixels){
    MDDPoints_MemManager::add_pixels_in_memory(data_buffer,all_pixels,pixels_selected,cell_indexes, n_selected_pixels);
  }
  //


};
//*****************************************
class MDDPoints_MemManagerTest :    public CxxTest::TestSuite
{
  MDDMemMan *pMemMan;
  MD_img_data ImgArray;
  size_t nCells;
  // data buffer to keep pixel data;
  std::vector<char> data_buffer;

   static const unsigned int  pix_size = 36;

  void buildTestImgData(size_t niCells){
    // this pseudo-image data are not consistent with the pixels
    nCells = niCells;
    ImgArray.data_size = nCells;
    ImgArray.data_array_size = nCells;
    ImgArray.data = new MD_image_point[nCells];

    MD_image_point* pData = ImgArray.data;
    uint64_t nPixels(0);
    for(size_t i=0;i<nCells;i++){
      pData[i].npix = i;
      pData[i].s    = (double)i;
      pData[i].err  = 1.;
      nPixels+=       i;
    }
    ImgArray.npixSum = nPixels;


  }
  void build_consistent_pixels(std::vector<char> &pix_buf,std::vector<bool> &pix_sel, std::vector<size_t> &cell_indexes){
    // this function simulates proper result for rebinning with pixels;
    uint64_t nPixels(0);
    size_t   MAX_MEM_DATA_SIZE(0);
    size_t n_pixLocal;
    MAX_MEM_DATA_SIZE = ~MAX_MEM_DATA_SIZE;

    if(ImgArray.data_size!=nCells){
      delete [] ImgArray.data;
      ImgArray.data            = new MD_image_point[nCells];
      ImgArray.data_size       = nCells;
      ImgArray.data_array_size = nCells;
    }
    MD_image_point* pData = ImgArray.data;
    for(size_t i=0;i<nCells;i++){
        pData[i].npix = i;
      pData[i].s    = i;
      pData[i].err  = 1;

      nPixels += pData[i].npix;
    }
    ImgArray.npixSum = nPixels;
    if(nPixels<MAX_MEM_DATA_SIZE){
      n_pixLocal = (size_t)nPixels;
    }else{
      throw(std::invalid_argument("data type is too big for this memory model"));
    }

    pix_buf.resize(2*n_pixLocal*pix_size);
    pix_sel.resize(2*n_pixLocal);
    cell_indexes.assign(2*n_pixLocal,-1);

  // assign nPixels almost equivalent pixels;
    float *pPix = (float *)(&pix_buf[0]);
    size_t ic(0);
    size_t ic_retained(0);
    for(size_t i=0;i<nCells;i++){
      size_t n_pix = pData[i].npix;
      for(size_t j=0;j<n_pix;j++){
        // rebinned pixel
        pPix[ic*9+0]=1;
        pPix[ic*9+1]=2;
        pPix[ic*9+2]=3;
        pPix[ic*9+3]=4;
        pPix[ic*9+4]=(float)i;
        pPix[ic*9+5]= float(ic_retained);
      // indexes;
        pPix[ic*9+6]=(float)j;
        pPix[ic*9+7]=(float)n_pix;
        pPix[ic*9+8]=50;
        pix_sel[ic]= true;
        cell_indexes[ic_retained]=i;
        ic++;
        ic_retained++;
// now pixels which has not been rebinned
        pPix[ic*9+0]=-1;
        pPix[ic*9+1]=-2;
        pPix[ic*9+2]=-3;
        pPix[ic*9+3]=-4;
        pPix[ic*9+4]=(float)i;
        pPix[ic*9+5]=1/float(i+1);
      // indexes;
        pPix[ic*9+6]=100;
        pPix[ic*9+7]=200;
        pPix[ic*9+8]=50;
        pix_sel[ic]= false;
        ic++;

      }
    }
  }
  void add_consistent_pixels(std::vector<char> &pix_buf,std::vector<bool> &pix_sel, std::vector<size_t> &cell_indexes){
    // this function simulates proper result for rebinning with pixels;
    uint64_t nPixels(0);
    if(ImgArray.data_size!=nCells){
      throw(std::invalid_argument("Can not add pixels to different number of cells;"));
    }
    MD_image_point* pData = ImgArray.data;
    for(size_t i=0;i<nCells;i++){
        pData[i].npix += i;
      pData[i].s     = i;
      pData[i].err   = 1;

      nPixels += pData[i].npix;
    }
    ImgArray.npixSum = nPixels;

    pix_buf.resize(nPixels*pix_size);
    pix_sel.resize(nPixels);
    cell_indexes.assign(nPixels,-1);

  // assign nPixels almost equivalent pixels;
    float *pPix = (float *)(&pix_buf[0]);
    size_t ic(0);
    size_t ic_retained(0);
    for(size_t i=0;i<nCells;i++){
      size_t n_pix = pData[i].npix/2;
      for(size_t j=0;j<n_pix;j++){
        // rebinned pixel
        pPix[ic*9+0]=1;
        pPix[ic*9+1]=2;
        pPix[ic*9+2]=3;
        pPix[ic*9+3]=4;
        pPix[ic*9+4]=(float)i;
        pPix[ic*9+5]=float(ic_retained);
      // indexes;
        pPix[ic*9+6]=(float)j;
        pPix[ic*9+7]=(float)n_pix;
        pPix[ic*9+8]=50;
        pix_sel[ic]= true;
        cell_indexes[ic_retained]=i;
        ic++;
        ic_retained++;
// now pixels which has not been rebinned
        pPix[ic*9+0]=-1;
        pPix[ic*9+1]=-2;
        pPix[ic*9+2]=-3;
        pPix[ic*9+3]=-4;
        pPix[ic*9+4]=(float)i;
        pPix[ic*9+5]=1/float(i+1);
      // indexes;
        pPix[ic*9+6]=1000;
        pPix[ic*9+7]=2000;
        pPix[ic*9+8]=500;
        pix_sel[ic]= false;
        ic++;

      }
    }
  }
public:

  void testMDDPMemManConstructor(){
    buildTestImgData(256);
    TSM_ASSERT_THROWS_NOTHING("Mem manager constructor should not throw, as this invalidates all tests below",pMemMan = new MDDMemMan(ImgArray,nCells,pix_size));
  }
  void testAllocPixArray(){
    std::vector<char> Buf1;
    TSM_ASSERT_THROWS_NOTHING("Getting buffer should not throw",pMemMan->alloc_pix_array(Buf1,50));
    TSM_ASSERT_THROWS_NOTHING("Getting bigger buffer should not throw",pMemMan->alloc_pix_array(Buf1,100));
    TSM_ASSERT_THROWS_NOTHING("Getting even bigger buffer should not throw",pMemMan->alloc_pix_array(Buf1,200));

// cleaning up after test above
    delete pMemMan;
    TSM_ASSERT_THROWS_NOTHING("Mem manager constructor should not throw, as this invalidates all tests below",pMemMan = new MDDMemMan(ImgArray,nCells,pix_size));
  }
  void testAddPixelsInMemoryThrows(){
    std::vector<char> AllPixels;
      std::vector<bool> pix_sel;	
    std::vector<size_t> cell_indexes;
   
    size_t nPixels = 100;
    size_t n_selected_pixels(50);

    AllPixels.resize(nPixels*pix_size);
    pix_sel.resize(nPixels);
    cell_indexes.resize(nPixels);

  
    TSM_ASSERT_THROWS("This should throw as pixels and image are not consistent",pMemMan->add_pixels_in_memory(data_buffer,AllPixels,pix_sel,cell_indexes,n_selected_pixels),std::invalid_argument);
  }
//
  void testAddPixelsInEmptyMemory(){
    std::vector<char> AllPixels;
      std::vector<bool> pix_sel;	
    std::vector<size_t> cell_indexes;
  
    build_consistent_pixels(AllPixels,pix_sel,cell_indexes);
    size_t n_selected_pixels = pix_sel.size()/2;
    data_buffer.resize(n_selected_pixels*this->pix_size);

        TSM_ASSERT_THROWS_NOTHING("Adding consistent pixels in memory should not throw",pMemMan->add_pixels_in_memory(data_buffer,AllPixels,pix_sel,cell_indexes,n_selected_pixels));

  // verify if everything has been added properly (user usually should not use this function to get access to data)
      float *pData =(float *)(&(data_buffer[0]));
  
    // nPix-j = (nCell-1)*(nCell-2)/2;
    for(size_t i=0;i<n_selected_pixels;i++){
    
      //TSM_ASSERT_DELTA("Signal in a properly read pixel should be equal to the nuber of cell",CellNum,pData[i*9+4],1e-5);
            TSM_ASSERT_DELTA("Error in this pixel should be equal to the nuber of pixel",i,pData[i*9+5],1e-5);
      TSM_ASSERT_DELTA("All Qx values in a properly read pixels should be set to one ",1,pData[i*9],1e-5);
      TSM_ASSERT_DELTA("All last indexes in a properly read pixels should be set to 50 ",50,pData[i*9+8],1e-5);
    }


  }
  void testAddInconsistentPixelsInMemoryToExistingThrows(){
    std::vector<char> AllPixels;
      std::vector<bool> pix_sel;	
    std::vector<size_t> cell_indexes;


      size_t nPixels = 100;
    size_t n_selected_pixels(50);

    AllPixels.resize(nPixels*pix_size);
    pix_sel.resize(nPixels);
    cell_indexes.resize(nPixels);

    TSM_ASSERT_THROWS("This should throw as pixels and image anre not consistent",pMemMan->add_pixels_in_memory(data_buffer,AllPixels,pix_sel,cell_indexes,n_selected_pixels),std::invalid_argument);
  }

  void testAddConsistentPixelsInMemory(){
    std::vector<char> AllPixels;
      std::vector<bool> pix_sel;	
    std::vector<size_t> cell_indexes;
    add_consistent_pixels(AllPixels,pix_sel,cell_indexes);
    size_t n_selected_pixels = pix_sel.size()/2;

        TSM_ASSERT_THROWS_NOTHING("Adding consistent pixels in memory to existent pixels should not throw",pMemMan->add_pixels_in_memory(data_buffer,AllPixels,pix_sel,cell_indexes,n_selected_pixels));

    // verify if everything has been added properly (user should not use this function to get access to data)
        float *pData =(float *)(&(data_buffer[0]));
    size_t nPixels = nCells*(nCells-1);
    TSM_ASSERT_EQUALS("It should be specified number of pixels in menory after this operation ",nPixels,pMemMan->getNPixInMemory());
      
    // nPix-j = (nCell-1)*(nCell-2)/2;
    size_t nCell,nPix,nPixSum;
    for(size_t i=0;i<nPixels;i++){
        nCell = (size_t)pData[i*9+4];
      nPix  = (nCell-1)*nCell/2 +(size_t)pData[i*9+6];
      nPixSum = (size_t)pData[i*9+7];
           //TSM_ASSERT_DELTA("Signal in a properly read pixel should be equal to the nuber of cell",CellNum,pData[i*9+4],1e-5);
            TSM_ASSERT_DELTA("The obtained values should be equal by construction: ",nPixSum,nCell,1e-5);
      TSM_ASSERT_DELTA("All Qx values in a properly read pixels should be set to one ",1,pData[i*9],1e-5);
      TSM_ASSERT_DELTA("All last indexes in a properly read pixels should be set to 50 ",50,pData[i*9+8],1e-5);
    }

  }
  void testReadBlockOfCellsFromMemory(){
    std::vector<size_t> selected_cells(4);
    std::vector<char>   out_pix_buffer(100*36);
    size_t nCells = pMemMan->getNControlCells();
    size_t nSelCells = selected_cells.size();
    std::vector<size_t> expectations(nSelCells*(nSelCells-1));
    size_t ic(0);
    for(size_t i=0;i<nSelCells;i++){
      selected_cells[i] = i;
      for(size_t j=0;j<2*i;j++){
        expectations[ic]=i;
        ic++;
      }

    }
    
    size_t starting_cell(0);
    size_t n_pix_inBuffer;

    TSM_ASSERT_THROWS_NOTHING("Reading set of cells fitting buffer should not throw",pMemMan->get_pix_from_memory(data_buffer,selected_cells,starting_cell,out_pix_buffer,n_pix_inBuffer));

    TSM_ASSERT_EQUALS("First four cells should contain 12 pixels",12,n_pix_inBuffer);
       TSM_ASSERT_EQUALS("All contents of the last cell should be read properly ",true, pMemMan->is_readCell_completed());
    float *pData =(float *)(&out_pix_buffer[0]);

  //	int CellNum(selected_cells[0]),nc(0);
      //  MD_image_point* pImgData = ImgArray.data;
  
    for(size_t i=0;i<n_pix_inBuffer;i++){
      //TSM_ASSERT_DELTA("Signal in a properly read pixel should be equal to the nuber of cell",CellNum,pData[i*9+4],1e-5);
            TSM_ASSERT_DELTA("Properly read pixel's signal should be equal to the nuber expected ",expectations[i],pData[i*9+4],1e-5);
      TSM_ASSERT_DELTA("All Qx values in a properly read pixels should be set to one ",1,pData[i*9],1e-5);
      TSM_ASSERT_DELTA("All last indexes in a properly read pixels should be set to 50 ",50,pData[i*9+8],1e-5);
    }

  }
  void testReadBlockOfCellsFromMemoryIncomplete(){
    // this read should fill buffer with the pixels read 
    std::vector<size_t> selected_cells(4);
    std::vector<char>   out_pix_buffer(100*36);
    size_t nCells = pMemMan->getNControlCells();
    std::vector<size_t> expectations;
    for(size_t i=0;i<selected_cells.size();i++){
      selected_cells[i] = 49+i;
      for(size_t j=0;j<2*selected_cells[i];j++){
        expectations.push_back(selected_cells[i]);

      }

    }
    size_t starting_cell(0);
    size_t n_pix_inBuffer;

    TSM_ASSERT_THROWS_NOTHING("Reading set of cells fitting buffer should not throw",starting_cell=pMemMan->get_pix_from_memory(data_buffer,selected_cells,starting_cell,out_pix_buffer,n_pix_inBuffer));

    TSM_ASSERT_EQUALS("First reading operation should fill all buffer",100,n_pix_inBuffer);
    TSM_ASSERT_EQUALS("This  reading be able to read one full and one incomplete cell",1,starting_cell);
    float *pData =(float *)(&out_pix_buffer[0]);

  
    size_t ic(0);
    for(size_t i=0;i<n_pix_inBuffer;i++){
            TSM_ASSERT_DELTA("Properly read pixel's signal should be equal to the nuber expected ",expectations[ic],pData[i*9+4],1e-5);
      ic++;
      TSM_ASSERT_DELTA("All Qx values in a properly read pixels should be set to one ",1,pData[i*9],1e-5);
      TSM_ASSERT_DELTA("All last indexes in a properly read pixels should be set to 50 ",50,pData[i*9+8],1e-5);
    }
  }
  void testReadBlockOfCellsFromMemoryIncompleteContinues(){
    // this read should continue reading where the previous function ended
    std::vector<size_t> selected_cells(4);
    std::vector<char>   out_pix_buffer(100*36);
    size_t nCells = pMemMan->getNControlCells();
    std::vector<size_t> expectations;

    for(size_t i=0;i<selected_cells.size();i++){
      selected_cells[i] = 49+i;
      for(size_t j=0;j<2*selected_cells[i];j++){
        expectations.push_back(selected_cells[i]);
      }

    }
    size_t starting_cell(1);
      size_t ic(100);
    size_t n_pix_inBuffer;

    TSM_ASSERT_THROWS_NOTHING("Reading set of cells fitting buffer should not throw",starting_cell=pMemMan->get_pix_from_memory(data_buffer,selected_cells,starting_cell,out_pix_buffer,n_pix_inBuffer));

    TSM_ASSERT_EQUALS("First reading operation should fill all buffer",100,n_pix_inBuffer);
    TSM_ASSERT_EQUALS("This  reading be able to read one full and one incomplete cell",2,starting_cell);
    float *pData =(float *)(&out_pix_buffer[0]);

    for(size_t i=0;i<n_pix_inBuffer;i++){
      //TSM_ASSERT_DELTA("Signal in a properly read pixel should be equal to the nuber of cell",CellNum,pData[i*9+4],1e-5);
            TSM_ASSERT_DELTA("Properly read pixel's signal should be equal to the nuber expected ",expectations[ic],pData[i*9+4],1e-5);
      ic++;

      TSM_ASSERT_DELTA("All Qx values in a properly read pixels should be set to one ",1,pData[i*9],1e-5);
      TSM_ASSERT_DELTA("All last indexes in a properly read pixels should be set to 50 ",50,pData[i*9+8],1e-5);
    }
    //**************************************************************************************************************************************************************************************
    // check consequentive reading operations
    TSM_ASSERT_THROWS_NOTHING("Reading set of cells fitting buffer should not throw",starting_cell=pMemMan->get_pix_from_memory(data_buffer,selected_cells,starting_cell,out_pix_buffer,n_pix_inBuffer));

    TSM_ASSERT_EQUALS("First reading operation should fill all buffer",100,n_pix_inBuffer);
    TSM_ASSERT_EQUALS("This  reading be able to read one full and one incomplete cell",3,starting_cell);
    TSM_ASSERT_EQUALS("Ocasionally we should read all contents of the cells completed",true, pMemMan->is_readCell_completed());
  
  
    for(size_t i=0;i<n_pix_inBuffer;i++){
          TSM_ASSERT_DELTA("Properly read pixel's signal should be equal to the nuber expected ",expectations[ic],pData[i*9+4],1e-5);
      ic++;

            //TSM_ASSERT_DELTA("Error in a properly read pixel should be equal to the nuber of pixel",selected_cells[2]*(selected_cells[2]-1)+2+i,pData[i*9+5],1e-5);
      TSM_ASSERT_DELTA("All Qx values in a properly read pixels should be set to one ",1,pData[i*9],1e-5);
      TSM_ASSERT_DELTA("All last indexes in a properly read pixels should be set to 50 ",50,pData[i*9+8],1e-5);
    }

      TSM_ASSERT_THROWS_NOTHING("Reading set of cells fitting buffer should not throw",starting_cell=pMemMan->get_pix_from_memory(data_buffer,selected_cells,starting_cell,out_pix_buffer,n_pix_inBuffer));

      TSM_ASSERT_EQUALS("First reading operation should fill all buffer",100,n_pix_inBuffer);
    TSM_ASSERT_EQUALS("This  reading be able to read one full and one incomplete cell",3,starting_cell);
    TSM_ASSERT_EQUALS("We have not finished reading all pixels from the last cell",false, pMemMan->is_readCell_completed());
  
// cleaning up dependencies after the tests above
    delete pMemMan;
    data_buffer.clear();
    TSM_ASSERT_THROWS_NOTHING("Mem manager constructor should not throw, as this invalidates all tests below",pMemMan = new MDDMemMan(ImgArray,nCells,pix_size));
  }

  void testStorePixelsToNewMemBlock(){
    std::vector<char> AllPixels;
      std::vector<bool> pix_sel;	
    std::vector<size_t> cell_indexes;
    build_consistent_pixels(AllPixels,pix_sel,cell_indexes);	
    size_t n_selected_pixels = pix_sel.size()/2;
    size_t free_memory = 3*n_selected_pixels*36/3;

    TSM_ASSERT_THROWS_NOTHING("Should add pixels to new buffer in memory without throwing",pMemMan->store_pixels(AllPixels,pix_sel,cell_indexes,n_selected_pixels,free_memory,data_buffer));

    // verify if everything has been added properly (user should not use this function to get access to data)
    float *pData =(float *)(&data_buffer[0]);
  
    // nPix-j = (nCell-1)*(nCell-2)/2;
    for(size_t i=0;i<n_selected_pixels;i++){
    
      //TSM_ASSERT_DELTA("Signal in a properly read pixel should be equal to the nuber of cell",CellNum,pData[i*9+4],1e-5);
            TSM_ASSERT_DELTA("Error in a properly read pixel should be equal to the nuber of pixel",i,pData[i*9+5],1e-5);
      TSM_ASSERT_DELTA("All Qx values in a properly read pixels should be set to one ",1,pData[i*9],1e-5);
      TSM_ASSERT_DELTA("All last indexes in a properly read pixels should be set to 50 ",50,pData[i*9+8],1e-5);
    }

  }
  void testStoreMorePixelsToNewMemBlock(){
    std::vector<char> AllPixels;
      std::vector<bool> pix_sel;	
    std::vector<size_t> cell_indexes;
    add_consistent_pixels(AllPixels,pix_sel,cell_indexes);
    
    size_t n_selected_pixels = pix_sel.size()/2;
    size_t free_memory = 3*n_selected_pixels*36;

    TSM_ASSERT_THROWS_NOTHING("Should add pixels to old buffer in memory without throwing",pMemMan->store_pixels(AllPixels,pix_sel,cell_indexes,n_selected_pixels,free_memory,data_buffer));

    // verify if everything has been added properly (user should not use this function to get access to data)
    size_t n_pix_in_memory = pMemMan->getNPixInMemory();
    TSM_ASSERT_EQUALS("It should be double of selected pixels in memory after second addition",2*n_selected_pixels,n_pix_in_memory);


      float *pData =(float *)(&data_buffer[0]);
  
    size_t nCl,sumI(0);
    for(size_t i=0;i<n_pix_in_memory;i++){
        nCl = size_t(pData[i*9+4]);
      sumI++;

//			TSM_ASSERT_DELTA("These numbers should be equal: ",2*sumI,pData[i*9+4]+pData[i*9+6],1e-5);
      TSM_ASSERT_DELTA("These numbers should be equal: ",pData[i*9+4],pData[i*9+7],1e-5);
   
      TSM_ASSERT_DELTA("All Qx values in a properly read pixels should be set to one ",1,pData[i*9],1e-5);
      TSM_ASSERT_DELTA("All last indexes in a properly read pixels should be set to 50 ",50,pData[i*9+8],1e-5);
    }

  }

  MDDPoints_MemManagerTest():pMemMan(NULL){};
  ~MDDPoints_MemManagerTest(){
    if(pMemMan)delete pMemMan;
    if(ImgArray.data)delete [] ImgArray.data;
  }
};


#endif