#include "MDDataObjects/stdafx.h"
#include "MDDataObjects/SlicingProperty.h"
//slicing property will go out

/// the function returns the rotation vector which allows to transform vector inumber i into the basis;
namespace Mantid{
    namespace MDDataObjects{

std::vector<double> 
SlicingProperty::rotations(unsigned int i,const std::vector<double> basis[3])const
{
// STUB  !!! 
    this->check_index(i,"rotations");
    if(i>2){
        return std::vector<double>(1,1);
    }

    std::vector<double> tmp;
    tmp.assign(3,0);
    tmp[i]=1;
    return tmp;
}
/// this extracts the size and shape of the current DND object
SlicingProperty::SlicingProperty(const MDGeometry &origin)
{
    Dimension *pDim;

    this->nDimensions = origin.getNumDims();
    this->coordinates[0].assign(3,0);
    pDim = origin.getDimension(eh);
    if(pDim){
        this->coordinates[0] = pDim->getCoord();
    }
    this->coordinates[1].assign(3,0);
    pDim = origin.getDimension(ek);
    if(pDim){
        this->coordinates[1] = pDim->getCoord();
    }

    this->coordinates[2].assign(3,0);
    pDim = origin.getDimension(el);
    if(pDim){
        this->coordinates[2] = pDim->getCoord();
    }



    this->trans_bott_left.assign(nDimensions,0);    
    this->cut_min.assign(nDimensions,0);       
    this->cut_max.assign(nDimensions,0);       
    this->nBins.assign(nDimensions,0);        
    this->AxisName.assign(nDimensions,"");     
    this->pAxis.assign(nDimensions,eh);        
    this->AxisID.assign(MAX_NDIMS_POSSIBLE,-1);

    unsigned int i;
    for(i=0;i<nDimensions;i++){
        pDim=origin.getDimension(i);
        this->cut_min[i] = pDim->getMinimum();
        this->cut_max[i] = pDim->getMaximum();
        this->nBins[i]   = pDim->getNBins();
        this->pAxis[i]   = pDim->getDimensionID();
        this->AxisName[i]= pDim->getName();
        this->AxisID[int(pDim->getDimensionID())]=i;
    }

}

//****** SET *******************************************************************************
void 
SlicingProperty::setCoord(unsigned int i,const std::vector<double> &coord)
{
    this->check_index(i,"setCoord");
    if(i<3){
        if(coord.size()!=3){
            throw(std::invalid_argument("SlicingProperty::setCoord wrong parameter, index<3 and attempting to set a non-3 point coordinate"));
        }
        this->coordinates[i]=coord;
    }else{
        if(coord.size()!=1){
            throw(std::invalid_argument("SlicingProperty::setCoord wrong parameter, index>=3 and attempting to set a coordinate of orthogonal dimension"));
        }
    }
}
void 
SlicingProperty::setPAxis(unsigned int i, DimensionsID ID)
{
   this->check_index(i,"setPAxis");

   unsigned int ic(0);

   std::vector<DimensionsID>::iterator itOld;
   for(itOld=this->pAxis.begin();itOld!=this->pAxis.end();itOld++){
       if(*itOld==ID){
           break;
       }
       ic++;

   }

   if(itOld == this->pAxis.end()){ // if not found, replace some old dimension 
        this->pAxis[i] = ID;
   }else if(ic==i){                // if on the old position, leave everything as it is
       return;
   }else{
       this->pAxis.erase(itOld);
       std::vector<DimensionsID>::iterator itNew;
       ic=0;
       for(itNew=this->pAxis.begin();itNew!=this->pAxis.end();itNew++){
           if(ic==i){
               this->pAxis.insert(itNew,ID);          
               return;
           }
           ic++;
       }
       this->pAxis.push_back(ID);
   }

}
void
SlicingProperty::setShift(unsigned int i,double Val)
{
   this->check_index(i,"setShift");
   this->trans_bott_left[i]=Val;
}
void
SlicingProperty::setCutMin(unsigned int i,double Val)
{
    this->check_index(i,"setCutMin");
    this->cut_min[i]=Val;
}
void
SlicingProperty::setCutMax(unsigned int i,double Val)
{
    this->check_index(i,"setCutMax");
    this->cut_max[i]=Val;
}
void
SlicingProperty::setNumBins(unsigned int i,unsigned int Val)
{
    this->check_index(i,"setNumBins");
    if(Val>MAX_REASONABLE_BIN_NUMBER){
        throw(std::invalid_argument("SlicingProperty::setNumBins value bin requested is larger than MAX_REASONABLE_BIN_NUMBER"));
    }
    this->nBins[i]=Val;
}
void
SlicingProperty::setAxisName(unsigned int i,const std::string &Name)
{
    this->check_index(i,"setAxisName");
    this->AxisName[i]=Name;
}
//*************************************************************************************

double 
SlicingProperty::cutMin(unsigned int i)const
{
    this->check_index(i,"cutMin");
    return this->cut_min[i];
}
double
SlicingProperty::cutMax(unsigned int i)const
{
    this->check_index(i,"cutMax");
    return this->cut_max[i];

}
unsigned int 
SlicingProperty::numBins(unsigned int i)const
{
    this->check_index(i,"numBins");
    return this->nBins[i];

}
bool
SlicingProperty::isAxisNamePresent(unsigned int i)const
{
    this->check_index(i,"isAxisNamePresent");

    if(this->AxisName[i].empty()){
       return false;
    }else{
        return true;
    }
}
std::string 
SlicingProperty::getAxisName(unsigned int i)const
{
    this->check_index(i,"getAxisName");
    return (this->AxisName.at(i));
}
DimensionsID 
SlicingProperty::getPAxis(unsigned int i)const
{
    this->check_index(i,"getPAxis");
    return this->pAxis[i];
}


SlicingProperty::SlicingProperty(unsigned int numDims):
nDimensions(numDims)
{
    this->intit_default_slicing(numDims);

}
SlicingProperty::SlicingProperty(std::vector<DimensionsID> &IDs)
{
    unsigned int i;
    unsigned int nDims=(unsigned int)IDs.size();
    this->intit_default_slicing(nDims);

    // calculate the reciprocal dimensions which are not present in the ID list and clear their coordinates;
    unsigned int nReciprocalDims(0);
    std::vector<unsigned int> rec_dim_to_clear(3,1);
    for(i=0;i<nDims;i++){
        if(IDs[i]<3){
            nReciprocalDims++;
            rec_dim_to_clear[i]=0;
        }
    }

    if(nReciprocalDims<3){
        for(i=0;i<nDims;i++){
            if(rec_dim_to_clear[i]){
                this->coordinates[i].assign(3,0);
            }
        }
    }
}

void
SlicingProperty::intit_default_slicing(unsigned int nDims)
{
    if(nDims>MAX_NDIMS_POSSIBLE){
        throw(std::invalid_argument("SlicingProperty::intit_default_slicing: attemting to init more dimension that it is actually possible "));
    }
    nDimensions=nDims;

    unsigned int i;
    this->AxisID.assign(MAX_NDIMS_POSSIBLE,-1);
    this->cut_min.assign(nDimensions,-1);
    this->cut_max.assign(nDimensions, 1);
    this->nBins.assign(nDimensions,1);
    this->trans_bott_left.assign(nDimensions,0);

    for(i=0;i<3;i++){
        this->coordinates[i].assign(3,0);
        this->coordinates[i].at(i)= 1;
    }

    this->AxisName.resize(nDimensions);
    this->pAxis.assign(nDimensions,eh);
    for(i=0;i<nDimensions;i++){
        this->pAxis[i]=(DimensionsID)i;
        this->AxisID[i]=i;
    }

}
std::vector<double> 
SlicingProperty::getCoord(DimensionsID id)const
{
    if(id>2){
        throw(std::invalid_argument("SlicingProperty::getCoord attemt to get coordinate of non-reciprocal dimension"));
    }
    return this->coordinates[id];
}


SlicingProperty::~SlicingProperty(void)
{
}
void 
SlicingProperty::check_index(unsigned int i,const char *fName)const
{
    if(i>=this->nDimensions){
        std::stringstream err;
        err<<" index out of range for function: "<<fName<<std::endl;
        err<<" Allowed nDims: "<<this->nDimensions<<" and requested is: "<<i<<std::endl;
        throw(std::invalid_argument(err.str()));
    }
}
}
}