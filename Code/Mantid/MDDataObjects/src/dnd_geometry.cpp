#include "stdafx.h"
#include "dnd_geometry.h"
// the counter of the dimensions, already initiated;
int  dnd_geometry::n_dim_IDS=0;
// the holder of the singleton class
dnd_geometry * dnd_geometry::Geometry=NULL;

/*! The function provides singleton implementation of  this class*/
dnd_geometry* dnd_geometry::instance(int nDimensions)
{
    if(!Geometry){
            Geometry=new dnd_geometry(nDimensions);
        }else{
            if(nDimensions!=Geometry->n_total_dim){
                throw(errorMantid("Attempt to redefine the number of dimensions in the existing geomerty"));
            }
        }
        return Geometry;
} 

// Private constructor;
dnd_geometry::dnd_geometry(int nDimensions):
n_total_dim(nDimensions)
{
    // reset number of initiated dimensions to 0;
    dnd_geometry::n_dim_IDS=0;
    if(n_total_dim<1||n_total_dim>MAX_NDIMS_POSSIBLE){
        std::stringstream ErrBuf;
        ErrBuf<<" Attemted to create workspace with "<<nDimensions<<" which is out of the limits allowed";
        throw(errorMantid(ErrBuf.str()));
    }
    for(int i=0;i<n_total_dim;i++){
        this->pDimension.push_back(new Dimension());
    }
}


dnd_geometry::~dnd_geometry(void){
    for(int i=0;i<this->n_total_dim;i++){
        delete this->pDimension[i];
    }
    if(Geometry)delete Geometry;
    Geometry=NULL;
}
// this private function is used 
int dnd_geometry::getFreeID(void)
{
   int tmp=n_dim_IDS;
   n_dim_IDS++;
   if(n_dim_IDS>n_total_dim){
       throw(errorMantid(" Attempt to allocate extra dimensions for the workspace geomerty"));
   }
   return tmp;
}


