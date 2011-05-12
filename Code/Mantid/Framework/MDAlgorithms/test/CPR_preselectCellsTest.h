#ifndef H_CPR_PRESELECT_CELLS
#define H_CPR_PRESELECT_CELLS
#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MDWorkspace.h"
#include "MantidMDAlgorithms/DynamicCPRRebinning.h"
#include "MDDataObjects/MDTestWorkspace.h"
using namespace Mantid;
using namespace API;
using namespace Kernel;
using namespace MDDataObjects;
using namespace MDAlgorithms;
// this test work in close cooperation with MDTestWorkspace where all test workspace parameters are defined;

class DynamicCPRRt: public DynamicCPRRebinning
{
public:
   DynamicCPRRt(const MDDataObjects::MDWorkspace_const_sptr &pSourceWS, 
                Geometry::MDGeometryDescription const * const pTargetDescr,
                const MDDataObjects::MDWorkspace_sptr  & TargetWS ):
   DynamicCPRRebinning(pSourceWS,pTargetDescr,TargetWS)
   {}
    virtual bool rebin_data_chunk(){return false;}
    virtual bool rebin_data_chunk_keep_pixels(){return false;}
	// this function is similar to the one defined in rebinning but has its own data_chunk (usually rebinning defines it)
    virtual unsigned int getNumDataChunks(void)const{
        unsigned int rez,max_int(0);
		max_int =~max_int;
		uint64_t n_data_chunks_rough = this->n_preselected_pix/1000;
		if(n_data_chunks_rough >(uint64_t)max_int){
			throw(std::invalid_argument(" number of data chunks for rebinning exceeds 2^32. Something wrong and you will not be able to rebin such number anyway"));
		}
        rez = (unsigned int)(n_data_chunks_rough);
        if(rez*1000!= this->n_preselected_pix)rez++;
        return rez;
    }
	// function accessing important internal variables in the class
	const std::vector<size_t> & getPreselectedCells()const{return preselected_cells;}
};

class CPR_preselectCellsTest :    public CxxTest::TestSuite
{
    boost::shared_ptr<MDDataObjects::MDWorkspace> pOrigin;
    boost::shared_ptr<MDDataObjects::MDWorkspace> pTarget;

    std::auto_ptr<Geometry::MDGeometryDescription> pTargDescr;

    std::auto_ptr<DynamicCPRRt>    pRebin;

	// function verifys if preselection has dublicated cells
	bool check_cell_unique(const std::vector<size_t> &cell_nums)const{
		for(size_t i=1;i<cell_nums.size();i++){
			if(cell_nums[i-1]==cell_nums[i])return true;
		}
		return false;
	}
 public:



     void testINIT_WS(){
         std::auto_ptr<MDTestWorkspace> tw = std::auto_ptr<MDTestWorkspace>(new MDTestWorkspace());
		 // get usual workspace from the test workspace
         pOrigin = tw->get_spWS();

         TSM_ASSERT_THROWS_NOTHING("Source WS should be constructed not throwing",pTarget= boost::shared_ptr<MDDataObjects::MDWorkspace>(new MDDataObjects::MDWorkspace()));

		 // init geometry description equal to the source geometry;
         TSM_ASSERT_THROWS_NOTHING("Target WS descr should be constructed not throwing",pTargDescr = 
             std::auto_ptr<Geometry::MDGeometryDescription>(new Geometry::MDGeometryDescription(pOrigin->get_const_MDGeometry())));


         TSM_ASSERT_THROWS_NOTHING("Target empty WS  should be constructed not throwing",pTarget = 
             boost::shared_ptr<MDDataObjects::MDWorkspace>(new MDDataObjects::MDWorkspace()));

		 // init target workspace as we need
		 TSM_ASSERT_THROWS_NOTHING("Target workspace initialisation should not throw",pTarget->init(pOrigin,pTargDescr.get()));
	 }
	 void testCPRConstructor(){

         TSM_ASSERT_THROWS_NOTHING("Rebinning should be constructed withoug throwing",pRebin  = 
             std::auto_ptr<DynamicCPRRt>(new DynamicCPRRt(pOrigin,pTargDescr.get(),pTarget)));

     }
     void testPreselectAllUnique(){
		 size_t nCells(0);

		 TSM_ASSERT_THROWS_NOTHING("Preselect cells should not normaly throw",nCells=pRebin->preselect_cells());
		 // check if the generic properties of the preselection are correct:
		 TSM_ASSERT_EQUALS("The selection above should describe nDim0*nDim1*nDim2*nDim3 geometry",50*50*50*50,nCells);

		 const std::vector<size_t> psCells = pRebin->getPreselectedCells();
		 TSM_ASSERT_EQUALS("All selected cells have to be unique but found non-unique numbers:",false,check_cell_unique(psCells));

     }
     void testPreselect3DWorks(){
		 size_t nCells(0);
		 pTargDescr->pDimDescription(3)->cut_max = 0;  
		 // modify description: The numbers have to be known from the source workspace and the workpsace range is from -1 to 49
		 pTargDescr->pDimDescription(2)->cut_max = 0.99;


		 TSM_ASSERT_THROWS_NOTHING("Preselect cells should not normaly throw",nCells=pRebin->preselect_cells());
		 // check if the generic properties of the preselection are correct:
		 TSM_ASSERT_EQUALS("The selection above should describe nDim0*nDim1*2*1 geometry",50*50*2*1,nCells);

		 const std::vector<size_t> psCells = pRebin->getPreselectedCells();
		 TSM_ASSERT_EQUALS("All selected cells have to be unique but found non-unique numbers:",false,check_cell_unique(psCells));

		 //
		 TSM_ASSERT_EQUALS("The selection should refer to nCells*(nCells+1)/2 pixels but it is not",nCells*(nCells+1)/2,pRebin->getNumPreselectedPixels());

     }
    void testPreselect3Dx2Works(){
		 size_t nCells(0);
		 pTargDescr->pDimDescription(3)->cut_max = 1;  

		 TSM_ASSERT_THROWS_NOTHING("Preselect cells should not normaly throw",nCells=pRebin->preselect_cells());
		 // check if the generic properties of the preselection are correct:
		 TSM_ASSERT_EQUALS("The selection above should describe nDim0*nDim1*2*1 geometry",50*50*2*2,nCells);

		 const std::vector<size_t> psCells = pRebin->getPreselectedCells();
		 TSM_ASSERT_EQUALS("All selected cells have to be unique but found non-unique numbers:",false,check_cell_unique(psCells));

		 //
		 size_t nHalfCells = nCells/2;
		 // number of pixels in the first half of the selection; verified above
		 uint64_t nPix   = nHalfCells*(nHalfCells+1)/2;
		 // other half of the selection:
		 nPix          += pOrigin->get_const_MDGeometry().get_constDimension(3)->getStride()*nHalfCells+(nHalfCells+1)*nHalfCells/2;

		 TSM_ASSERT_EQUALS("The selection should refer to proper number of pixels but it is not",nPix,pRebin->getNumPreselectedPixels());

     }
	 void testClearWorkspaces(){
		 //  not entirely according to standarts, but does not test anything but deletes workpsaces to free memory when running in suite
		 // before the real destructor is called
		 this->pOrigin.reset();
		 this->pTarget.reset();
		 this->pTargDescr.reset();
		 this->pRebin.reset();
	 }
};

#endif
