#include "MantidMDAlgorithms/CenterpieceRebinningFunctions.h"
#include <boost/ptr_container/ptr_vector.hpp>
//
namespace Mantid
{
namespace MDAlgorithms
{

using namespace API;
using namespace Geometry;
using namespace MDDataObjects;

//
/*! function calculates min and max values of the array of nPoints points (vertices of hypercube)
 *
 */
void minmax(double &rMin,double &rMax,const std::vector<double> &box)
{
  rMin=box[0];
  rMax=box[0];
  size_t nPoints = box.size();
  for(size_t i=1;i<nPoints ;i++){
    if(box[i]<rMin)rMin=box[i];
    if(box[i]>rMax)rMax=box[i];
  }
}

//*********************************************************************
void preselect_cells(const MDDataObjects::MDWorkspace &Source, const Geometry::MDGeometryDescription &target, std::vector<size_t> &cells_to_select,size_t &n_preselected_pix)
{
  // this algoithm can be substantially enhanced
  // a) by implementing fast search within the limits;
  // b) by selecting the sells which lie entirely within the cut and the sells which can be cut through;
  // c) should we try to use deque to avoid multiple insertions for the same indexes? (correct equalities will eliminate them anyway)
  unsigned int i;
  n_preselected_pix=0;
  //transf_matrix scaled_trf = this->build_scaled_transformation_matrix(Source,target);

  // transform the grid into new system of coordinate and calculate cell indexes, which contribute into the
  // dataset;
  unsigned int j,k,l,mp,mm,sizem;
  double xt1,yt1,zt1,Etm,Etp;
  boost::shared_ptr<IMDDimension> pDim;
  //TO DO: this will be obtained from the taget
  double rotations[9];
  rotations[0]=rotations[1]=rotations[2]=rotations[3]=rotations[4]=rotations[5]=rotations[6]=rotations[7]=rotations[8]=0;
  rotations[0]=rotations[4]=rotations[8]=1;

  // get pointer to the image data;
  const MD_image_point  *const data   = Source.get_const_MDImage().get_const_pData();
  const MDGeometry * const pGeom = Source.getGeometry();

  // evaluate the capacity of the orthogonal dimensions;
  unsigned int  nReciprocal    = pGeom->getNumReciprocalDims();
  unsigned int  nOrthogonal    = pGeom->getNumDims() - nReciprocal;

  std::vector<boost::shared_ptr<IMDDimension> > pAllDim  = pGeom->getDimensions();
  std::vector<boost::shared_ptr<IMDDimension> > pOrthogonal(nOrthogonal);
  std::vector<boost::shared_ptr<IMDDimension> > pReciprocal(nReciprocal);
  unsigned int nr(0),no(0);

  // get the orthogonal and reciprocal dimensions separately

  for(i=0;i<Source.getNumDims();i++){
    if(pAllDim[i]->isReciprocal()){
      pReciprocal[nr]=pAllDim[i];
      nr++;
    }else{
      pOrthogonal[no]=pAllDim[i];
      no++;
    }
  }

  size_t ind,orthoSize=1;

  size_t stride;
  int  nContributed(0);
  // this is the array of vectors to keep orthogonal indexes;
  std::vector< std::vector<size_t> > enInd(nOrthogonal,std::vector<size_t>(0,0));

  // estimate cut limits in orthogonal dimensions
  std::vector<double> ort_cut_min(nOrthogonal,0),ort_cut_max(nOrthogonal,0);
  int dim_num;
  for(i=0;i<nOrthogonal;i++){
    dim_num = target.getTagNum(pOrthogonal[i]->getDimensionId(),true);
	ort_cut_min[i]=target.dimDescription(dim_num).cut_min;
    ort_cut_max[i]=target.dimDescription(dim_num).cut_max;
  }

  for(i=0;i<nOrthogonal;i++){
    nContributed=0;
    pDim   = pOrthogonal[i];
    sizem  = pDim->getNBins();
    stride = pDim->getStride();
    for(mm=0;mm<sizem;mm++){
      // is rightmpst for min or leftmost for max in range?
      mp=mm+1;
      // check if it can be in ranges
      Etp=pDim->getX(mp);
      Etm=pDim->getX(mm);

      if(Etp<ort_cut_min[i]||Etm>ort_cut_max[i]) continue;
      // remember the index of THIS axis
      enInd[i].push_back(mm*stride);
      // increase the counter of the cells, contributed into cut
      nContributed++;
    }
    orthoSize*=nContributed;
    if(nContributed==0){  // no cells contribute into the cut; Return
      return;
    }

  }
  // multiply all orthogonal vectors providing linear but size(en)*size(ga1)*size(ga2)*size(ga3)*... matrix of indexes;
  std::vector<size_t> orthoInd(orthoSize,0);
  size_t ic(0);
  for(i=0;i<enInd[0].size();i++){
    orthoInd[ic]=enInd[0].at(i);
    ic++;
  }
  for(l=1;l<nOrthogonal;l++){
    size_t orthSize=orthoInd.size();
    for(j=0;j<orthSize;j++){
      size_t indDim0=orthoInd.at(j);
      for(i=0;enInd[l].size();i++){
        orthoInd[ic]=indDim0+enInd[l].at(i);
        ic++;
      }
    }
  }
  enInd.clear();
  std::vector<boost::shared_ptr<IMDDimension> > rec_dim;
  rec_dim.resize(3);
  // evaluate the capacity of the real space (3D or less);
  // Define (1<=N<=3)D subspace and transform it into the coordinate system of the new cut;
  size_t size3D(1);
  std::vector<double> cut_min(3,0);
  std::vector<double> cut_max(3,0);
  for(i=0;i<nReciprocal;i++){
    size3D    *= (pReciprocal[i]->getNBins()+1);
    rec_dim[i] = pReciprocal[i];

    dim_num    = target.getTagNum(rec_dim[i]->getDimensionId(),true);
	cut_min[i] = target.dimDescription(dim_num).cut_min;
    cut_max[i] = target.dimDescription(dim_num).cut_max;
  }
  // if there are less then 3 reciprocal dimensions, lets make 3 now to make the algorithm generic
  for(i=nReciprocal;i<3;i++){
    rec_dim[i] =  boost::shared_ptr<MDDimension>(new MDDimDummy(i));
    // we should know the limits the dummy dimensions have
    cut_min[i] = rec_dim[i]->getMinimum();
	//TODO: Verify
    cut_max[i] = rec_dim[i]->getMaximum()*(1+FLT_EPSILON);

    //TO DO: deal with rotations in case they have not been dealt with before;
    //  for(j=nReciprocal;j<3;j++){
    //    rotations[3*i+j]=0;
    //    rotations[3*j+i]=0;
    // }
    // rotations[3*i+i] = 1;
  }

  std::vector<double> rx,ry,rz,xx,yy,zz;
  rx.assign(size3D,0); xx.assign(size3D,0);
  ry.assign(size3D,0); yy.assign(size3D,0);
  rz.assign(size3D,0); zz.assign(size3D,0);
  // nAxis points equal nBins+1;
  // lattice points transformed into new coordinate system.
  // needed modifications for nRecDim<3
  ic = 0;
  for(k=0;k<=rec_dim[2]->getNBins();k++){
    for(j=0;j<=rec_dim[1]->getNBins();j++){
      for(i=0;i<=rec_dim[0]->getNBins();i++){
        rx[ic]=rec_dim[0]->getX(i);
        ry[ic]=rec_dim[1]->getX(j);
        rz[ic]=rec_dim[2]->getX(k);
        ic++;
      }
    }
  }
  for(i=0;i<size3D;i++){
    xt1=rx[i];yt1=ry[i];zt1=rz[i];

    xx[i]=xt1*rotations[0]+yt1*rotations[3]+zt1*rotations[6];
    yy[i]=xt1*rotations[1]+yt1*rotations[4]+zt1*rotations[7];
    zz[i]=xt1*rotations[2]+yt1*rotations[5]+zt1*rotations[8];
  }
  rx.clear();   ry.clear();   rz.clear();
  unsigned int ip,jp,kp;
  nCell3D sh(rec_dim[0]->getNBins()+1,rec_dim[1]->getNBins()+1);
  //           ind3D(this->dim_sizes[u1],this->dim_sizes[u2]);
  double rMin,rMax;
  std::vector<double> r(8,0);
  size_t ind3;

  for(k=0;k <rec_dim[2]->getNBins();k++){
    kp=k+1;
    for(j=0;j<rec_dim[1]->getNBins();j++){
      jp=j+1;

      for(i=0;i<rec_dim[0]->getNBins();i++){
        ip=i+1;

        r[0]=xx[sh.nCell(i,j,k )]; r[1]=xx[sh.nCell(ip,j ,k )]; r[2]=xx[sh.nCell(i ,jp,k )]; r[3]=xx[sh.nCell(ip,jp,k )];
        r[4]=xx[sh.nCell(i,j,kp)]; r[5]=xx[sh.nCell(ip,j ,kp)]; r[6]=xx[sh.nCell(i ,jp,kp)]; r[7]=xx[sh.nCell(ip,jp,kp)];

        minmax(rMin,rMax,r);
        // unlike the cut over point, we select cells with points on the upper boundary
        if(rMax<cut_min[0]||rMin>cut_max[0])continue;

        r[0]=yy[sh.nCell(i ,j ,k )];  r[1]=yy[sh.nCell(ip,j ,k )];r[2]=yy[sh.nCell(i ,jp,k )];  r[3]=yy[sh.nCell(ip,jp,k )];
        r[4]=yy[sh.nCell(i ,j ,kp)];  r[5]=yy[sh.nCell(ip,j ,kp)];r[6]=yy[sh.nCell(i ,jp,kp)];  r[7]=yy[sh.nCell(ip,jp,kp)];

        minmax(rMin,rMax,r);
        if(rMax<cut_min[1]||rMin>cut_max[1])continue;

        r[0]=zz[sh.nCell(i,j,k )];  r[1]=zz[sh.nCell(ip,j,k )];r[2]=zz[sh.nCell(i ,jp,k )];  r[3]=zz[sh.nCell(ip,jp,k )];
        r[4]=zz[sh.nCell(i,j,kp)];  r[5]=zz[sh.nCell(ip,j,kp)];r[6]=zz[sh.nCell(i ,jp,kp)];  r[7]=zz[sh.nCell(ip,jp,kp)];

        minmax(rMin,rMax,r);
        if(rMax<cut_min[2]||rMin>cut_max[2])continue;

        ind3=i*rec_dim[0]->getStride()+j*rec_dim[1]->getStride()+k*rec_dim[2]->getStride();
        // multiply reciprocal indexes by orthogonal indexes and srore all indexes for selection
        for(l=0;l<orthoInd.size();l++){
          ind = ind3+orthoInd.at(l);
          if(data[ind].npix>0){
            cells_to_select.push_back(ind);
            n_preselected_pix+=data[ind].npix;
          }
        }


      }
    }
  }
  orthoInd.clear();
}

transf_matrix 
build_scaled_transformation_matrix(const Geometry::MDGeometry &Source,const Geometry::MDGeometryDescription &target,bool ignoreNaN,bool ignoreInf)
{

  transf_matrix trf;


  unsigned int  i,ic,j;
  unsigned int nDims = Source.getNumDims();
  trf.ignore_NaN     = ignoreNaN;
  trf.ignore_Inf     = ignoreInf;
  trf.nDimensions    = nDims;
  trf.trans_bott_left.assign(nDims,0);
  trf.cut_min.assign(nDims,-1);
  trf.cut_max.assign(nDims,1);
  trf.axis_step.assign(nDims,1);
  //  trf.stride.assign(nDims,0);

  MDDimension *pDim(NULL);
  // for convenience can use dimension accessors from source
  //MDGeometryDescription tSource(Source);

  for(i=0;i<nDims;i++){
	  DimensionDescription dimD = target.dimDescription(i);
	  trf.trans_bott_left[i]=dimD.data_shift;
	  trf.axis_step[i]=(dimD.cut_max-dimD.cut_min)/dimD.nBins;
    trf.cut_max[i]  = dimD.cut_max/trf.axis_step[i];
    trf.cut_min[i]  = dimD.cut_min/trf.axis_step[i];
    //   trf.stride[i]   = target.getStride(i);
  }
  std::vector<double> rot = target.getRotations();
  std::vector<double> basis[3]; // not used at the momemnt;

  for(i=0;i<3;i++){
    ic = i*3;
    for(j=0;j<3;j++){
      trf.rotations[ic+j]=rot[ic+j]/trf.axis_step[i];
    }
  }

  return trf;

}


//
size_t finalise_rebinning(MDDataObjects::MD_image_point *data,size_t data_size)
{
  size_t i;
  // normalize signal and error of the dnd object;
  if(data[0].npix>0){
    data[0].s   /= data[0].npix;
    data[0].err /=(data[0].npix*data[0].npix);
  }
  // and calculate cells location for pixels;
  data[0].chunk_location=0;

  // counter for the number of retatined pixels;
  size_t nPix = data[0].npix;
  for(i=1;i<data_size;i++){
    data[i].chunk_location=data[i-1].chunk_location+data[i-1].npix; // the next cell starts from the the boundary of the previous one
    // plus the number of pixels in the previous cell
    if(data[i].npix>0){
      nPix        +=data[i].npix;
      data[i].s   /=data[i].npix;
      data[i].err /=(data[i].npix*data[i].npix);
    }
  };
  return nPix;
}//***************************************************************************************


size_t rebin_Nx3dataset(const transf_matrix &rescaled_transf, const char *source_pix_buf, size_t nPix, MDDataObjects::MDWorkspace &TargetWorkspace)
{
  // set up auxiliary variables and preprocess them.
  double xt,yt,zt,xt1,yt1,zt1,Et,Inf(0),
      pix_Xmin,pix_Ymin,pix_Zmin,pix_Emin,pix_Xmax,pix_Ymax,pix_Zmax,pix_Emax;
  size_t nPixel_retained(0);


  unsigned int nDims    = TargetWorkspace.getNumDims();
  unsigned int nRecDims = 3;

  std::vector<double> boxMin(nDims,0);
  std::vector<double> boxMax(nDims,0);
  std::vector<double> rN(nDims,0);

  // reduction dimensions; if stride = 0, the dimension is reduced;
  std::vector<boost::shared_ptr<IMDDimension> >  dims = TargetWorkspace.get_const_MDImage().getGeometry()->getDimensions();

  std::vector<size_t> strides(dims.size());
  for(unsigned int i=0;i<dims.size();i++){
	  strides[i]= dims[i]->getStride();
  }

  double rotations_ustep[9];
  std::vector<double> axis_step_inv(nDims,0),shifts(nDims,0),min_limit(nDims,-1),max_limit(nDims,1);
  bool  ignore_something,ignote_all,ignore_nan(rescaled_transf.ignore_NaN),ignore_inf(rescaled_transf.ignore_Inf);

  ignore_something=ignore_nan|ignore_inf;
  ignote_all      =ignore_nan&ignore_inf;
  if(ignore_inf){
    Inf=std::numeric_limits<double>::infinity();
  }
  MD_image_point *data = TargetWorkspace.get_spMDImage()->get_pData();


  for(unsigned int ii=0;ii<nDims;ii++){

    boxMin[ii]       = TargetWorkspace.get_spMDImage()->get_pMDImgData()->min_value[ii];
    boxMax[ii]       = TargetWorkspace.get_spMDImage()->get_pMDImgData()->max_value[ii];

    axis_step_inv[ii]=1/rescaled_transf.axis_step[ii];
    shifts[ii]   =rescaled_transf.trans_bott_left[ii];
    min_limit[ii]=rescaled_transf.cut_min[ii];
    max_limit[ii]=rescaled_transf.cut_max[ii];


  }
  for(int ii=0;ii<9;ii++){
    rotations_ustep[ii]=rescaled_transf.rotations[ii];
  }

  //bool keep_pixels(false);

  //int nRealThreads;

  size_t i,indl;
  int    indX,indY,indZ,indE;
  double s,err;
  size_t nDimX(strides[0]),nDimY(strides[1]),nDimZ(strides[2]);
  // this one should coinside with the description, obtained from the MDDataPoints and readers;
  MDDataObjects::MDDataPoint<float,uint16_t,float> unPacker(const_cast<char *>(source_pix_buf),nDims,2,3);
  // min-max value initialization

  pix_Xmin=pix_Ymin=pix_Zmin=pix_Emin=  std::numeric_limits<double>::max();
  pix_Xmax=pix_Ymax=pix_Zmax=pix_Emax=- std::numeric_limits<double>::max();
  //
  // work at least for MSV 2008
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

#pragma omp for schedule(static,1)
    for(i=0;i<nPix;i++){

      s  = unPacker.getSignal(i);
      err= unPacker.getError(i);
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
      for(size_t j=nRecDims; j<nDims; j++)
      {
        // transform orthogonal dimensions
        Et=(unPacker.getDataField(j,i)-shifts[j])*axis_step_inv[j];

        if(Et<min_limit[j]||Et>=max_limit[j]){
          out = true;
          continue;
        }else{
          indE=(unsigned int)floor(Et-min_limit[3]);
        }
        indl+=indE*strides[j];
        rN[j]=Et;
      }
      if(out)continue;


      // Transform the coordinates u1-u4 into the new projection axes, if necessary
      //    indx=[(v(1:3,:)'-repmat(trans_bott_left',[size(v,2),1]))*rot_ustep',v(4,:)'];  % nx4 matrix
      xt1=unPacker.getDataField(0,i)   -shifts[0];
      yt1=unPacker.getDataField(1,i)   -shifts[1];
      zt1=unPacker.getDataField(2,i)   -shifts[2];


      xt=xt1*rotations_ustep[0]+yt1*rotations_ustep[3]+zt1*rotations_ustep[6];
      if(xt<min_limit[0]||xt>=max_limit[0]){
        continue;
      }

      yt=xt1*rotations_ustep[1]+yt1*rotations_ustep[4]+zt1*rotations_ustep[7];
      if(yt<min_limit[1]||yt>=max_limit[1]){
        continue;
      }

      zt=xt1*rotations_ustep[2]+yt1*rotations_ustep[5]+zt1*rotations_ustep[8];
      if(zt<min_limit[2]||zt>=max_limit[2]) {
        continue;
      }
      nPixel_retained++;



      //     indx=indx(ok,:);    % get good indices (including integration axes and plot axes with only one bin)
      indX=(int)floor(xt-min_limit[0]);
      indY=(int)floor(yt-min_limit[1]);
      indZ=(int)floor(zt-min_limit[2]);

      //
      indl += indX*nDimX+indY*nDimY+indZ*nDimZ;
      // i0=nPixel_retained*OUT_PIXEL_DATA_WIDTH;    // transformed pixels;
#pragma omp atomic
      data[indl].s   +=s;
#pragma omp atomic
      data[indl].err +=err;
#pragma omp atomic
      data[indl].npix++;
#pragma omp atomic
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
      if(boxMin[0]>pix_Xmin/axis_step_inv[0])boxMin[0]=pix_Xmin/axis_step_inv[0];
      if(boxMin[1]>pix_Ymin/axis_step_inv[1])boxMin[1]=pix_Ymin/axis_step_inv[1];
      if(boxMin[2]>pix_Zmin/axis_step_inv[2])boxMin[2]=pix_Zmin/axis_step_inv[2];


      if(boxMax[0]<pix_Xmax/axis_step_inv[0])boxMax[0]=pix_Xmax/axis_step_inv[0];
      if(boxMax[1]<pix_Ymax/axis_step_inv[1])boxMax[1]=pix_Ymax/axis_step_inv[1];
      if(boxMax[2]<pix_Zmax/axis_step_inv[2])boxMax[2]=pix_Zmax/axis_step_inv[2];

      for(size_t j=nRecDims;j<nDims;j++)
      {
        if(boxMin[j]>rN[j]/axis_step_inv[j])boxMin[j]=rN[j]/axis_step_inv[j];
        if(boxMax[j]<rN[j]/axis_step_inv[j])boxMax[j]=rN[j]/axis_step_inv[j];
      }
    }
  } // end parallel region
  //TODO: enable this when MDDataPoints are in working order;
  //for(unsigned int ii=0;ii<nDims;ii++){
  //  TargetWorkspace.get_spMDDPoints()->rPixMin(ii) = boxMin[ii];
  //  TargetWorkspace.get_spMDDPoints()->rPixMax(ii) = boxMax[ii];
  //}

  return nPixel_retained;
}


} //namespace 
}
