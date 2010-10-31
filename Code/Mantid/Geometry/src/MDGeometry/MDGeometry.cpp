//#include "MDDataObjects/stdafx.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include <boost/functional/hash.hpp>


using namespace Mantid::Kernel;

namespace Mantid{
    namespace Geometry{
       
//----------------------------------------------------------------

void
MDGeometry::setRanges(MDGeometryDescription const &trf)
{
    unsigned int i;
    unsigned int n_new_dims=trf.getNumDims();
    MDDimension *pDim;
    if(n_new_dims>this->n_total_dim){
        throw(std::invalid_argument("Geometry::setRanges: Attempting to set more dimensions then are currently defined "));
    }
    this->arrangeDimensionsProperly(trf.getAxisTags());


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


void 
MDGeometry::reinit_Geometry(const MDGeometryDescription &trf)
{

    unsigned int i;
/*
    std::vector<DimensionID> ID(trf.getPAxis());
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
*/
}

void 
MDGeometry::arrangeDimensionsProperly(const std::vector<std::string> &tags)
{
    unsigned int n_new_dims=tags.size();
    unsigned int i;

    if(n_new_dims>this->getNumDims()){
        g_log.error()<<"Geometry::arrangeDimensionsProperly: Attempting to arrange more dimensions then are currently defined \n";
        throw(std::invalid_argument("Geometry::arrangeDimensionsProperly: Attempting to arrange more dimensions then are currently defined "));
    }


    // array to keep final expanded dimensions
    std::vector<MDDimension *> pExpandedDims(this->n_total_dim,NULL);    
    // array to keep final collapsed dimensions which sould be placed after expanded
    std::vector<MDDimension *> pCollapsedDims(this->n_total_dim,NULL);  
    // array to keep thd initial dimensions which were not mentioned in transformation
    std::vector<MDDimension *> pCurrentDims(this->theDimension);  
    
  

    unsigned int n_expanded_dimensions(0),n_collapsed_dimensions(0);
    int dim_num;
    MDDimension *pDim;
 
     // let's sort dimensions as requested by the list of tags
    for(i=0;i<n_new_dims;i++){

        // when dimension num we want to use next
        dim_num = this->getDimNum(tags[i],false);
        if(dim_num<0){
            g_log.error()<<" The dimension with tag "<<tags[i]<<" does not belong to current geometry\n";
            throw(std::invalid_argument("Geometry::arrangeDimensionsProperly: new dimension requested but this function can not add new dimensions"));
        }
        pDim    = this->theDimension[dim_num];
        this->theDimension[dim_num] = NULL;
           
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
    for(i=0;i<this->getNumDims();i++){
        pDim = this->theDimension[i];
        if(!pDim)continue;

        if(pDim->getIntegrated()){ // this is collapsed dimension;
            pCollapsedDims[n_collapsed_dimensions]=pDim;
            n_collapsed_dimensions++;
        }else{
            pExpandedDims[n_expanded_dimensions]  =pDim;
            n_expanded_dimensions++;
        }
        theDimension[i]=NULL;
    }
 

    this->n_expanded_dim=n_expanded_dimensions;
    // total number of dimensions should not change;
    if(n_expanded_dimensions+n_collapsed_dimensions!=this->getNumDims()){
        g_log.error()<<"Geometry::arrangeDimensionsProperly: Dimensions: n_expanded+n_collapsed!= nTotal; serious logical error";
        throw(Exception::NotImplementedError("Geometry::arrangeDimensionsProperly: Dimensions: n_expanded+n_collapsed!= nTotal; serious logical error"));
    }
    dimensions_map.clear();

    // deal with expanded dimensions
    for(i=0;i<this->n_expanded_dim;i++){
        pDim  = pExpandedDims[i];
        this->theDimension[i]=pDim;
    }
    // now with collapsed dimensions;
    unsigned int ind(n_expanded_dim);
    for(i=0;i<n_collapsed_dimensions;i++){
        pDim  = pCollapsedDims[i];
        this->theDimension[ind+i]=pDim;

    }
    size_t hash;
    for(i=0;i<getNumDims();i++){
        hash  = theDimension[i]->getDimHash();
       dimensions_map[hash]=i;
    }


};
//
MDDimension & 
MDGeometry::getYDimension(void)const
{
    if(this->n_total_dim<2){
        throw(std::invalid_argument("No Y dimension is defined in this workspace"));
    }
    return *(theDimension[1]);
}
//
MDDimension & 
MDGeometry::getZDimension(void)const
{
    if(this->n_total_dim<3){
        throw(std::invalid_argument("No Z dimension is defined in this workspace"));
    }
    return *(theDimension[2]);
}
//
MDDimension & 
MDGeometry::getTDimension(void)const
{
    if(this->n_total_dim<4){
        throw(std::invalid_argument("No T dimension is defined in this workspace"));
    }
    return *(theDimension[3]);
}
//
std::vector<MDDimension *> 
MDGeometry::getIntegratedDimensions(void)
{
    std::vector<MDDimension *> tmp;

    if(this->n_expanded_dim!=this->n_total_dim){
        std::vector<MDDimension *>::iterator it;
        it=theDimension.begin()+this->n_expanded_dim;

        tmp.assign(it,theDimension.end());
    }
    return tmp;
}

MDDimension * const 
MDGeometry::getDimension(unsigned int i)const
{
    if(i>=this->getNumDims()){
        g_log.error()<<"Geometry::getDimension: attemting to get the dimension N"<<i<<" but this is out of the dimensions range";
        throw(std::out_of_range("Geometry::getDimension: attemting to get the dimension with non-existing number"));
    }
    return theDimension[i];
}
MDDimension * const 
MDGeometry::getDimension(const std::string &tag)const
{
//    MDDimension *pDim(NULL);
    int dim_num=getDimNum(tag,true);

  
    return theDimension[dim_num];
}
 /// hasher;
boost::hash<std::string> geomerty_hash;
int
MDGeometry::getDimNum(const std::string &tag,bool do_throw)const
{
// finds the index of the dimension in the array of dimensions
    int dimNum=-1;

    std::map<size_t,int>::const_iterator it;

    size_t tag_hash = geomerty_hash(tag);
    it = dimensions_map.find(tag_hash);
    if(it == dimensions_map.end()){
        if(do_throw){
            g_log.error()<<" MDGeometry::getDimNum: dimension with tag: "<<tag<<" does not exist in current geometry\n";
            throw(std::invalid_argument("Geometry::getDimension: wrong dimension tag"));
        }
    }else{
        dimNum = it->second;
    }
    return dimNum;
}
/*
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
*/

MDGeometry::MDGeometry(unsigned int nDimensions,unsigned int nReciprocalDimensions):
MDGeometryBasis(nDimensions,nReciprocalDimensions),
n_expanded_dim(0)
{
    this->theDimension.assign(nDimensions,NULL);
    this->init_empty_dimensions(this->getDimensionIDs());
    //this->reinit_Geometry(this->DimensionIDs);
    
}

 void 
MDGeometry::init_empty_dimensions(const std::vector<DimensionID> &ID)
 {
     unsigned int i;
     size_t hash;
     for(i=0;i<ID.size();i++){
            if(ID[i].isReciprocal()){  // 1) initialize reciprocal space dimensions (momentum components)
                this->theDimension[i] = new MDDimensionRes(ID[i]);
            }else{                     // 2) initialize additional orthogonal dimensions
                this->theDimension[i] = new MDDimension(ID[i]);
            }
            hash = ID[i].getDimHash();
            dimensions_map[hash]=i;
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
    this->dimensions_map.clear();
    this->theDimension.clear();

    
}

}
}