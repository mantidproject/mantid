#include "MantidMDAlgorithms/CpRebinningNx3.h"
#include <boost/ptr_container/ptr_vector.hpp>
// temporary -- to provide size of sqw pixel
#include "MDDataObjects/MD_File_hdfMatlab.h"

namespace Mantid{
namespace MDAlgorithms{

     using namespace Mantid;
     using namespace MDDataObjects;
     using namespace Kernel;
     using namespace API;
     using namespace Geometry;


CpRebinningNx3::CpRebinningNx3(const MDDataObjects::MDWorkspace_const_sptr &sourceWS, 
                 Geometry::MDGeometryDescription const * const pTargetDescr,
                 const MDDataObjects::MDWorkspace_sptr  & targetWS,bool in_keep_pixels):
DynamicCPRRebinning(sourceWS,pTargetDescr,targetWS),
nRecDims(3), // can not do anything else here
keep_pixels(in_keep_pixels),
n_starting_cell(0),
n_pixels_read(0),
n_pixels_selected(0),
n_pix_in_buffer(0),
pTargetDataPoints(targetWS->get_spMDDPoints().get())
{

     this->build_scaled_transformation_matrix(sourceWS->get_const_MDGeometry(),*pTargetDescr);
 
	 pix_buf = this->pSourceDataPoints->get_pBuffer();

     if(keep_pixels){
         // not necessary; will clear things out
         //out_pix = this->pTargetDataPoints->getBuffer();
         // get number of pixels which fit the buffer
         size_t n_pix = (*pix_buf).size()/this->pSourceDataPoints->sizeofMDDataPoint();
         retained_cell_indexes.resize(n_pix);
         pixel_valid.resize(n_pix,false);
		 // initialize target points to treat current target image as keys to pixels locations
		 this->pTargetWS->get_spMDDPoints()->initialize(this->spTargetImage);

     }

}
bool
CpRebinningNx3::rebin_data_chunk()
{
   time_t start,end;
   time(&start);  //*******
   n_starting_cell  = this->pSourceDataPoints->get_pix_subset(preselected_cells,n_starting_cell,*pix_buf,n_pix_in_buffer);
   n_pixels_read   += n_pix_in_buffer;

   time(&end);   
   bin_log.debug()<<" data obtained in: "<<difftime (end,start)<<" sec\n";
   std::cout<<" data obtained in: "<<difftime (end,start)<<" sec\n";
   time(&start);  //*******
  
    n_pixels_selected+= rebin_Nx3dataset();
    time(&end);   
    bin_log.debug()<<" cells rebinned in: "<<difftime (end,start)<<" sec\n";;
	std::cout<<" pixels and cells rebinned in: "<<difftime (end,start)<<" sec\n";

    if(n_starting_cell==preselected_cells.size()){
          return false; // no more data left to process
    }
      // more data are still availible
    return true;

}
bool
CpRebinningNx3::rebin_data_chunk_keep_pixels()
{
    size_t  n_pixels_retained_now;
    time_t start,end;
    time(&start);  //*******
 
    n_starting_cell  = this->pSourceDataPoints->get_pix_subset(preselected_cells,n_starting_cell,*pix_buf,n_pix_in_buffer);
    n_pixels_read   += n_pix_in_buffer;
    time(&end);   
    bin_log.debug()<<" data obtained in: "<<difftime (end,start)<<" sec\n";
    time(&start);  //*******


    n_pixels_retained_now  =  this->rebin_Nx3dataset();
    n_pixels_selected     += n_pixels_retained_now;
    time(&end);   
    bin_log.debug()<<" pixels and cells rebinned in: "<<difftime (end,start)<<" sec\n";
    time(&start);  //*******
    this->pTargetDataPoints->store_pixels(*pix_buf,pixel_valid,retained_cell_indexes,n_pixels_retained_now);
    time(&end);   
    bin_log.debug()<<" pixels and cells stored in: "<<difftime (end,start)<<" sec\n";
	

    if(n_starting_cell==preselected_cells.size()){
          return false; // no more data left to process
    }
      // more data are still availible
    return true;

}
//
unsigned int 
CpRebinningNx3::getNumDataChunks()const
{
    size_t pix_buffer_size = this->pSourceDataPoints->get_pix_bufSize();
    size_t n_data_chunks  = (size_t)this->n_preselected_pix/pix_buffer_size;

    if(n_data_chunks*pix_buffer_size!=n_preselected_pix)n_data_chunks++;
    return n_data_chunks;
}
//
CpRebinningNx3::~CpRebinningNx3()
{
}
//
void 
CpRebinningNx3::build_scaled_transformation_matrix(const Geometry::MDGeometry &Source,const Geometry::MDGeometryDescription &target)
{
  unsigned int  i,ic,j;
  this->n_starting_cell = 0;
  this->n_pixels_read   = 0;
  this->n_pixels_selected=0;
  this->n_pix_in_buffer  = 0;
  this->nDimensions     = Source.getNumDims();
  this->shifts.resize(this->nDimensions);
  this->cut_min.resize(this->nDimensions);
  this->cut_max.resize(this->nDimensions);
  this->axis_step.resize(this->nDimensions);
  this->axis_step_inv.resize(this->nDimensions);
  this->strides.resize(this->nDimensions);
  this->rec_dim_indexes.assign(3,-1);


  // reduction dimensions; if stride = 0, the dimension is reduced;
   /// order of dimensions in sparce dimension array is different from the order, obtained in dimensions
  std::vector<std::string> source_dimID = this->pSourceWS->get_const_MDDPoints().getDimensionsID();
  IMDDimension const*  pmDim;
  unsigned int rec_dim_count(0);
  for(i=0;i<this->nDimensions;i++){
      pmDim             = (this->pTargetGeom->get_constDimension(source_dimID[i])).get();

      shifts[i]        =  pmDim->getDataShift();
      axis_step[i]     = (pmDim->getMaximum()-pmDim->getMinimum())/pmDim->getNBins();
      axis_step_inv[i] = 1/axis_step[i];
      cut_max[i]       = pmDim->getMaximum()*axis_step_inv[i];
      cut_min[i]       = pmDim->getMinimum()*axis_step_inv[i];
      strides[i]       = pmDim->getStride();
      if(pmDim->isReciprocal()){
          this->rec_dim_indexes[rec_dim_count]=i;
          rec_dim_count++;
      }
 
  }
  std::vector<double> rot = target.getRotations();
  //std::vector<double> basis[3]; // not used at the momemnt;

  for(i=0;i<3;i++){
    ic = i*3;
    for(j=0;j<3;j++){
      rotations[ic+j]=rot[ic+j]*axis_step_inv[i];
    }
  }


}

size_t 
CpRebinningNx3::rebin_Nx3dataset()
{
  // set up auxiliary variables and preprocess them.
  double xt,yt,zt,xt1,yt1,zt1,Et,Inf(0),
      pix_Xmin,pix_Ymin,pix_Zmin,pix_Emin,pix_Xmax,pix_Ymax,pix_Zmax,pix_Emax;
  size_t nPixel_retained(0);


 
  std::vector<double> boxMin(this->nDimensions,FLT_MAX);
  std::vector<double> boxMax(this->nDimensions,-FLT_MAX);
  std::vector<double> rN(this->nDimensions,0);
  
  bool  ignore_something,ignote_all;

  ignore_something=ignore_nan|ignore_inf;
  ignote_all      =ignore_nan&ignore_inf;
  if(ignore_inf){
    Inf=std::numeric_limits<double>::infinity();
  }
 

  //bool keep_pixels(false);

  //int nRealThreads;

  size_t i,indl;
  int    indX,indY,indZ,indE;
  double s,err;
  size_t nDimX(strides[this->rec_dim_indexes[0]]),nDimY(strides[this->rec_dim_indexes[1]]),nDimZ(strides[this->rec_dim_indexes[2]]);
  // this one should coinside with the description, obtained from the MDDataPoints and readers;
  float *MDDataPoint =(float *)(&(pix_buf->operator[](0)));
  unsigned int signal_shift = nDimensions;
  unsigned int data_stride  = this->pSourceDataPoints->sizeofMDDataPoint()/sizeof(float);
  // min-max value initialization

  pix_Xmin=pix_Ymin=pix_Zmin=pix_Emin=  std::numeric_limits<double>::max();
  pix_Xmax=pix_Ymax=pix_Zmax=pix_Emax=- std::numeric_limits<double>::max();
  //size_t nCells  = this->n_target_cells;
  //
  // work at least for MSV 2008
// The following code does not work cross platform. Hence the undef.
#undef _OPENMP
#ifdef _OPENMP
  int num_OMP_Threads(1);
  omp_set_num_threads(num_OMP_Threads);


#pragma omp parallel default(none), private(i,j0,xt,yt,zt,xt1,yt1,zt1,Et,indX,indY,indZ,indE), \
    shared(actual_pix_range,this->pix,ok,ind, \
        this->nPixels,newsqw), \
        firstprivate(pix_Xmin,pix_Ymin,pix_Zmin,pix_Emin, pix_Xmax,pix_Ymax,pix_Zmax,pix_Emax,\
            ignote_all,ignore_nan,ignore_inf,ignore_something,transform_energy,
  ebin_inv,Inf,trf,\
  nDimX,nDimY,nDimZ,nDimE), \
  reduction(+:nPixel_retained)
#endif
  {
    //	#pragma omp master
    //{
    //    nRealThreads= omp_get_num_threads()
    //	 mexPrintf(" n real threads %d :\n",nRealThread);}

//#pragma omp for schedule(static,1)
    for(i=0;i<n_pix_in_buffer;i++){
      size_t base = i*data_stride;

      s  = *(MDDataPoint+base+signal_shift); //unPacker.getSignal(i);
      err= *(MDDataPoint+base+signal_shift+1); // //unPacker.getError(i);
      // Check for the case when either data.s or data.e contain NaNs or Infs, but data.npix is not zero.
      // and handle according to options settings.
      if(ignore_something){
        if(ignote_all){
          if(s==Inf||isNaN(s)||
              err==Inf ||isNaN(err)){
            continue;
          }
        }else if(ignore_nan){
          if(isNaN(s)||isNaN(err)){
            continue;
          }
        }else if(ignore_inf){
          if(s==Inf||err==Inf){
            continue;
          }
        }
      }
      indl=0;
      bool out(false);
      for(size_t j=nRecDims; j<nDimensions; j++)
      {
        // transform orthogonal dimensions
        Et=(*(MDDataPoint+base+j)-shifts[j])*axis_step_inv[j];

        if(Et<cut_min[j]||Et>=cut_max[j]){
          out = true;
          continue;
        }else{
          indE=(unsigned int)floor(Et-cut_min[j]);
        }
        indl+=indE*strides[j];
        rN[j]=Et;
      }
      if(out)continue;


      // Transform the coordinates u1-u4 into the new projection axes, if necessary
      //    indx=[(v(1:3,:)'-repmat(trans_bott_left',[size(v,2),1]))*rot_ustep',v(4,:)'];  % nx4 matrix
      xt1=*(MDDataPoint+base+0)   -shifts[0]; // unPacker.getDataField(0,i)   -shifts[0];
      yt1=*(MDDataPoint+base+1)   -shifts[1]; //unPacker.getDataField(1,i)   -shifts[1];
      zt1=*(MDDataPoint+base+2)   -shifts[2]; //unPacker.getDataField(2,i)   -shifts[2];


      xt=xt1*rotations[0]+yt1*rotations[3]+zt1*rotations[6];
      if(xt<cut_min[0]||xt>=cut_max[0]){
        continue;
      }

      yt=xt1*rotations[1]+yt1*rotations[4]+zt1*rotations[7];
      if(yt<cut_min[1]||yt>=cut_max[1]){
        continue;
      }

      zt=xt1*rotations[2]+yt1*rotations[5]+zt1*rotations[8];
      if(zt<cut_min[2]||zt>=cut_max[2]) {
        continue;
      }

      //     indx=indx(ok,:);    % get good indices (including integration axes and plot axes with only one bin)
      indX=(int)floor(xt-cut_min[0]);
      indY=(int)floor(yt-cut_min[1]);
      indZ=(int)floor(zt-cut_min[2]);

      //
      indl += indX*nDimX+indY*nDimY+indZ*nDimZ;
      if(keep_pixels){
          pixel_valid[i] = true;
          retained_cell_indexes[nPixel_retained] = indl;
      }
      nPixel_retained++;
	  // DEBUGGING:
	  //if(indl>=nCells){
		 // continue;
	  //}
      // i0=nPixel_retained*OUT_PIXEL_DATA_WIDTH;    // transformed pixels;
//#pragma omp atomic
      pTargetImgData[indl].s   +=s;
//#pragma omp atomic
      pTargetImgData[indl].err +=err;
//#pragma omp atomic
      pTargetImgData[indl].npix++;
//#pragma omp atomic
      // this request substantial thinking -- will not do it this way as it is very slow
      // this->pix_array[indl].cell_memPixels.push_back(pix);

      //
      //    actual_pix_range = [min(actual_pix_range(1,:),min(indx,[],1));max(actual_pix_range(2,:),max(indx,[],1))];  % true range of data
      if(xt<pix_Xmin)pix_Xmin=xt;
      if(xt>pix_Xmax)pix_Xmax=xt;

      if(yt<pix_Ymin)pix_Ymin=yt;
      if(yt>pix_Ymax)pix_Ymax=yt;

      if(zt<pix_Zmin)pix_Zmin=zt;
      if(zt>pix_Zmax)pix_Zmax=zt;


    } // end for i -- imlicit barrier;
#pragma omp critical
    {
      if(boxMin[0]>pix_Xmin*axis_step[0])boxMin[0]=pix_Xmin*axis_step[0];
      if(boxMin[1]>pix_Ymin*axis_step[1])boxMin[1]=pix_Ymin*axis_step[1];
      if(boxMin[2]>pix_Zmin*axis_step[2])boxMin[2]=pix_Zmin*axis_step[2];


      if(boxMax[0]<pix_Xmax*axis_step[0])boxMax[0]=pix_Xmax*axis_step[0];
      if(boxMax[1]<pix_Ymax*axis_step[1])boxMax[1]=pix_Ymax*axis_step[1];
      if(boxMax[2]<pix_Zmax*axis_step[2])boxMax[2]=pix_Zmax*axis_step[2];

      for(size_t j=nRecDims;j<nDimensions;j++)
      {
        if(boxMin[j]>rN[j]*axis_step_inv[j])boxMin[j]=rN[j]*axis_step_inv[j];
        if(boxMax[j]<rN[j]*axis_step_inv[j])boxMax[j]=rN[j]*axis_step_inv[j];
      }
    }
  } // end parallel region
 
  for(unsigned int ii=0;ii<nDimensions;ii++){
    pTargetDataPoints->rPixMin(ii) = (pTargetDataPoints->rPixMin(ii)<boxMin[ii])?pTargetDataPoints->rPixMin(ii):boxMin[ii];
    pTargetDataPoints->rPixMax(ii) = (pTargetDataPoints->rPixMax(ii)>boxMax[ii])?pTargetDataPoints->rPixMax(ii):boxMax[ii];
  }
  // keep image array properly defined 
  this->pTargetWS->get_spMDImage()->get_pMDImgData()->npixSum+=nPixel_retained;
  //
  return nPixel_retained;
}


} // end namespaces
}
