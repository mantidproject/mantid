#include "MantidMDAlgorithms/DynamicCPRRebinning.h"
#include <algorithm>

namespace Mantid{
namespace MDAlgorithms{

       using namespace Mantid::MDDataObjects;
       using namespace Mantid::Kernel;
       using namespace Mantid::API;
       using namespace Mantid::Geometry;

/// helper class to identify the indexes on an auxilary 3D lattice; Needed for Preselect_Cells
class nCell3D
{
size_t NX,NY;
public:
    nCell3D(size_t nx,size_t ny):NX(nx),NY(ny){};
    size_t nCell(long i,long j, long k)const{return i+NX*(j+k*NY);}  
};

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
//
DynamicCPRRebinning::DynamicCPRRebinning(const MDWorkspace_const_sptr &sourceWS, MDGeometryDescription const *const pTargetDescr,
                                         const MDWorkspace_sptr  & TargetWS ):
pSourceWS(sourceWS),
pTargetWS(TargetWS),
// get pointer to the source image data --> provides number of pixels contributing ;
pSourceImg(&(sourceWS->get_const_MDImage())),
pSourceGeom(&(sourceWS->get_const_MDGeometry())),  
pSourceImgData(pSourceImg->get_const_pData()),
pSourceDataReader(&(sourceWS->get_const_FileReader())),
pTargetDescr(pTargetDescr),
n_preselected_pix(0)
{
    // initialisze the target workspace to have proper size and shape
    pTargetWS->init(pSourceWS,pTargetDescr);

    pTargetGeom    = pTargetWS->get_spMDImage()->getGeometry();
    //
    n_target_cells = pTargetWS->get_spMDImage()->getDataSize();
    pTargetImgData = pTargetWS->get_spMDImage()->get_pData();
}
//*********************************************************************
size_t 
DynamicCPRRebinning::preselect_cells()
{
  // this algoithm can be substantially enhanced
  // a) by implementing fast search within the limits;
  // b) by identifying the sells which lie entirely within the cut and the sells which can be cut through;
  // c) should we try to use deque to avoid multiple insertions for the same indexes? (correct equalities will eliminate them anyway)
  unsigned int i;
  this->n_preselected_pix=0;
  this->preselected_cells.clear();

  // transform the grid into new system of coordinate and calculate cell indexes, which contribute into the
  // dataset;
  unsigned int j,k,l,mp,mm,sizem;
  double       xt1,yt1,zt1,Etm,Etp;

  //TODO: this will be obtained from the taget and source?
  double rotations[9];
  rotations[0]=rotations[1]=rotations[2]=rotations[3]=rotations[4]=rotations[5]=rotations[6]=rotations[7]=rotations[8]=0;
  rotations[0]=rotations[4]=rotations[8]=1;

 
  // evaluate the capacity of the orthogonal dimensions;
  unsigned int  nReciprocal    = pSourceGeom->getNumReciprocalDims();
  unsigned int  nOrthogonal    = pSourceGeom->getNumDims() - nReciprocal;

  IMDDimension *pDim;
  std::vector<boost::shared_ptr<IMDDimension> > pAllDim  = pSourceGeom->getDimensions();
  std::vector<boost::shared_ptr<IMDDimension> > pOrthogonal(nOrthogonal);
  std::vector<boost::shared_ptr<IMDDimension> > pReciprocal(nReciprocal);
  unsigned int nr(0),no(0);

  // get the orthogonal and reciprocal dimensions separately
  size_t grid_capasity(1);
  for(i=0;i<pSourceGeom->getNumDims();i++){
    if(pAllDim[i]->isReciprocal()){
      pReciprocal[nr]=pAllDim[i];
      nr++;
    }else{
      pOrthogonal[no]=pAllDim[i];
      no++;
    }
    grid_capasity*= pAllDim[i]->getNBins();
  }

  size_t ind,orthoSize=1;

  size_t stride;
  int  nContributed(0);
  // this is the array of vectors to keep orthogonal indexes;
  std::vector< std::vector<size_t> > enInd(nOrthogonal,std::vector<size_t>(0,0));

  // estimate cut limits in the orthogonal dimensions
  std::vector<double> ort_cut_min(nOrthogonal,0),ort_cut_max(nOrthogonal,0);

  DimensionDescription *pDimDescr;

  for(i=0;i<nOrthogonal;i++){

    pDimDescr = pTargetDescr->pDimDescription(pOrthogonal[i]->getDimensionId());
	ort_cut_min[i]=pDimDescr->cut_min;
    ort_cut_max[i]=pDimDescr->cut_max;
  }

  for(i=0;i<nOrthogonal;i++){
    nContributed=0;
    pDim   = pOrthogonal[i].get();
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
      return 0;
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

    // new reciprocal dimensions are placed to absolutely different positions wrt the source reciprocal dimensions
    pDimDescr  = pTargetDescr->pDimDescription(rec_dim[i]->getDimensionId());
	cut_min[i]=pDimDescr->cut_min;
    cut_max[i]=pDimDescr->cut_max;
  }
  // if there are less then 3 reciprocal dimensions, lets make 3 now to make the algorithm generic
  for(i=nReciprocal;i<3;i++){
    rec_dim[i] =  boost::shared_ptr<MDDimension>(new MDDimDummy(i));
    // we should know the limits the dummy dimensions have
    cut_min[i] = rec_dim[i]->getMinimum();
    cut_max[i] = rec_dim[i]->getMaximum()*(1+FLT_EPSILON);

    //TODO: deal with rotations in case they have not been dealt with before;
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
        // unlike the cut over point, we select cells with points on the upper boundary; should we?
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
          if(pSourceImgData[ind].npix>0){
            preselected_cells.push_back(ind);
            n_preselected_pix+=pSourceImgData[ind].npix;
          }
        }


      }
    }
  }
  orthoInd.clear();
  // this piece of code can be absent in some algorithms when preselection does not select the same cells twice and
  // selecting adjasten cells is not important;
  size_t n_preselected_cells = preselected_cells.size();
  if(n_preselected_cells>1){
      // sort in increasing order (should be N*ln(N))
        std::sort(preselected_cells.begin(),preselected_cells.end());
        size_t ic(1);
        // remove dublicated values
        size_t prev_value=preselected_cells[0];
        for(size_t i=1;i<n_preselected_cells;i++){
            //HACK! TODO: fix this!;
            if(preselected_cells[i]!=prev_value&&preselected_cells[i]<grid_capasity){
                prev_value           =preselected_cells[i];
                preselected_cells[ic]=prev_value;
                ic++;
            }
        }
        if(ic!=n_preselected_cells)preselected_cells.resize(ic);
        n_preselected_cells = ic;
  }
  return n_preselected_cells;
}

//
size_t 
DynamicCPRRebinning::finalize_rebinning()
{
  size_t i;
  // normalize signal and error of the dnd object;
  if(pTargetImgData[0].npix>0){
    pTargetImgData[0].s   /= pTargetImgData[0].npix;
    pTargetImgData[0].err /=(pTargetImgData[0].npix*pTargetImgData[0].npix);
  }
  // and calculate cells location for pixels;
  pTargetImgData[0].chunk_location=0;

  // counter for the number of retatined pixels;
  size_t nPix = pTargetImgData[0].npix;
  for(i=1;i<n_target_cells;i++){
    pTargetImgData[i].chunk_location=pTargetImgData[i-1].chunk_location+pTargetImgData[i-1].npix; // the next cell starts from the the boundary of the previous one
    // plus the number of pixels in the previous cell
    if(pTargetImgData[i].npix>0){
      nPix        +=pTargetImgData[i].npix;
      pTargetImgData[i].s   /=pTargetImgData[i].npix;
      pTargetImgData[i].err /=(pTargetImgData[i].npix*pTargetImgData[i].npix);
    }
  };
  return nPix;
}//***************************************************************************************



} // end namespaces
}
