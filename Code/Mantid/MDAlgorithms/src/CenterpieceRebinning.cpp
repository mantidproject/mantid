#include "MantidMDAlgorithms/CenterpieceRebinning.h"

namespace Mantid{
    namespace MDAlgorithms{
        using namespace Mantid;
        using namespace MDDataObjects;
        using namespace Kernel;
        using namespace API;
        using namespace Geometry;


// Register the class into the algorithm factory
DECLARE_ALGORITHM(CenterpieceRebinning)

CenterpieceRebinning::CenterpieceRebinning(void): API::Algorithm(), m_progress(NULL) 
{}

/** Destructor     */
CenterpieceRebinning::~CenterpieceRebinning()
{
    if( m_progress ){
            delete m_progress;
            m_progress=NULL;
    }
}
void 
CenterpieceRebinning::init_source(MDWorkspace_sptr inputWSX)
{
MDWorkspace_sptr inputWS;
  // Get the input workspace
    if(existsProperty("Input")){
         inputWS = getProperty("Input");
         if(!inputWS){
              throw(std::runtime_error("input workspace has to exist"));
         }

    }else{
       throw(std::runtime_error("input workspace has to be availible through properties"));
    }
   std::string filename;
   if(existsProperty("Filename")){
      filename= getProperty("Filename");
   }else{
      throw(std::runtime_error("filename property can not be found"));
   }

    inputWS->read_mdd(filename.c_str());

    this->slicingProperty.build_from_geometry(*inputWS);
}
/*
void
CenterpieceRebinning::set_from_VISIT(const std::string &slicing_description,const std::string &XML_definition)
{
    this->slicingProperty.fromXMLstring(slicing_description);
}
*/
void
CenterpieceRebinning::init()
{
      declareProperty(new WorkspaceProperty<MDWorkspace>("Input","",Direction::Input),"initial MD workspace");
      declareProperty(new WorkspaceProperty<MDWorkspace>("Result","",Direction::Output),"final MD workspace");

   //     declareProperty(new MDPropertyGeometry<MDWorkspace>("SlicingData","Result",Direction::Output));
      declareProperty(new API::FileProperty("Filename","", API::FileProperty::Load), "The file containing input MD dataset");


      m_progress = new Progress(this,0,1,10);

   
}
//
void 
CenterpieceRebinning::exec()
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
        AnalysisDataService::Instance().add("OutWorkspace", outputWS);
     }
  }else{
        throw(std::runtime_error("output workspace has to be created "));
  }
  if(inputWS==outputWS){
      throw(std::runtime_error("input and output workspaces have to be different"));
  }
 
  
 
  // transform output workspace to the target shape and allocate memory for resulting matrix
  outputWS->alloc_mdd_arrays(this->slicingProperty);

 

  std::vector<size_t> preselected_cells_indexes;
  size_t  n_precelected_pixels(0);
  this->preselect_cells(*inputWS,inputWS->getPData(),this->slicingProperty,preselected_cells_indexes,n_precelected_pixels);
  if(n_precelected_pixels == 0)return;

  unsigned int n_hits = n_precelected_pixels/PIX_BUFFER_SIZE+1;

  sqw_pixel *pix_buf(NULL);
  size_t    n_pixels_read(0),
            n_pixels_selected(0),
            n_pix_in_buffer(0),pix_buffer_size(PIX_BUFFER_SIZE);

  pix_buf = new sqw_pixel[PIX_BUFFER_SIZE];
  // start reading and rebinning;
  size_t n_starting_cell(0);
 //
  transf_matrix trf = this->build_scaled_transformation_matrix(*inputWS,this->slicingProperty);
  for(unsigned int i=0;i<n_hits;i++){
      n_starting_cell+=inputWS->read_pix_selection(preselected_cells_indexes,n_starting_cell,pix_buf,pix_buffer_size,n_pix_in_buffer);
      n_pixels_read+=n_pix_in_buffer;
      
      n_pixels_selected+=outputWS->rebin_dataset4D(trf,pix_buf,n_pix_in_buffer);
  }
  if(pix_buf){
      delete [] pix_buf;
  }


}
//

void 
CenterpieceRebinning::set_from_VISIT(const std::string &slicing_description_in_hxml,const std::string &definition)
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

transf_matrix 
CenterpieceRebinning::build_scaled_transformation_matrix(const Geometry::MDGeometry &Source,const Geometry::MDGeometryDescription &target)
{

    transf_matrix trf;


    unsigned int  i,ic,j;
    unsigned int nDims = Source.getNumDims();
    trf.nDimensions    = nDims;
    trf.trans_bott_left.assign(nDims,0);
    trf.cut_min.assign(nDims,-1);
    trf.cut_max.assign(nDims,1);
    trf.axis_step.assign(nDims,1);
    MDDimension *pDim(NULL);
    // for convenience can use dimension accessors from source
    //MDGeometryDescription tSource(Source);

    for(i=0;i<nDims;i++){
        trf.trans_bott_left[i]=target.shift(i);
        trf.axis_step[i]=(target.cutMax(i)-target.cutMin(i))/target.numBins(i);
        trf.cut_max[i]  = target.cutMax(i)/trf.axis_step[i];
        trf.cut_min[i]  = target.cutMin(i)/trf.axis_step[i];
    }
    std::vector<double> rot;
    std::vector<double> basis[3]; // not used at the momemnt;

    for(i=0;i<3;i++){
        ic = i*3;
        rot = target.rotations(i,basis);
        for(j=0;j<3;j++){
            trf.rotations[i+j*3]=rot[j]/trf.axis_step[i];
        }
    }
   
    return trf;

}



//
/*! function calculates min and max values of the array of nPoints points (vertices of hypercube)
*
*/
void 
minmax(double &rMin,double &rMax,const std::vector<double> &box)
{
    rMin=box[0];
    rMax=box[0];
    size_t nPoints = box.size();
    for(int i=1;i<nPoints ;i++){
        if(box[i]<rMin)rMin=box[i];
        if(box[i]>rMax)rMax=box[i];
    }
}
//
void
CenterpieceRebinning::preselect_cells(const Geometry::MDGeometry &Source, const data_point *const data,const Geometry::MDGeometryDescription &target, std::vector<size_t> &cells_to_select,size_t &n_preselected_pix)
{
// this algorithm can be substantially enhanced;
    int i;
    n_preselected_pix=0;
    //transf_matrix scaled_trf = this->build_scaled_transformation_matrix(Source,target);

   // transform the grid into new system of coordinate and calculate cell indexes, which contribute into the 
   // dataset;
   int j,k,l,m,mp,mm,sizem;
   double xt1,yt1,zt1,Etm,Etp;
   MDDimension *pDim(NULL);
   double rotations[9];
   rotations[0]=rotations[1]=rotations[2]=rotations[3]=rotations[4]=rotations[5]=rotations[6]=rotations[7]=rotations[8]=0;
   rotations[0]=rotations[4]=rotations[8]=1;

   // evaluate the capacity of the orthogonal dimensions;
   unsigned int  nReciprocalDims= Source.getNumReciprocalDims();
   unsigned int nOrthogonal     = Source.getNumDims()-nReciprocalDims;
   std::vector<std::string> tag=Source.getBasisTags();
   long ind,orthoSize=1;
 
   size_t stride;
   int  nContributed(0);
   // this is the array of vectors to keep orthogonal indexes;
   std::vector<size_t> *enInd = new std::vector<size_t>[nOrthogonal];
   for(l=nReciprocalDims;l<Source.getNumDims();l++){
       nContributed=0;
       pDim   = Source.getDimension(tag[l]);
       sizem  = pDim->getNBins();
       stride = pDim->getStride();
       for(m=0;m<sizem;m++){
           // is rightmpst for min or leftmost for max in range?
              mp=m+1; 
              mm=m-1; if(mm<0)    mm=0;
            // check if it can be in ranges
              Etp=pDim->getX(mp);
              Etm=pDim->getX(mm);

              if(Etp<this->slicingProperty.cutMin(l)||Etm>=this->slicingProperty.cutMax(l)) continue;
            // remember the index of THIS axis 
            enInd[l-nReciprocalDims].push_back(m*stride);
            nContributed++;
       }
       orthoSize*=nContributed;
       if(nContributed==0){  // no cells contribute into the cut; Return
           return;
       }

   }
   // multiply all orthogonal vectors providing size(en)*size(ga1)*size(ga2)*size(ga3) matrix;
   std::vector<long> *orthoInd = new std::vector<long>;
   orthoInd->reserve(orthoSize);
   for(i=0;i<enInd[0].size();i++){
       orthoInd->push_back(enInd[0].at(i));
   }
   for(l=1;l<nOrthogonal;l++){
       size_t orthSize=orthoInd->size();
       for(j=0;j<orthSize;j++){
           long indDim0=orthoInd->at(j);
           for(i=0;enInd[l].size();i++){
               orthoInd->push_back(indDim0+enInd[l].at(i));
           }
       }
   }
   delete [] enInd;

// evaluate the capacity of the 3D space;
// Define 3D subspace and transform it into the coordinate system of the new box;
   size_t size3D(1);
   std::vector<MDDimension* const>rec_dim(nReciprocalDims,NULL);
   for(i=0;i<nReciprocalDims;i++){
       rec_dim[i] = Source.getDimension(tag[i]);
       size3D    *= (rec_dim[i]->getNBins()+1);
   }
   /*
   // dummy dimension is always integrated and 
   MDDimension Dummy();
   for(i=nReciprocalDims;i<3;i++){
       rec_dim[i]=&Dummy;
   }
   */
   std::vector<double> rx,ry,rz,xx,yy,zz;
   rx.assign(size3D,0); xx.assign(size3D,0);
   ry.assign(size3D,0); yy.assign(size3D,0);
   rz.assign(size3D,0); zz.assign(size3D,0);
// nAxis points equal nBins+1;
// lattice points transformed into new coordinate system. 
// needed modifications for nRecDim<3
   for(k=0;k<=rec_dim[2]->getNBins();k++){
       for(j=0;j<=rec_dim[1]->getNBins();j++){
           for(i=0;i<=rec_dim[0]->getNBins();i++){
               rx.push_back(rec_dim[0]->getX(i));
               ry.push_back(rec_dim[1]->getX(j));
               rz.push_back(rec_dim[2]->getX(k));

 //              rx.push_back(rec_dim[0]->getX(i)-scaled_trf.trans_bott_left[0]);
               //ry.push_back(rec_dim[1]->getX(j)-scaled_trf.trans_bott_left[1]);
//               rz.push_back(rec_dim[2]->getX(k)-scaled_trf.trans_bott_left[2]);
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
   int im,ip,jm,jp,km,kp;
   nCell3D sh(rec_dim[0]->getNBins()+1,rec_dim[1]->getNBins()+1);
//           ind3D(this->dim_sizes[u1],this->dim_sizes[u2]);
   double rMin,rMax;
   unsigned int box_dim=1;
   for(i=0;i<nReciprocalDims;i++){
       box_dim*=2;
   }
   std::vector<double> r(box_dim,0);
   size_t ind3;

   for(k=0;k<rec_dim[2]->getNBins();k++){
       km=k-1; if(km<0)            km=0;
       kp=k+1; 
       for(j=0;j<rec_dim[1]->getNBins();j++){
           jm=j-1; if(jm<0)            jm=0;
           jp=j+1; 

           for(i=0;i<rec_dim[0]->getNBins();i++){
               im=i-1; if(im<0)            im=0;
               ip=i+1; 

               r[0]=xx[sh.nCell(im,jm,km)]; r[1]=xx[sh.nCell(ip,jm,km)]; r[2]=xx[sh.nCell(im,jp,km)]; r[3]=xx[sh.nCell(ip,jp,km)];
               r[4]=xx[sh.nCell(im,jm,kp)]; r[5]=xx[sh.nCell(ip,jm,kp)]; r[6]=xx[sh.nCell(im,jp,kp)]; r[7]=xx[sh.nCell(ip,jp,kp)];
    
               minmax(rMin,rMax,r);
               if(rMax<slicingProperty.cutMin(0)||rMin>=slicingProperty.cutMax(0))continue;

               r[0]=yy[sh.nCell(im,jm,km)];  r[1]=yy[sh.nCell(ip,jm,km)];r[2]=yy[sh.nCell(im,jp,km)];  r[3]=yy[sh.nCell(ip,jp,km)];
               r[4]=yy[sh.nCell(im,jm,kp)];  r[5]=yy[sh.nCell(ip,jm,kp)];r[6]=yy[sh.nCell(im,jp,kp)];  r[7]=yy[sh.nCell(ip,jp,kp)];
    
               minmax(rMin,rMax,r);
               if(rMax<slicingProperty.cutMin(1)||rMin>=slicingProperty.cutMax(1))continue;

               r[0]=zz[sh.nCell(im,jm,km)];  r[1]=zz[sh.nCell(ip,jm,km)];r[2]=zz[sh.nCell(im,jp,km)];  r[3]=zz[sh.nCell(ip,jp,km)];
               r[4]=zz[sh.nCell(im,jm,kp)];  r[5]=zz[sh.nCell(ip,jm,kp)];r[6]=zz[sh.nCell(im,jp,kp)];  r[7]=zz[sh.nCell(ip,jp,kp)];
    
               minmax(rMin,rMax,r);
               if(rMax<slicingProperty.cutMin(2)||rMin>=slicingProperty.cutMax(2))continue;

               ind3=i*rec_dim[0]->getStride()+j*rec_dim[1]->getStride()+k*rec_dim[2]->getStride();
               for(l=0;l<orthoInd->size();l++){
                   ind = ind3+orthoInd->at(l);
                   if(data[ind].npix>0){
                        cells_to_select.push_back(ind);
                        n_preselected_pix+=data[ind].npix;
                   }
               }


           }
       }
    }
    delete orthoInd;


}

} //namespace MDAlgorithms
} //namespace Mantid