#ifndef MDDENSITY_TEST_HELPER_TEST_H_
#define MDDENSITY_TEST_HELPER_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MD_FileFormatFactory.h"
#include "MDDataObjectsTestHelpers/MD_FileTestDataGenerator.h"
#include "MDDataObjectsTestHelpers/MDDensityTestHelper.h"

using namespace Mantid;
using namespace MDDataTestHelper;
class MDDensityHomogeneousTester: public MDDensityHomogeneous
{
public:
    MDDensityHomogeneousTester(const Geometry::MDGeometryDescription &geomDescr):   MDDensityHomogeneous(geomDescr){}
    std::vector<uint64_t>const & getFineStride()const{return MDDensityHomogeneous::getFineStride();}
    std::vector<size_t>const & getCoarseStride()const{return MDDensityHomogeneous::getCoarseStride();}
    const std::vector<size_t> findCoarseIndexes(size_t ind)const{ 
        std::vector<size_t> rez(nDims,0);
        MDDensityHomogeneous::findCoarseIndexes(ind,rez);
        return rez;
    }
    const std::vector<uint64_t>  findFineIndexes(uint64_t ind)const{ 
        std::vector<uint64_t> rez(nDims,0);
        MDDensityHomogeneous::findFineIndexes(ind,rez);
        return rez;
    }
    bool ind_plus(const std::vector<uint64_t> &ind_min,const std::vector<uint64_t> &ind_max, std::vector<uint64_t> &ind)const{
         return  MDDensityHomogeneous::ind_plus(ind_min,ind_max,ind);
    }
    uint64_t getCellPixCoordinates(size_t ind, std::vector<MDDPoint_t> &coord)const{
        return MDDensityHomogeneous::getCellPixCoordinates(ind, coord);
    }

};

class MDDensityTestHelperTest : public CxxTest::TestSuite
{
public:
  static MDDensityTestHelperTest *createSuite() { return new MDDensityTestHelperTest(); }
  static void destroySuite(MDDensityTestHelperTest *suite) { delete suite; }
 

  void testConstructorThrows1D(){
      std::auto_ptr<Geometry::MDGeometryDescription> pnDescr = std::auto_ptr<Geometry::MDGeometryDescription>(new Geometry::MDGeometryDescription(1,1));
      TSM_ASSERT_THROWS("should be at least 2 dimensions",MDDensityHomogeneous(pnDescr.get()),std::invalid_argument);

  }
  void testDimStrides(){
      std::vector<uint64_t> fine_strides = pHomDens->getFineStride();
      std::vector<uint64_t> default_f_patten(4);
      default_f_patten[0]=1;
      default_f_patten[1]=2102;
      default_f_patten[2]=2102*default_f_patten[1];
      default_f_patten[3]=2102*default_f_patten[2];

      for(size_t i=0;i<fine_strides.size();i++){
          TS_ASSERT_EQUALS(default_f_patten[i],fine_strides[i]);
      }

      std::vector<size_t> coarse_strides = pHomDens->getCoarseStride();
      std::vector<uint64_t> default_c_patten(4);
      default_c_patten[0]=1;
      default_c_patten[1]=50;
      default_c_patten[2]=50*default_c_patten[1];
      default_c_patten[3]=50*default_c_patten[2];
     for(size_t i=0;i<coarse_strides.size();i++){
          TS_ASSERT_EQUALS(default_c_patten[i],coarse_strides[i]);
      }

  }
  void testConstructorWith3DDescr(){
      // check one dimension collapsed
       std::auto_ptr<Geometry::MDGeometryDescription> pDescr = std::auto_ptr<Geometry::MDGeometryDescription>(new Geometry::MDGeometryDescription(4,3));
       pDescr->pDimDescription(1)->nBins = 1;

       pHomDens = std::auto_ptr<MDDensityHomogeneousTester>(new MDDensityHomogeneousTester(*pDescr));

       TSM_ASSERT_EQUALS("the pixel size should be 4 dimensions+ (2 indexes from rec dim+1-orthogonal)=3+2 for singal and error, all multiplied by 4",
                          4*(4+3+2),pHomDens->sizeofMDDataPoint());

      std::vector<size_t> coarse_strides = pHomDens->getCoarseStride();
      std::vector<uint64_t> default_c_patten(4);
      default_c_patten[0]=1;
      default_c_patten[1]=0;
      default_c_patten[2]=50*default_c_patten[0];
      default_c_patten[3]=50*default_c_patten[2];
      for(size_t i=0;i<coarse_strides.size();i++){
          TS_ASSERT_EQUALS(default_c_patten[i],coarse_strides[i]);
      }
  }
  void testCalcIndexes(){
      std::vector<size_t> coarse_strides   = pHomDens->getCoarseStride();
      std::vector<uint64_t> fine_strides   = pHomDens->getFineStride();

      std::vector<uint64_t> guess_ind(4,0);
      guess_ind[0]=3;
      guess_ind[1]=4;
      guess_ind[2]=5;
      guess_ind[3]=6;

      size_t coarse_ind = size_t(guess_ind[0]*coarse_strides[0]+guess_ind[1]*coarse_strides[1]+guess_ind[2]*coarse_strides[2]+guess_ind[3]*coarse_strides[3]);
      uint64_t fine_ind = guess_ind[0]*fine_strides[0]  +guess_ind[1]*fine_strides[1]  +guess_ind[2]*fine_strides[2]  +guess_ind[3]*fine_strides[3];

      std::vector<size_t> ci = pHomDens->findCoarseIndexes(coarse_ind);
      std::vector<uint64_t> fi = pHomDens->findFineIndexes(fine_ind);
      // coarce grid here is 3D
      TSM_ASSERT_EQUALS("this coarse should be equal 0",0,ci[1]);
      ci[1]=4;
      for(size_t i=0;i<guess_ind.size();i++){
          TSM_ASSERT_EQUALS("coarse should be equal",guess_ind[i],ci[i]);
          TSM_ASSERT_EQUALS("fine should be equal",guess_ind[i],fi[i]);
      }
  }
  void testIndPlus(){
      std::vector<uint64_t> ind_min(4,1);
      std::vector<uint64_t> ind(4,1);
      std::vector<uint64_t> ind_max(4,3);

      unsigned int ic(0);
      bool index_correct(true);
      while(index_correct){
        index_correct=pHomDens->ind_plus(ind_min,ind_max,ind);
        ic++;
      }
      TSM_ASSERT_EQUALS("should do 2*2*2*2 steps",16,ic);
      for(size_t i=0;i<4;i++)TSM_ASSERT_EQUALS("final index should be equal to maximal index less one",ind[i],ind_max[i]);
  }
  void testGetQContr(){
    // build 3D geometry description
    std::auto_ptr<Geometry::MDGeometryDescription> pDescr = std::auto_ptr<Geometry::MDGeometryDescription>(new Geometry::MDGeometryDescription(3,2));
    pDescr->pDimDescription(0)->nBins = 10;
    pDescr->pDimDescription(1)->nBins = 10;
    pDescr->pDimDescription(2)->nBins = 10;
    pDescr->nContributedPixels=100*100*100;

    // arrange test dataset on the basis of this description
    pHomDens = std::auto_ptr<MDDensityHomogeneousTester>(new MDDensityHomogeneousTester(*pDescr));

    TSM_ASSERT_EQUALS("the pixel size should be 3 dimensions+ (2 indexes from rec dim+1-orthogonal)=3 +2 for singal and error, all multiplied by 4",
                          4*(3+3+2),pHomDens->sizeofMDDataPoint());

    uint64_t nContr_pix  = pHomDens->coarseCellCapacity(159);
    std::vector<float> coord;
    uint64_t nContr_pix1 = pHomDens->getCellPixCoordinates(159,coord);
    TS_ASSERT_EQUALS(nContr_pix,nContr_pix1);
    //TODO: need to verify coordinates themself

  }
  void testGetHomogeneousData(){
      //
      double S,Err;
      uint64_t nPix;
      // random cell, all cells are equal at the moment, but randomisation errors lead 
      //to various number of contribution of subsells into each cell.
      pHomDens->getMDImageCellData(38,S,Err,nPix);
      TS_ASSERT_DELTA(double(nPix),S,1e-5);
      TS_ASSERT_DELTA(0.5/double(nPix),Err,1e-5);
      TS_ASSERT_EQUALS(nPix,pHomDens->coarseCellCapacity(38));

      size_t nnPix    = (size_t)nPix;
      size_t PixStride= pHomDens->sizeofMDDataPoint();
      size_t buf_size = nnPix*PixStride;

      std::vector<char> dataBuf(buf_size );
      pHomDens->getMDDPointData(38,&dataBuf[0],buf_size,nnPix);

      // 3-dimensional dataset
      float Sp,ERp;
      int ind1,ind2,ind3;

      retrieve3DPix(&dataBuf[0],Sp,ERp,ind1,ind2,ind3);
      TSM_ASSERT_DELTA("Start signal incorrect",1.,Sp,1e-6);
      TSM_ASSERT_DELTA("Start error incorrect",0.5,ERp,1e-6);
      TSM_ASSERT_EQUALS("Start ind1 incorrect",2,ind1);
      TSM_ASSERT_EQUALS("Start ind2 incorrect",3,ind2);
      TSM_ASSERT_EQUALS("Start ind3 incorrect",4,ind3);

      retrieve3DPix(&dataBuf[(nnPix-1)*PixStride],Sp,ERp,ind1,ind2,ind3);
      TSM_ASSERT_DELTA("End signal incorrect",1.,Sp,1e-6);
      TSM_ASSERT_DELTA("End error incorrect",0.5,ERp,1e-6);
      TSM_ASSERT_EQUALS("End ind1 incorrect",2,ind1);
      TSM_ASSERT_EQUALS("End ind2 incorrect",3,ind2);
      TSM_ASSERT_EQUALS("End ind3 incorrect",4,ind3);


      retrieve3DPix(&dataBuf[(nnPix/2)*PixStride],Sp,ERp,ind1,ind2,ind3);
      TSM_ASSERT_DELTA("Middle signal incorrect",1.,Sp,1e-6);
      TSM_ASSERT_DELTA("Middle error incorrect",0.5,ERp,1e-6);
      TSM_ASSERT_EQUALS("Middle ind1 incorrect",2,ind1);
      TSM_ASSERT_EQUALS("Middle ind2 incorrect",3,ind2);
      TSM_ASSERT_EQUALS("Middle ind3 incorrect",4,ind3);


  }

  void test3DImageOver5DDataset(){
    // build 5D geometry description with 2 integrated axis
    std::auto_ptr<Geometry::MDGeometryDescription> pDescr = std::auto_ptr<Geometry::MDGeometryDescription>(new Geometry::MDGeometryDescription(5,3));
    pDescr->pDimDescription(0)->nBins = 10;
    pDescr->pDimDescription(1)->nBins = 1;
    pDescr->pDimDescription(2)->nBins = 10;
    pDescr->pDimDescription(3)->nBins = 1;
    pDescr->pDimDescription(4)->nBins = 10;

    pDescr->nContributedPixels=(100000000);

    // arrange test dataset on the basis of this description
    pHomDens = std::auto_ptr<MDDensityHomogeneousTester>(new MDDensityHomogeneousTester(*pDescr));

    TSM_ASSERT_EQUALS("the pixel size should be 5 dimensions+ (2 indexes from rec dim+ 2 for orthogonal)=4 +2 for singal and error, all multiplied by 4",
                          4*(5+4+2),pHomDens->sizeofMDDataPoint());

    // check N-D data
     double S,Err;
     uint64_t nPix;
     // random cell, all cells are equal at the moment, but randomisation errors lead 
     //to various number of contribution of subsells into each cell.
     pHomDens->getMDImageCellData(950,S,Err,nPix);
     TS_ASSERT_DELTA(double(nPix),S,1e-5);
     TS_ASSERT_DELTA(1./double(nPix),Err,1e-5);
     TS_ASSERT_EQUALS(nPix,pHomDens->coarseCellCapacity(950));

     size_t nnPix    = (size_t)nPix;
     size_t pixSize  = pHomDens->sizeofMDDataPoint();
     size_t buf_size = nnPix*pixSize;

     std::vector<char> dataBuf(buf_size );
     pHomDens->getMDDPointData(950,&dataBuf[0],buf_size,nnPix);

      // 5-dimensional dataset
     float Sp,Erp;
     std::vector<int>  ind;
     unsigned int nDims(5),nInd(4);

     retrieveNDPix(&dataBuf[0],Sp,Erp,ind,nDims,nInd);
     TSM_ASSERT_DELTA("Start signal incorrect",1.,Sp,1e-6);
     TSM_ASSERT_DELTA("Start error incorrect",0.5,Erp,1e-6);
     for(size_t i=0;i<nInd;i++){
            TSM_ASSERT_EQUALS("Start index n-2 incorrect",2+i,ind[i]);
     }
       // end test data correct;
        retrieveNDPix(&dataBuf[(nnPix-1)*pixSize],Sp,Erp,ind,nDims,nInd);
        TSM_ASSERT_DELTA("Start signal incorrect",1.,Sp,1e-6);
        TSM_ASSERT_DELTA("Start error incorrect",0.5,Erp,1e-6);
        for(size_t i=0;i<nInd;i++){
            TSM_ASSERT_EQUALS("Start index n-2 incorrect",2+i,ind[i]);
        }

      // middle test data correct;
        retrieveNDPix(&dataBuf[(nnPix/2)*pixSize],Sp,Erp,ind,nDims,nInd);
        TSM_ASSERT_DELTA("Start signal incorrect",1.,Sp,1e-6);
        TSM_ASSERT_DELTA("Start error incorrect",0.5,Erp,1e-6);
        for(size_t i=0;i<nInd;i++){
            TSM_ASSERT_EQUALS("Start index n-2 incorrect",2+i,ind[i]);
        }


  }
  MDDensityTestHelperTest(){
    std::auto_ptr<Geometry::MDGeometryDescription> pDescr = std::auto_ptr<Geometry::MDGeometryDescription>(new Geometry::MDGeometryDescription());
    pHomDens = std::auto_ptr<MDDensityHomogeneousTester>(new MDDensityHomogeneousTester(*pDescr));
  }
private:
    std::auto_ptr<MDDensityHomogeneousTester> pHomDens;
    void retrieve3DPix(const char *pData,float &S,float &Err,int &ind1,int &ind2, int &ind3){

      float *pPoint = (float *)(pData);

      S   = *(pPoint+3);
      Err = *(pPoint+4);
      ind1=(int)*(pPoint+5);
      ind2=(int)*(pPoint+6);
      ind3=(int)*(pPoint+7);

    }
    void retrieveNDPix(const char *pData,float &S,float &Err,std::vector<int> &ind,unsigned int nDims,unsigned int nInd){
   // the function is complementary to MDDensityTestHelper; asks for nDimensions and nIndexes for reciprocal dimensions 
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