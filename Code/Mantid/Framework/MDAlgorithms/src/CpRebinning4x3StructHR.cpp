#include "MantidMDAlgorithms/CpRebinning4x3StructHR.h"
#include "MDDataObjects/MD_File_hdfMatlab4D.h"

namespace Mantid{
namespace MDAlgorithms{

     using namespace Mantid;
     using namespace MDDataObjects;
     using namespace Kernel;
     using namespace API;
     using namespace Geometry;

CpRebinning4x3StructHR::CpRebinning4x3StructHR(const MDDataObjects::MDWorkspace_const_sptr &sourceWS, 
                 Geometry::MDGeometryDescription const * const pTargetDescr,
                 const MDDataObjects::MDWorkspace_sptr  & targetWS,bool keep_pixels):
CpRebinningNx3(sourceWS,pTargetDescr,targetWS,keep_pixels)
{
    if(boost::dynamic_pointer_cast<MD_File_hdfMatlab4D>(this->pSourceDataPoints->getFileReader())==0){
        this->bin_log.error()<<" CpRebinning4x3StructHR can not work with any reader except MD_File_hdfMatlab4D\n";
        throw(std::invalid_argument("Wrong data reader for this kind of rebinning"));
    }

   //Provide data buffer for input rebinning data
    size_t pix_buf_size = this->pix_buf.size();
    pix_buf.resize(pix_buf_size*sizeof(sqw_pixel));
}
//
bool
CpRebinning4x3StructHR::rebin_data_chunk()
{

    n_starting_cell  = this->pSourceDataPoints->get_pix_subset(preselected_cells,n_starting_cell,pix_buf,n_pix_in_buffer);
    n_pixels_read   += n_pix_in_buffer;
      
    n_pixels_selected+= rebin_4x3struct_dataset();
    if(n_starting_cell==preselected_cells.size()){
         return false; // no more data left to process
    }
    // more data are still availible
    return true;

}
//
size_t 
CpRebinning4x3StructHR::rebin_4x3struct_dataset()
{
  // set up auxiliary variables and preprocess them.
  double xt,yt,zt,xt1,yt1,zt1,Et,Inf(0),
      pix_Xmin,pix_Ymin,pix_Zmin,pix_Emin,pix_Xmax,pix_Ymax,pix_Zmax,pix_Emax;
  size_t nPixel_retained(0);


 
  std::vector<double> boxMin(this->nDimensions,0);
  std::vector<double> boxMax(this->nDimensions,0);
  std::vector<double> rN(this->nDimensions,0);
  
  bool  ignore_something,ignote_all;

  ignore_something=ignore_nan|ignore_inf;
  ignote_all      =ignore_nan&ignore_inf;
  if(ignore_inf){
    Inf=std::numeric_limits<double>::infinity();
  }
 
  for(unsigned int ii=0;ii<this->nDimensions;ii++){
    boxMin[ii]       =  FLT_MAX;
    boxMax[ii]       = -FLT_MAX;
  }

  //bool keep_pixels(false);

  //int nRealThreads;

  size_t i,indl;
  int    indX,indY,indZ,indE;
//  double s,err;
  size_t nDimX(strides[this->rec_dim_indexes[0]]),
         nDimY(strides[this->rec_dim_indexes[1]]),
         nDimZ(strides[this->rec_dim_indexes[2]]),
         nDimE(strides[3]);
 
  pix_Xmin=pix_Ymin=pix_Zmin=pix_Emin=  std::numeric_limits<double>::max();
  pix_Xmax=pix_Ymax=pix_Zmax=pix_Emax=- std::numeric_limits<double>::max();
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
   for(i=0;i<n_pix_in_buffer;i++)
    {
      sqw_pixel pix=*((sqw_pixel*)(&pix_buf[0])+i);

      // Check for the case when either data.s or data.e contain NaNs or Infs, but data.npix is not zero.
      // and handle according to options settings.
      if(ignore_something){
        if(ignote_all){
          if(pix.s==Inf||isNaN(pix.s)||
              pix.err==Inf ||isNaN(pix.err)){
            continue;
          }
        }else if(ignore_nan){
          if(isNaN(pix.s)||isNaN(pix.err)){
            continue;
          }
        }else if(ignore_inf){
          if(pix.s==Inf||pix.err==Inf){
            continue;
          }
        }
      }

      // Transform the coordinates u1-u4 into the new projection axes, if necessary
      //    indx=[(v(1:3,:)'-repmat(trans_bott_left',[size(v,2),1]))*rot_ustep',v(4,:)'];  % nx4 matrix
      xt1=pix.qx    -shifts[0];
      yt1=pix.qy    -shifts[1];
      zt1=pix.qz    -shifts[2];

      // transform energy
      Et=(pix.En    -shifts[3])*axis_step_inv[3];

      //  ok = indx(:,1)>=cut_range(1,1) & indx(:,1)<=cut_range(2,1) & indx(:,2)>=cut_range(1,2) & indx(:,2)<=urange_step(2,2) & ...
      //       indx(:,3)>=cut_range(1,3) & indx(:,3)<=cut_range(2,3) & indx(:,4)>=cut_range(1,4) & indx(:,4)<=cut_range(2,4);
      if(Et<cut_min[3]||Et>=cut_max[3]){
        continue;
      }

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
      nPixel_retained++;



      //     indx=indx(ok,:);    % get good indices (including integration axes and plot axes with only one bin)
      indX=(int)floor(xt-cut_min[0]);
      indY=(int)floor(yt-cut_min[1]);
      indZ=(int)floor(zt-cut_min[2]);
      indE=(int)floor(Et-cut_min[3]);
      //
      indl  = indX*nDimX+indY*nDimY+indZ*nDimZ+indE*nDimE;
      // i0=nPixel_retained*OUT_PIXEL_DATA_WIDTH;    // transformed pixels;
//#pragma omp atomic
      pTargetImgData[indl].s   +=pix.s;
//#pragma omp atomic
      pTargetImgData[indl].err +=pix.err;
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

      if(Et<pix_Emin)pix_Emin=Et;
      if(Et>pix_Emax)pix_Emax=Et;

    } // end for i -- imlicit barrier;
#pragma omp critical
    {
      if(boxMin[0]>pix_Xmin*axis_step[0])boxMin[0]=pix_Xmin*axis_step[0];
      if(boxMin[1]>pix_Ymin*axis_step[1])boxMin[1]=pix_Ymin*axis_step[1];
      if(boxMin[2]>pix_Zmin*axis_step[2])boxMin[2]=pix_Zmin*axis_step[2];
      if(boxMin[3]>pix_Emin*axis_step[3])boxMin[3]=pix_Emin*axis_step[3];

      if(boxMax[0]<pix_Xmax*axis_step[0])boxMax[0]=pix_Xmax*axis_step[0];
      if(boxMax[1]<pix_Ymax*axis_step[1])boxMax[1]=pix_Ymax*axis_step[1];
      if(boxMax[2]<pix_Zmax*axis_step[2])boxMax[2]=pix_Zmax*axis_step[2];
      if(boxMax[3]<pix_Emax*axis_step[3])boxMax[2]=pix_Emax*axis_step[3];
    }
  } // end parallel region

   //TODO: enable this when MDDataPoints are in working order;
  //for(unsigned int ii=0;ii<nDims;ii++){
  //  TargetWorkspace.get_spMDDPoints()->rPixMin(ii) = boxMin[ii];
  //  TargetWorkspace.get_spMDDPoints()->rPixMax(ii) = boxMax[ii];
  //}

  return nPixel_retained;
}



} // end namespaces
}
