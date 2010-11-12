#include "MantidGeometry/MDGeometry/MDGeometryBasis.h"
#include <algorithm>

#include <boost/functional/hash.hpp>
#include <sstream>


namespace Mantid
{
    namespace Geometry
    {
     using namespace Kernel;

  Logger& MDGeometryBasis::g_log=Kernel::Logger::get("MDWorkspaces");

 // default names for Dimensions used by default constructor; length should match the size of the MAX_MD_DIMENSIONS_NUM
 const char *DefaultDimTags[]={"q1","q2","q3","en","u1","u2","u3","u4","u5","u6","u7"};

// Protected constructor;
MDGeometryBasis::MDGeometryBasis(unsigned int nDimensions,unsigned int nReciprocalDimensions):
n_total_dim(nDimensions),
n_reciprocal_dimensions(nReciprocalDimensions),
unit(1,1)
{
    this->check_nDims(nDimensions,nReciprocalDimensions);

    // assign default tags to the array of string;
    std::vector<std::string> DIMS(nDimensions,"");
    unsigned int ic(0);
    for(unsigned int i=0;i<nDimensions;i++){

        if(i==nReciprocalDimensions){   ic=3;
        }
        DIMS[i].assign(DefaultDimTags[ic]);
        ic++;
    }

    this->reinit_GeometryBasis(DIMS,nReciprocalDimensions);

}
//
MDGeometryBasis::MDGeometryBasis(const std::vector<std::string> &tags_list,unsigned int nReciprocalDimensions):
n_total_dim(0),
n_reciprocal_dimensions(nReciprocalDimensions),
unit(1,1)
{
  unsigned int nDimensions = tags_list.size();
  n_total_dim =nDimensions;

  this->check_nDims(nDimensions,nReciprocalDimensions);
 
  this->reinit_GeometryBasis(tags_list,nReciprocalDimensions);
}
//
void 
MDGeometryBasis::check_nDims(unsigned int nDimensions,unsigned int nReciprocalDimensions)
{
   if(nReciprocalDimensions<1||nReciprocalDimensions>3){
        g_log.error()<<"MDGeometryBasis::MDGeometryBasis(unsigned int nDimensions,unsigned int nReciprocalDimensions): number of reciprocal dimensions can vary from 1 to 3 but attempted"<<nReciprocalDimensions<<std::endl;
        throw(std::invalid_argument("This constructor can not be used to buid low dimension datasets geometry"));
  }
    if(nDimensions>MAX_MD_DIMS_POSSIBLE||nDimensions<1){
        g_log.error()<<"MDGeometryBasis::MDGeometryBasis(unsigned int nDimensions,unsigned int nReciprocalDimensions): This constructor attempts to initiate wrong number of dimensions\n";
        throw(std::invalid_argument("This constructor attempts to initiate more than allowed number of dimensions"));
    }
    if(nDimensions<nReciprocalDimensions){
        g_log.error()<<"MDGeometryBasis::MDGeometryBasis(unsigned int nDimensions,unsigned int nReciprocalDimensions): Attempting to initiate total dimensions less than reciprocal dimensions\n";
        throw(std::invalid_argument("Number of reciprocal dimensions is bigger than the total number of dimensions"));
    }
}
//
bool 
MDGeometryBasis::checkTagsCompartibility(const std::vector<std::string> &newTags)const
{
    // if size is different, tags are not compartible;
    if(newTags.size()!=this->getNumDims())return false;
    DimensionID id;
    std::set<DimensionID>::const_iterator it;


    for(unsigned int i=0;i<newTags.size();i++){
        // if one new tag is not among the old one -- the tags lists are not compartible
       id.setDimensionIDValues(-1,newTags[i]);

        it = DimensionIDs.find(id);
        if(it==DimensionIDs.end())return false;
    }
    return true;
}
//
// discards previous geometry and initates the new one. 
void
MDGeometryBasis::reinit_GeometryBasis(const std::vector<std::string> &tag_list,unsigned int nReciprocalDims)
{
    unsigned int i;    
    this->n_reciprocal_dimensions= nReciprocalDims;
    this->n_total_dim            = tag_list.size();
    this->check_nDims(n_total_dim,n_reciprocal_dimensions);

    // clear contents of previous map and set if it was any
    this->dim_names.clear();
    this->DimensionIDs.clear();
    std::pair<std::set<DimensionID>::iterator,bool> ret;
    

    // default geometry assumes all resiprocal dimensions at the beginning and all other orthogonal dimensions after that.
    DimensionID id;
    bool  isReciprocal(true);
    for(i=0;i<n_total_dim;i++){
        if(i<nReciprocalDims){
            isReciprocal=true;
        }else{
            isReciprocal=false;
        }
 
        id.setDimensionIDValues(i,tag_list[i],isReciprocal);
        // remember the id;
        ret=this->DimensionIDs.insert(id);
        if(!ret.second){
          g_log.error()<<" all tags have to be unique but the list of imput tags have some equivalent elements\n";
          throw(std::invalid_argument("Imput tags have equivalent elements"));
        }
       // specify the map to find the name by index
        dim_names.insert(std::pair<int,std::string>(i,tag_list[i]));
    }

    // initiate all real orts to 0
    for(i=0;i<nReciprocalDims;i++){
        this->lattice_ort[i].assign(3,0);
    }
    // default reciprocal lattice: cubic Define three orthogonal unit vectors. 
    this->buildCubicGeometry();

  
  

    // build workspace name 
    for(i=0;i<n_total_dim;i++){
         workspace_name+=tag_list[i]+":";
    }
    std::stringstream  n_rec_dim;   
    n_rec_dim<<"_NDIM_"<<this->n_total_dim<<"x"<<this->n_reciprocal_dimensions;
    workspace_name+=n_rec_dim.str();
 
    this->DimensionIDs;

}
//
std::vector<std::string>
MDGeometryBasis::getBasisTags(void)const
{
    std::vector<std::string> tmp(this->getNumDims(),"");
    for(unsigned int i=0;i<this->getNumDims();i++){
         tmp[i].assign(dim_names.find(i)->second);
    }
    return tmp;
}
//
std::vector<MDGeometryBasis::DimensionID> 
MDGeometryBasis::getDimIDs(void)const
{
  DimensionID id; 
  std::string tag;
  std::vector<MDGeometryBasis::DimensionID> IDs(this->n_total_dim,id);
  std::set<DimensionID>::const_iterator it;
  for(unsigned int i=0;i<IDs.size();i++){
    tag = dim_names.find(i)->second;
    id.setDimensionIDValues(-1,tag);

    it = DimensionIDs.find(id);
    IDs[i] = *it;
  }
  return IDs;
}
// get the list of the column numbers for the list of column names
std::vector<int>  
MDGeometryBasis::getColumnNumbers(const std::vector<std::string> &tag_list)const
{
  size_t n_elements=tag_list.size();
  std::vector<int> nums(n_elements,-1);
  std::set<DimensionID>::const_iterator it;
  DimensionID id;

  for(unsigned int i=0;i<tag_list.size();i++){
      id.setDimensionIDValues(-1,tag_list[i]);
      it = DimensionIDs.find(id);
      if(it==DimensionIDs.end()){
        g_log.error()<<" dimension with name: "<<tag_list[i]<<" can not be found among the basis\n";
        throw(std::invalid_argument("MDGeometryBasis::getOrt: argument out of range")); 
      }
      nums[i]=it->getDimNum();
  }
  return nums;
}

/*
const std::vector<double> & 
MDGeometryBasis::getOrt(const std::string &tag)const
{
    unsigned int nDim=this->getDimIDNum(tag,true);  // throws if not found
    return getOrt(nDim);
}
//
const std::vector<double> &
MDGeometryBasis::getOrt(unsigned int dimNum)const
{

    if(dimNum>=this->n_total_dim){
        g_log.error()<<"WorkspaceGeometry::getOrt: Workspace has "<<this->n_total_dim<<" dimensions but the coordinate for Dimension N: "<<dimNum<<" reqested\n";
        throw(std::invalid_argument("MDGeometryBasis::getOrt: argument out of range"));
    }

    if(dimNum<this->getNumReciprocalDims()){
        return this->lattice_ort[dimNum];
    }else{
        return this->unit;
    }
}
*/
//
std::string
MDGeometryBasis::getColumnName(unsigned int dimNum)const
{
    if(dimNum>=this->n_total_dim){
        g_log.error()<<"WorkspaceGeometry::getOrt: Workspace has "<<this->n_total_dim<<" dimensions but the coordinate for Dimension N: "<<dimNum<<" reqested\n";
        throw(std::invalid_argument("MDGeometryBasis::getOrt: argument out of range")); 
    }

    return dim_names.find(dimNum)->second;
}
//
int 
MDGeometryBasis::getDimNum(const std::string &tag, bool do_throw)const
{
  int i(-1);
  DimensionID id(-1,tag.c_str());
  std::set<DimensionID>::iterator it = this->DimensionIDs.find(id);
  if(it != DimensionIDs.end()){
    i=it->getDimNum();
  }else{
    if(do_throw){
      g_log.error()<<"tag "<<tag<<" does not exist in these dimensions\n";
      throw(std::invalid_argument("Wrong tag requested"));
    }
  }
  return i;
}


/** Build cubic geometry of three orthogonal vectors. 
*/
void
MDGeometryBasis::buildCubicGeometry(void)
{
    // default reciprocal lattice: cubic Define n_rsprcl_dim orthogonal unit vectors. 
    unsigned int i;
    for(i=0;i<this->getNumReciprocalDims();i++){
        this->lattice_ort[i].assign(this->getNumReciprocalDims(),0);
        this->lattice_ort[i].at(i)=1;
    }
}

MDGeometryBasis::~MDGeometryBasis(void){
}



}
}
