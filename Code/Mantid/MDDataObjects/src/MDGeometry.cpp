#include "MDDataObjects/stdafx.h"
#include "MDDataObjects/MDGeometry.h"
#include "MDDataObjects/SlicingProperty.h"

using namespace Mantid::Kernel;

namespace Mantid{
    namespace MDDataObjects{
//----------------------------------------------------------------
void 
MDGeometry::reinit_Geometry(const SlicingProperty &trf)
{
    unsigned int i;
    std::vector<DimensionsID> ID(trf.getPAxis());
    sort(ID.begin(),ID.end());
    // are the old geometry congruent to the new geometry?
    bool congruent_geometries(true);
    unsigned int nExistingDims=this->getNumDims();
    if(ID.size()!=nExistingDims){
        congruent_geometries=false;
    }else{
        for(i=0;i<nExistingDims;i++){
            if(this->DimensionIDs[i]!=ID[i]){
                congruent_geometries=false;
                break;
            }
        }
    }
    if(congruent_geometries){
        this->arrangeDimensionsProperly(trf.getPAxis());
    }else{

        this->reinit_WorkspaceGeometry(ID);
    
        // clear old dimensions if any
        for(i=0;i<this->theDimension.size();i++){
            if(this->theDimension[i]){
                delete this->theDimension[i];
                theDimension[i]=NULL;
            }
        }
        this->init_empty_dimensions(ID);
    }
    // all reciprocal dimensions may have new coordinates in the WorkspaceGeometry coordinate system, so these coordinates have to 
    // be set properly;
    Dimension *pDim;
    DimensionsID id;
    for(i=0;i<3;i++){
        id=DimensionsID(i);
        // get nDim and cycle if this dim does not exist
        if(getDimRefNum(id,true)<0)continue;

        // get a reciprocal dimension with proper ID
        pDim = this->getDimension(id);
        // and set its coordinate from the transformation matrix;
        pDim->setCoord(trf.getCoord(id));
    }

}


void 
MDGeometry::arrangeDimensionsProperly(const std::vector<DimensionsID> &IDS)
{
    unsigned int n_new_dims=(unsigned int)IDS.size();

    if(n_new_dims>this->n_total_dim){
        throw(std::invalid_argument("Geometry::arrangeDimensionsProperly: Attempting to arrange more dimensions then are currently defined "));
    }

    // array to keep final expanded dimensions
    std::vector<Dimension *> pExpandedDims(this->n_total_dim,NULL);    
    // array to keep final collapsed dimensions which sould be placed after expanded
    std::vector<Dimension *> pCollapsedDims(this->n_total_dim,NULL);  
    // array to keep thd initial dimensions which were not mentioned in transformation
    std::vector<Dimension *> pCurrentDims(this->theDimension);  
    
   
    unsigned int i,n_expanded_dimensions(0),n_collapsed_dimensions(0);
    int dim_num;
    Dimension *pDim;
    DimensionsID id;

     // let's sort dimensions as requested by pAxis
    for(i=0;i<n_new_dims;i++){
        // when dimension id we want to use next
        id   = IDS.at(i);
        // get the index of this dimension in current dimensions array;
        dim_num=this->getDimNum(id);


        if(dim_num<0){
              throw(std::invalid_argument("Geometry::arrangeDimensionsProperly: new dimension requested but this function can not add new dimensions"));
        }
        // get existing dimension to work with;
        pDim = this->theDimension[dim_num];
        // this dimension has been dealt with, no need to deal with it again;
        pCurrentDims[dim_num]=NULL;

 
        // set range according to request;
        if(pDim->getIntegrated()){ // this is collapsed dimension;
            pCollapsedDims[n_collapsed_dimensions]=pDim;
            n_collapsed_dimensions++;
        }else{
            pExpandedDims[n_expanded_dimensions]  =pDim;
            n_expanded_dimensions++;
        }
    }
    // deal with the dimensions, which were not menshioned in the transformation request
    for(i=0;i<this->n_total_dim;i++){
        pDim = pCurrentDims[i];
        if(!pDim)continue;

        if(pDim->getIntegrated()){ // this is collapsed dimension;
            pCollapsedDims[n_collapsed_dimensions]=pDim;
            n_collapsed_dimensions++;
        }else{
            pExpandedDims[n_expanded_dimensions]  =pDim;
            n_expanded_dimensions++;
        }
    }
    // rearrange the dimensions together into the final dimensions array 
    this->theDimension.assign(this->n_total_dim,NULL);
    this->theDimensionIDNum.assign(MAX_NDIMS_POSSIBLE,-1);


    this->n_expanded_dim=n_expanded_dimensions;
    // total number of dimensions should not change;
    if(n_expanded_dimensions+n_collapsed_dimensions!=this->n_total_dim){
        throw(Exception::NotImplementedError("Geometry::arrangeDimensionsProperly: Dimensions: n_expanded+n_collapsed!= nTotal; serious logical error"));
    }
    // deal with expanded dimensions
    for(i=0;i<this->n_expanded_dim;i++){
        pDim  = pExpandedDims[i];
        this->theDimension[i]=pDim;
        this->theDimensionIDNum[pDim->getDimensionID()]=i;
    }
    // now with collapsed dimensions;
    unsigned int ind;
    for(i=0;i<n_collapsed_dimensions;i++){
        pDim  = pCollapsedDims[i];
        ind   = n_expanded_dimensions+i;
        this->theDimension[ind]=pDim;
        this->theDimensionIDNum[pDim->getDimensionID()]=ind;
    }


};
void
MDGeometry::setRanges(const SlicingProperty &trf)
{
    unsigned int i;
    unsigned int n_new_dims=trf.getNumDims();
    Dimension *pDim;
    if(n_new_dims>this->n_total_dim){
        throw(std::invalid_argument("Geometry::setRanges: Attempting to set more dimensions then are currently defined "));
    }



   // let's analyse the dimensions, which were mentioned in transformation matrix and set the ranges of these dimensions 
    // as requested
    for(i=0;i<n_new_dims;i++){
        pDim=this->getDimension(i);
        pDim->setRange(trf.cutMin(i),trf.cutMax(i),trf.numBins(i));
        // set new axis name ;
        if(trf.isAxisNamePresent(i)){
            pDim->setName(trf.getAxisName(i));
        }

    }
    this->n_expanded_dim=0;
    for(i=0;i<this->n_total_dim;i++){
          pDim=this->getDimension(i);
          if(!pDim->getIntegrated()){
              this->n_expanded_dim++;
          }
    }

}

Dimension & 
MDGeometry::getYDimension(void)const
{
    if(this->n_total_dim<2){
        throw(std::invalid_argument("No Y dimension is defined in this workspace"));
    }
    return *(theDimension[1]);
}
Dimension & 
MDGeometry::getZDimension(void)const
{
    if(this->n_total_dim<3){
        throw(std::invalid_argument("No Z dimension is defined in this workspace"));
    }
    return *(theDimension[2]);
}
Dimension & 
MDGeometry::getTDimension(void)const
{
    if(this->n_total_dim<4){
        throw(std::invalid_argument("No T dimension is defined in this workspace"));
    }
    return *(theDimension[3]);
}
std::vector<Dimension *> 
MDGeometry::getIntegratedDimensions(void)
{
    std::vector<Dimension *> tmp;

    if(this->n_expanded_dim!=this->n_total_dim){
        std::vector<Dimension *>::iterator it;
        it=theDimension.begin()+this->n_expanded_dim;

        tmp.assign(it,theDimension.end());
    }
    return tmp;
}
Dimension * 
MDGeometry::getDimension(unsigned int i)const
{
    if(i>=this->n_total_dim){
        throw(std::out_of_range("Geometry::getDimension: attemting to get the dimension which is out of range"));
    }
    return theDimension[i];
}
Dimension * 
MDGeometry::getDimension(DimensionsID ID)const
{
    int ind=this->getDimNum(ID);
    if(ind<0)return NULL;

    if(ind>=MAX_NDIMS_POSSIBLE){
        throw(std::out_of_range("Geometry::getDimension: dim index out of range; logical error"));
    }
    return this->theDimension[ind];

}
int
MDGeometry::getDimNum(DimensionsID ID)const
{
    if(ID<0||ID>MAX_NDIMS_POSSIBLE){
        throw(std::out_of_range("Geometry::getDimNum: DimensionsID out of range"));
    }
    return (this->theDimensionIDNum[ID]);

}
std::vector<double> 
MDGeometry::getOrt(DimensionsID id)const
{
    if(id>3||id<0){
        throw(std::out_of_range("Geometry::getOrt dimension ID out of range "));
    }
    std::vector<double> tmp(this->WorkspaceGeometry::getOrt(id));
    Dimension *pDim = this->getDimension(id);
    std::vector<double> coord=pDim->getCoord();
//TO DO : this is for orthogonal system of coordinates only; need write proper stuff for non-orthogonal
    tmp[0]*=coord[0];
    tmp[1]*=coord[1];
    tmp[2]*=coord[2];
    double norma=sqrt(tmp[0]*tmp[0]+tmp[1]*tmp[1]+tmp[2]*tmp[2]);
    tmp[0]/=norma;
    tmp[1]/=norma;
    tmp[2]/=norma;
    return tmp;
}

MDGeometry::MDGeometry(unsigned int nDimensions):
WorkspaceGeometry(nDimensions),
n_expanded_dim(0)
{
    this->theDimension.assign(nDimensions,NULL);
    this->theDimensionIDNum.assign(MAX_NDIMS_POSSIBLE,-1);
    this->init_empty_dimensions(this->DimensionIDs);

    this->reinit_Geometry(this->DimensionIDs);
    
}
 void 
MDGeometry::init_empty_dimensions(const std::vector<DimensionsID> &ID)
 {
     unsigned int i;
     for(i=0;i<ID.size();i++){
            if(ID[i]<en){     // 1) initialize reciprocal space dimensions (momentum components)
                this->theDimension[i] = new DimensionRes(ID[i]);
                this->theDimensionIDNum[DimensionsID(i)]=i;

            }else{           // 2) initialize additional orthogonal dimensions
                this->theDimension[i] = new Dimension(ID[i]);
                this->theDimensionIDNum[DimensionsID(i)]=i;
            }
     }
     // all dimensions initiated by default constructor are integrated;
     this->n_expanded_dim=0;

 }
MDGeometry::~MDGeometry(void)
{
    unsigned int i;
    for(i=0;i<this->n_total_dim;i++){
        delete this->theDimension[i];

    }
    this->theDimensionIDNum.clear();
    this->theDimension.clear();
    
}
}
}