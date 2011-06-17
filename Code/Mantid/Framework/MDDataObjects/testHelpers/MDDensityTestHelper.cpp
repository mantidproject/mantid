#include "MDDataObjectsTestHelpers/MDDensityTestHelper.h"

namespace Mantid{
namespace MDDataTestHelper{


MDDensityHomogeneous::MDDensityHomogeneous(const Geometry::MDGeometryDescription &geomDescr):
nContributedPixels(geomDescr.nContributedPixels),
MDDPixel_size(-1),
nIndexes(0)
{
    unsigned int i;

    this->nDims          =(unsigned int)geomDescr.getNumDims();
    if(nDims<2){ //TODO: should be relaxed to 1
        throw(std::invalid_argument("needs to have at least 2 dimensions"));
    }
 
    unsigned int nRecDim = geomDescr.getNumRecDims();
    unsigned int indLeft = nDims - nRecDim; // can not be negative as nDims>=nRecDims
    nIndexes     = 2+indLeft;  
    MDDPixel_size = (nDims+2+nIndexes)*sizeof(MDDPoint_t);

  
    this->nFullDims = nDims;

    // define fine grid
    this->fine_nbin.assign(nDims,1);
    this->fine_bin_stride.assign(nDims,1);
    this->fine_bin_size.assign(nDims,0);
    this->r_min.assign(nDims,0);
    this->r_max.assign(nDims,0);

    double rough_grid = pow(double(nContributedPixels),1./double(nDims));
    uint64_t nBins0   = uint64_t(rough_grid);
    if(nBins0<1)nBins0= 1;

    uint64_t nTotBins=1;
    for(i=0;i<this->nDims-1;i++){
        nTotBins        *= nBins0;
        fine_nbin[i]     = nBins0;
    }
    // adjust last fine bin to enclose all detectors values;
    uint64_t nLastBins = nContributedPixels/nTotBins;
    while(nLastBins*nTotBins<nContributedPixels)nLastBins++;

    fine_nbin[this->nDims-1]=nLastBins;


    r_min[0]         = geomDescr.pDimDescription(0)->cut_min;
    r_max[0]         = geomDescr.pDimDescription(0)->cut_max;
    fine_bin_size[0] = (r_max[0]-r_min[0])/fine_nbin[0];
    for(i=1;i<this->nDims;i++){
        fine_bin_stride[i]=fine_bin_stride[i-1]*fine_nbin[i-1];
        r_min[i]          = geomDescr.pDimDescription(i)->cut_min;
        r_max[i]          = geomDescr.pDimDescription(i)->cut_max;
        fine_bin_size[i]  = (r_max[i]-r_min[i])/fine_nbin[i];

    }
    fine_grid_size = fine_bin_stride[nDims-1]*fine_nbin[i-1];
    // define coarse grid
    this->coarse_bin_stride.assign(nDims,1);
    this->coarse_nbin.assign(nDims,1);
    this->coarse_bin_size.assign(nDims,0);
 
    unsigned int nIntegratedDim(0),nResolvedDim(0);
    // this to support an axis swap
    for(i=0;i<nDims;i++){
        if(geomDescr.pDimDescription(i)->nBins<=1){
            nIntegratedDim++;
        }else{
            nResolvedDim++;
        }
   }
    unsigned int ii(0),ir(0);
    size_t nBins;

    for(i=0;i<nDims;i++){
        nBins = geomDescr.pDimDescription(i)->nBins;
        if(nBins <=1) nBins=1;
        coarse_nbin[i]      = nBins;
        coarse_bin_size[i]  = (r_max[i]-r_min[i])/double(nBins);     
    }
    // calculate coarse grid strides; strades are 0 for integrated dimensions;
    size_t non_zero_nBin(1),non_zero_stride(1);
    for(i=0;i<nDims;i++){
        if(coarse_nbin[i]==1){
            coarse_bin_stride[i]=0;
        }else{
            coarse_bin_stride[i]=non_zero_nBin*non_zero_stride;
            non_zero_nBin        = coarse_nbin[i];
            non_zero_stride      = coarse_bin_stride[i];
        }
    }
    coarse_grid_size=non_zero_stride*non_zero_nBin;
}
//
void
MDDensityHomogeneous::getMDDPointData(size_t cell_index,char *pBuffer,size_t BufSize,size_t &nDataPoints)const
{
    uint64_t n_data_points= this->coarseCellCapacity(cell_index);
    size_t pix_size       = this->sizeofMDDataPoint();
    if(n_data_points*pix_size>BufSize){
        throw(std::invalid_argument("such cell can not be analyzed using current architectrue"));
    }
   
//    buffer.resize(size_t(n_data_points*pix_size));

    // this function returns constant signal and error, so we are providing 
    std::vector<MDDPoint_t> patch(2+nIndexes);
    patch[0]=1;   // signal
    patch[1]=0.5; // error
    for(size_t i=0;i<nIndexes;i++){ // indexes; not used so set arbitrary
        patch[2+i]=MDDPoint_t(2+i);
    }

    std::vector<MDDPoint_t> r;
    nDataPoints = (size_t)this->getCellPixCoordinates(cell_index,r);

    size_t field_width = sizeof(MDDPoint_t);
    for(size_t i=0;i<nDataPoints;i++){
      std::memcpy(pBuffer+i*pix_size,&r[i],field_width*this->nDims);
      std::memcpy(pBuffer+i*pix_size+field_width*this->nDims,&patch[0],field_width*(this->nIndexes+2));
    }

}
//
void 
MDDensityHomogeneous::get_rCoarseCell(size_t ind, std::vector<float> &r_cell)const
{
    std::vector<size_t> indexes(this->nDims,0);
    r_cell.resize(this->nDims);
    this->findCoarseIndexes(ind,indexes);
    for(unsigned int i=0;i<nFullDims;i++){
        r_cell[i]=(float)(r_min[i]+this->coarse_bin_size[i]*indexes[i]);
    }
}

void 
MDDensityHomogeneous::findFineIndexes(uint64_t ind, std::vector<uint64_t> &fine_ind)const
{
   if(ind>=fine_grid_size){
       throw(std::invalid_argument("fine index out of range"));
   }

    uint64_t rest(ind);
    for(int i=nDims-1;i>0;i--){
        uint64_t i1  =  rest/fine_bin_stride[i];
        fine_ind[i]  =  i1;
        rest        -=  i1*fine_bin_stride[i];
    }
    fine_ind[0]=rest;
}
void 
MDDensityHomogeneous::findCoarseIndexes(size_t ind,std::vector<size_t> &coarse_ind)const
{
    if(ind>=coarse_grid_size){
       throw(std::invalid_argument("coarse index out of range"));
   }


    size_t rest(ind);
    for(int i=nDims-1;i>0;i--){
        if(coarse_bin_stride[i]==0){
            coarse_ind[i] =  0;
            continue;
        }
        size_t i1   =  rest/coarse_bin_stride[i];
        coarse_ind[i] =  i1;
        rest         -=  i1*coarse_bin_stride[i];
    }
    coarse_ind[0]    = rest;

}

uint64_t
MDDensityHomogeneous::getCellPixCoordinates(size_t ind, std::vector<MDDPoint_t> &coord)const
{

    std::vector<size_t> coarse_ind(this->nDims,0);
    this->findCoarseIndexes(ind,coarse_ind);

 
    std::vector<uint64_t> fine_ind_start(nDims,0);
    std::vector<uint64_t> fine_ind_end(nDims,0);
    std::vector<uint64_t> fine_ind(nDims,0);
    std::vector<MDDPoint_t> rMax(nDims,0);

    // this is always right for orthogonal dimesnions;
    uint64_t cap_check(1);
    float rMinCell,rMaxCell;

    uint64_t fs,fe;
    for(unsigned int i=0;i<nDims;i++){
        // indexes of large cell:
        rMinCell=(float)(this->r_min[i]+(coarse_ind[i]  )*this->coarse_bin_size[i]);
        rMaxCell=(float)(this->r_min[i]+(coarse_ind[i]+1)*this->coarse_bin_size[i]);        

        fs=(uint64_t)((rMinCell-r_min[i])/fine_bin_size[i]); if(float(r_min[i]+fs*fine_bin_size[i])< rMinCell)fs++;
        fe=(uint64_t)((rMaxCell-r_min[i])/fine_bin_size[i]); if(float(r_min[i]+fe*fine_bin_size[i])>=rMaxCell)fe--;
              
        // loop used below goes to the last index which is one smaller then final index fine_ind_end;
        fe++;
        cap_check*=(fe-fs);
        // starting loop from start index
        fine_ind[i]     = fs;
        fine_ind_start[i]=fs;
        fine_ind_end[i]  =fe;
        rMax[i]          =rMaxCell; 

    }
    coord.resize(size_t(cap_check*nDims));

 
    uint64_t nCells(0);
    unsigned int id;

    // loop through all MD indexes to fill each MD-point with proper coordinates within the macro-cell;
    bool index_correct(true);
    while(index_correct){
        // MD vector filled in loop over dimensions; 
        for(id=0;id<nDims;id++){
            float r_id = (float)(this->r_min[id]+fine_ind[id]*fine_bin_size[id]);
            if(r_id>=rMax[id]){ // leftmost boundary point does not belong to this cell;
                if(nCells>0)nCells--; // clear this cell
                continue;
            }            
            coord[size_t(nCells*nDims+id)]=r_id;
        }
        // increment MD index by one;
        index_correct=ind_plus(fine_ind_start,fine_ind_end,fine_ind);
        nCells++;
    }
  
    return nCells;
}
//
uint64_t 
MDDensityHomogeneous::coarseCellCapacity(const std::vector<size_t> &coarse_ind)const
{
    float rMinCell,rMaxCell;
    uint64_t fs,fe;
 
    // calculates maximal capasity; real capacity may be a bit smaller (especially for non-axis-alighned cells)
    // all cells have equal size; Done this way to deal with randomisation errors properly
  
     uint64_t csCapacity = 1;
     for(unsigned int i=0;i<this->nDims;i++){
        // indexes of large cell:
          rMinCell=(float)(this->r_min[i]+(coarse_ind[i]  )*this->coarse_bin_size[i]);
          rMaxCell=(float)(this->r_min[i]+(coarse_ind[i]+1)*this->coarse_bin_size[i]);

          fs=(uint64_t)((rMinCell-r_min[i])/fine_bin_size[i]);  if(float(r_min[i]+fs*fine_bin_size[i])< rMinCell)fs++;
          fe=(uint64_t)((rMaxCell-r_min[i])/fine_bin_size[i]);  if(float(r_min[i]+fe*fine_bin_size[i])>=rMaxCell)fe--;
        
           csCapacity*=(fe-fs+1); // start and end belong to the cell, so +1
     }
      
    return csCapacity;
}
uint64_t 
MDDensityHomogeneous::coarseCellCapacity(size_t cell_num)const 
{
    std::vector<size_t> coarse_ind(this->nDims,0);
    this->findCoarseIndexes(cell_num,coarse_ind);

    return coarseCellCapacity(coarse_ind);
}
// 
bool 
MDDensityHomogeneous::ind_plus(const std::vector<uint64_t> &ind_min,const std::vector<uint64_t> &ind_max, std::vector<uint64_t> &ind)const
{
    int id;
    for(id=0;id<this->nDims;id++){
        if(ind[id]< ind_max[id]-1){
            ind[id]++;
            return true;
        }else{
           ind[id]=ind_min[id];
        }
    }

    for(id=0;id<this->nDims;id++)ind[id]=ind_max[id];    
    return false;
}
void 
MDDensityHomogeneous::getMDImageCellData(size_t index,double &Signal,double &Error,uint64_t &nPixels)const
{
    nPixels=this->coarseCellCapacity(index);
    Signal =double(nPixels);
    Error = 0.5/double(nPixels);
  
}
//****************************************************************************************************

MDPeakData::MDPeakData(double sigmaSq,const Geometry::MDGeometryDescription &geomDescr):
MDDensityHomogeneous(geomDescr),
SigmaSq(sigmaSq)
{

    this->nRecDim = geomDescr.getNumRecDims();
}
//
//void 
//MDPeakData::getMDImageCellData(size_t index,double &Signal,double &Error,uint64_t &nPixels)const
//{
//    // temporary
//    MDDensityHomogeneous::getMDImageCellData(index,Signal,Error,nPixels);
//}
//void 
//MDPeakData::getMDDPointData(size_t cell_index,char *pBuffer,size_t &nDataPoints)const
//{
//    // temporary;
//    MDDensityHomogeneous::getMDDPointData(cell_index,pBuffer,nDataPoints);
//}

} // endMDDataObjects namespace
} 