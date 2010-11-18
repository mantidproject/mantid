#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include <float.h>


namespace Mantid{
    namespace Geometry{

 // get reference to logger for MD workspaces
    Kernel::Logger& MDGeometryDescription::g_log=Kernel::Logger::get("MDWorkspaces");

        
/// the function returns the rotation vector which allows to transform vector inumber i into the basis;
std::vector<double> 
MDGeometryDescription::setRotations(unsigned int i,const std::vector<double> basis[3])
{
// STUB  !!! 
    this->check_index(i,"rotations");
    if(i>2){
        return std::vector<double>(1,1);
    }

    std::vector<double> tmp;
    tmp.assign(3,0);
    tmp[i]=1;
 
    rotations.assign(i*3+i,1);
    return tmp;
}
/// this extracts the size and shape of the current DND object
MDGeometryDescription::MDGeometryDescription(const MDGeometry &origin)
{
    this->build_from_geometry(origin);
}
//
void
MDGeometryDescription::build_from_geometry(const MDGeometry &origin)
{


    this->nDimensions             = origin.getNumDims();
    this->nReciprocalDimensions   = origin.getNumReciprocalDims();

    for(unsigned int i=0;i<nReciprocalDimensions;i++){
      this->coordinates[i].assign(3,0);
      MDDimension * pDim = origin.getDimension(i);
      if(pDim){
           this->coordinates[i] = pDim->getCoord();
      }
    }
    
    SlicingData any;
    this->data.assign(nDimensions,any);    
  
    unsigned int i;
    for(i=0;i<nDimensions;i++){

        MDDimension *pDim = origin.getDimension(i);
        this->data[i].Tag            = pDim->getDimensionTag();
        this->data[i].trans_bott_left= 0;
        this->data[i].cut_min        = pDim->getMinimum();
        this->data[i].cut_max        = pDim->getMaximum()*(1+FLT_EPSILON);
        this->data[i].nBins          = pDim->getNBins();
        this->data[i].AxisName       = pDim->getName();

    }

}
//
int 
MDGeometryDescription::getTagNum(const std::string &Tag,bool do_throw)const{
    int iTagNum=-1;
    int ic(0);
    it_const_data it;
    for(it=data.begin();it!=data.end();it++){
        if(it->Tag.compare(Tag)==0){
          iTagNum=ic;
          break;
        }
        ic++;
    }
    if(iTagNum<0 && do_throw){
        g_log.error()<<" Tag "<<Tag<<" does not exist\n";
        throw(std::invalid_argument(" The requested tag does not exist"));
    }
    return iTagNum;
}

//****** SET *******************************************************************************

void 
MDGeometryDescription::setPAxis(unsigned int i, const std::string &Tag) 
{

   this->check_index(i,"setPAxis");

// move existing dimension structure, described by the tag into new position, described by the index i;
   unsigned int ic(0),old_place_index;
   
   it_data it,old_place, new_place;
   old_place = data.end();
   for(it=data.begin();it!=old_place;it++){
       if(it->Tag.compare(Tag)==0){
            old_place      = it;
            old_place_index= ic;
            if(ic >i){
              old_place_index=ic+1; // after insertion it will be one index higher
              break;
            }
       }
       if(ic == i){
           new_place=it;
          if(old_place!=data.end())break;
       }
       ic++;
    }
    if(old_place==data.end()){
        g_log.error()<<" Tag "<<Tag<<" does not exist\n";
        throw(std::invalid_argument(" The requested tag does not exist"));
    }
    if(new_place!=old_place){
        data.insert(new_place,*old_place); //  this invalidates old iterators
        old_place = data.begin()+old_place_index;
        data.erase(old_place);
    }
 
}
//
void
MDGeometryDescription::setShift(unsigned int i,double Val)
{
   this->check_index(i,"setShift");
   this->data[i].trans_bott_left=Val;
}
void
MDGeometryDescription::setCutMin(unsigned int i,double Val)
{
    this->check_index(i,"setCutMin");
    this->data[i].cut_min=Val;
}
void
MDGeometryDescription::setCutMax(unsigned int i,double Val)
{
    this->check_index(i,"setCutMax");
    this->data[i].cut_max=Val;
}
void
MDGeometryDescription::setNumBins(unsigned int i,unsigned int Val)
{
    this->check_index(i,"setNumBins");
    if(Val>MAX_REASONABLE_BIN_NUMBER){
        throw(std::invalid_argument("SlicingProperty::setNumBins value bin requested is larger than MAX_REASONABLE_BIN_NUMBER"));
    }
    if(Val==0)Val=1;
    this->data[i].nBins=Val;
}
void
MDGeometryDescription::setAxisName(unsigned int i,const std::string &Name)
{
    this->check_index(i,"setAxisName");
    this->data[i].AxisName=Name;
}
//*************************************************************************************

double 
MDGeometryDescription::cutMin(unsigned int i)const
{
    this->check_index(i,"cutMin");
    return this->data[i].cut_min;
}
double
MDGeometryDescription::cutMax(unsigned int i)const
{
    this->check_index(i,"cutMax");
    return this->data[i].cut_max;

}
unsigned int 
MDGeometryDescription::numBins(unsigned int i)const
{
    this->check_index(i,"numBins");
    return this->data[i].nBins;

}
double 
MDGeometryDescription::shift(unsigned int i)const
{
    this->check_index(i,"Shift");
    return this->data[i].trans_bott_left;
}
bool
MDGeometryDescription::isAxisNamePresent(unsigned int i)const
{
    this->check_index(i,"isAxisNamePresent");

    if(this->data[i].AxisName.empty()){
       return false;
    }else{
        return true;
    }
}
std::string 
MDGeometryDescription::getAxisName(unsigned int i)const
{
    this->check_index(i,"getAxisName");
    return (this->data[i].AxisName);
}
std::string 
MDGeometryDescription::getTag(unsigned int i)const
{
    this->check_index(i,"getTag");
    return (this->data[i].Tag);
}

std::vector<std::string> 
MDGeometryDescription::getDimensionsTags(void)const
{
    std::vector<std::string> tags(this->nDimensions,"");

    it_const_data it;
    unsigned int ic(0);
    for(it=this->data.begin();it!=data.end();it++){
         tags[ic] = it->Tag;
         ic++;
    }
    return tags;
}


MDGeometryDescription::MDGeometryDescription(unsigned int numDims,unsigned int numRecDims):
nDimensions(numDims),
nReciprocalDimensions(numRecDims)
{
    this->intit_default_slicing(nDimensions,nReciprocalDimensions);

}
/*
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
*/
void
MDGeometryDescription::intit_default_slicing(unsigned int nDims,unsigned int nRecDims)
{
    if(nDims>MAX_MD_DIMS_POSSIBLE){
        throw(std::invalid_argument("SlicingProperty::intit_default_slicing: attemting to init more dimension that it is actually possible "));
    }
    nDimensions          = nDims;
    nReciprocalDimensions= nRecDims;


    rotations.assign(9,0);
    rotations[0]=rotations[4]=rotations[8]=1;

    nDimensions=nDims;
    MDGeometry DefBasis(nDimensions,nReciprocalDimensions);
    std::vector<std::string> def_tags=DefBasis.getBasisTags();

    unsigned int i;
    SlicingData defaults;
    defaults.trans_bott_left=0;
    defaults.cut_min =-1;
    defaults.cut_max = 1;
    defaults.nBins   = 1;
    defaults.AxisName.assign("");


    this->data.assign(nDimensions,defaults);
 
    for(i=0;i<nReciprocalDimensions;i++){
        this->coordinates[i].assign(3,0);
        this->coordinates[i].at(i)= 1;
    }
  
  
    for(i=0;i<nDimensions;i++){
      data[i].Tag =def_tags[i]; 
      this->data[i].AxisName = def_tags[i];
   }

}
void 
MDGeometryDescription::setCoord(unsigned int i,const std::vector<double> &coord)
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

/*
std::vector<double> 
SlicingProperty::getCoord(DimensionsID id)const
{
    if(id>2){
        throw(std::invalid_argument("SlicingProperty::getCoord attemt to get coordinate of non-reciprocal dimension"));
    }
    return this->coordinates[id];
}

*/
MDGeometryDescription::~MDGeometryDescription(void)
{
}
void 
MDGeometryDescription::check_index(unsigned int i,const char *fName)const
{
    if(i>=this->nDimensions){
        g_log.error()<<" index out of range for function: "<<fName<<std::endl;
        g_log.error()<<" Allowed nDims: "<<this->nDimensions<<" and requested is: "<<i<<std::endl;
        throw(std::invalid_argument("MDGeometryDescription: index out of range"));
    }
}

} // Geometry
} // Mantid
