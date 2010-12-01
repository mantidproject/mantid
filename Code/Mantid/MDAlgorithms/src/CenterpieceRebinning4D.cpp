#include "MantidMDAlgorithms/CenterpieceRebinning4D.h"

namespace Mantid{
namespace MDAlgorithms{
using namespace Mantid;
using namespace MDDataObjects;
using namespace Kernel;
using namespace API;
using namespace Geometry;


// Register the class into the algorithm factory
DECLARE_ALGORITHM(CenterpieceRebinning4D)

CenterpieceRebinning4D::CenterpieceRebinning4D(void): API::Algorithm(), m_progress(NULL) 
{}

/** Destructor     */
CenterpieceRebinning4D::~CenterpieceRebinning4D()
{
  if( m_progress ){
    delete m_progress;
    m_progress=NULL;
  }
}

void CenterpieceRebinning4D::init_source(MDWorkspace_sptr inputWSX)
{
  MDWorkspace_sptr inputWS;
  // Get the input workspace
  if(existsProperty("Input"))
  {
    inputWS = getProperty("Input");
    if(!inputWS){
      throw(std::runtime_error("input workspace has to exist"));
    }

  }else{
    throw(std::runtime_error("input workspace has to be availible through properties"));
  }

  std::string filename;
  // filename = "../../../../Test/VATES/fe_demo.sqw";

  if(existsProperty("Filename"))
  {
    filename = getPropertyValue("Filename");
  }
  else
  {
    throw(std::runtime_error("filename property can not be found"));
  }

  inputWS->read_mdd();

  // set up slicing property to the shape of current workspace;
  MDGeometryDescription *pSlicing = dynamic_cast< MDGeometryDescription *>((Property *)(this->getProperty("SlicingData")));
  if(!pSlicing){
    throw(std::runtime_error("can not obtain slicing property from the property manager"));
  }

  pSlicing->build_from_geometry(*(inputWS->getGeometry()));
  pSlicing=NULL; // should remain in Property
}
/*
void
CenterpieceRebinning::set_from_VISIT(const std::string &slicing_description,const std::string &XML_definition)
{
    this->slicingProperty.fromXMLstring(slicing_description);
}
 */
void
CenterpieceRebinning4D::init()
{
  declareProperty(new WorkspaceProperty<MDWorkspace>("Input","",Direction::Input),"initial MD workspace");
  declareProperty(new WorkspaceProperty<MDWorkspace>("Result","",Direction::Output),"final MD workspace");

  declareProperty(new MDPropertyGeometry("SlicingData","",Direction::Input));
  declareProperty(new API::FileProperty("Filename","", API::FileProperty::Load), "The file containing input MD dataset");


  m_progress = new Progress(this,0,1,10);


}
//
void 
CenterpieceRebinning4D::exec()
{
  MDWorkspace_sptr inputWS;
  MDWorkspace_sptr outputWS;


  // Get the input workspace
  if(existsProperty("Input")){
    inputWS = getProperty("Input");
    if(!inputWS){
      throw(std::runtime_error("input workspace has to exist"));
    }
  }else{
    throw(std::runtime_error("input workspace has to be accessible through properties"));
  }


  // Now create the output workspace
  if(existsProperty("Result")){

    outputWS = getProperty("Result");
    if(!outputWS){
      outputWS      = MDWorkspace_sptr(new MDWorkspace(4));
      setProperty("Result", outputWS);
    }
  }else{
    throw(std::runtime_error("output workspace has to be created "));
  }
  if(inputWS==outputWS){
    throw(std::runtime_error("input and output workspaces have to be different"));
  }


  MDPropertyGeometry  *pSlicing; 
  if(existsProperty("SlicingData")){ 
    // get slicing data from property manager. These data has to bebeen shaped to proper form .
    pSlicing = dynamic_cast< MDPropertyGeometry *>((Property *)(this->getProperty("SlicingData")));
    if(!pSlicing){
      throw(std::runtime_error("can not obtain slicing property from the property manager"));
    }
  }else{
    throw(std::runtime_error("slising property has to exist and has to be defined "));
  }


  // transform output workspace to the target shape and allocate memory for resulting matrix
  outputWS->alloc_mdd_arrays(*pSlicing);



  std::vector<size_t> preselected_cells_indexes;
  size_t  n_precelected_pixels(0);

  preselect_cells(*inputWS,*pSlicing,preselected_cells_indexes,n_precelected_pixels);
  if(n_precelected_pixels == 0)return;

  unsigned int n_hits = n_precelected_pixels/PIX_BUFFER_SIZE+1;

  size_t    n_pixels_read(0),
      n_pixels_selected(0),
      n_pix_in_buffer(0),pix_buffer_size(PIX_BUFFER_SIZE);

  std::vector<char  > pix_buf;
  pix_buf.resize(PIX_BUFFER_SIZE*sizeof(sqw_pixel));


  // get pointer for data to rebin to; 
  MD_image_point *pImage    = outputWS->get_spMDImage()->get_pData();
  // and the number of elements the image has;
  size_t         image_size=  outputWS->get_const_spMDImage()->getDataSize();
  //
  double boxMin[4],boxMax[4];
  boxMin[0]=boxMin[1]=boxMin[2]=boxMin[3]=FLT_MAX;
  boxMax[0]=boxMax[1]=boxMax[2]=boxMax[3]=FLT_MIN;
  std::vector<size_t> strides = outputWS->get_const_spMDImage()->getStrides();

  transf_matrix trf = build_scaled_transformation_matrix(*(inputWS->getGeometry()),*pSlicing,this->ignore_inf,this->ignore_nan);
  // start reading and rebinning;
  size_t n_starting_cell(0);
  for(unsigned int i=0;i<n_hits;i++){
    n_starting_cell+=inputWS->read_pix_selection(preselected_cells_indexes,n_starting_cell,pix_buf,n_pix_in_buffer);
    n_pixels_read  +=n_pix_in_buffer;

    n_pixels_selected+=this->rebin_dataset4D(trf,strides,(sqw_pixel *)(&pix_buf[0]),n_pix_in_buffer,pImage,boxMin,boxMax);
  } 
  finalise_rebinning(pImage,image_size);

  pix_buf.clear();


}
//

void 
CenterpieceRebinning4D::set_from_VISIT(const std::string &slicing_description_in_hxml,const std::string &definition)
{  

  double originX, originY, originZ, normalX, normalY, normalZ;
  /*
Mantid::API::ImplicitFunction* ifunc = Mantid::API::Instance().ImplicitFunctionFactory(xmlDefinitions, xmlInstructions);
  PlaneImplicitFunction* plane = dynamic_cast<PlaneImplicitFunction*>(ifunc);

for(int i = 0; i < compFunction->getNFunctions() ; i++)
{

  ImplicitFunction* nestedFunction =   compFunction->getFunction().at(i).get();
  PlaneImplicitFunction* plane = dynamic_cast<PlaneImplicitFunction*>(nestedFunction);
  if(NULL != plane)
  {
    originX = plane->getOriginX();
    originY = plane->getOriginY();
    originZ = plane->getOriginZ();
    normalX = plane->getNOrmalX();
    normalY = plane->getNormalY();
    normalZ = plane->getNOrmalX();
    break;
  }
}
   */
}





//
size_t  
CenterpieceRebinning4D::rebin_dataset4D(const transf_matrix &rescaled_transf,const std::vector<size_t> &stride, const sqw_pixel *source_pix, size_t nPix,MDDataObjects::MD_image_point *data, double boxMin[],double boxMax[])
{
  // set up auxiliary variables and preprocess them.
  double xt,yt,zt,xt1,yt1,zt1,Et,Inf(0),
      pix_Xmin,pix_Ymin,pix_Zmin,pix_Emin,pix_Xmax,pix_Ymax,pix_Zmax,pix_Emax;
  size_t nPixel_retained(0);


  unsigned int nDims = rescaled_transf.nDimensions;
  double rotations_ustep[9];
  std::vector<double> axis_step_inv(nDims,0),shifts(nDims,0),min_limit(nDims,-1),max_limit(nDims,1);
  bool  ignore_something,ignote_all,ignore_nan(rescaled_transf.ignore_NaN),ignore_inf(rescaled_transf.ignore_Inf);

  ignore_something=ignore_nan|ignore_inf;
  ignote_all      =ignore_nan&ignore_inf;
  if(ignore_inf){
    Inf=std::numeric_limits<double>::infinity();
  }


  for(unsigned int ii=0;ii<nDims;ii++){
    axis_step_inv[ii]=1/rescaled_transf.axis_step[ii];
  }
  for(int ii=0;ii<rescaled_transf.nDimensions;ii++){
    shifts[ii]   =rescaled_transf.trans_bott_left[ii];
    min_limit[ii]=rescaled_transf.cut_min[ii];
    max_limit[ii]=rescaled_transf.cut_max[ii];
  }
  for(int ii=0;ii<9;ii++){
    rotations_ustep[ii]=rescaled_transf.rotations[ii];
  }
  int num_OMP_Threads(1);
  bool keep_pixels(false);

  //int nRealThreads;

  size_t i,indl;
  int    indX,indY,indZ,indE;
  size_t  nDimX(stride[0]),nDimY(stride[1]),nDimZ(stride[2]),nDimE(stride[3]); // reduction dimensions; if 0, the dimension is reduced;


  // min-max value initialization

  pix_Xmin=pix_Ymin=pix_Zmin=pix_Emin=  std::numeric_limits<double>::max();
  pix_Xmax=pix_Ymax=pix_Zmax=pix_Emax=- std::numeric_limits<double>::max();
  //
  // work at least for MSV 2008
#ifdef _OPENMP  
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
    for(i=0;i<nPix;i++)
    {
      sqw_pixel pix=source_pix[i];

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
      if(Et<min_limit[3]||Et>=max_limit[3]){
        continue;
      }

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
      indE=(int)floor(Et-min_limit[3]);
      //
      indl  = indX*nDimX+indY*nDimY+indZ*nDimZ+indE*nDimE;
      // i0=nPixel_retained*OUT_PIXEL_DATA_WIDTH;    // transformed pixels;
#pragma omp atomic
      data[indl].s   +=pix.s;
#pragma omp atomic
      data[indl].err +=pix.err;
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

      if(Et<pix_Emin)pix_Emin=Et;
      if(Et>pix_Emax)pix_Emax=Et;

    } // end for i -- imlicit barrier;
#pragma omp critical
    {
      if(boxMin[0]>pix_Xmin/axis_step_inv[0])boxMin[0]=pix_Xmin/axis_step_inv[0];
      if(boxMin[1]>pix_Ymin/axis_step_inv[1])boxMin[1]=pix_Ymin/axis_step_inv[1];
      if(boxMin[2]>pix_Zmin/axis_step_inv[2])boxMin[2]=pix_Zmin/axis_step_inv[2];
      if(boxMin[3]>pix_Emin/axis_step_inv[3])boxMin[3]=pix_Emin/axis_step_inv[3];

      if(boxMax[0]<pix_Xmax/axis_step_inv[0])boxMax[0]=pix_Xmax/axis_step_inv[0];
      if(boxMax[1]<pix_Ymax/axis_step_inv[1])boxMax[1]=pix_Ymax/axis_step_inv[1];
      if(boxMax[2]<pix_Zmax/axis_step_inv[2])boxMax[2]=pix_Zmax/axis_step_inv[2];
      if(boxMax[3]<pix_Emax/axis_step_inv[3])boxMax[3]=pix_Emax/axis_step_inv[3];
    }
  } // end parallel region


  return nPixel_retained;
}
// ***********************************************************************************


} //namespace MDAlgorithms
} //namespace Mantid
