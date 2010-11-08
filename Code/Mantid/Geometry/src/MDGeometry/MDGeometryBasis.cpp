#include "MantidGeometry/MDGeometry/MDGeometryBasis.h"
#include <algorithm>

#include <boost/functional/hash.hpp>
#include <sstream>


namespace Mantid
{
    namespace Geometry
    {
     using namespace Kernel;

    /// hasher;
      boost::hash<std::string> hash_string;

void
DimensionID::setDimensionIDValues(int newNum,const std::string &newTag,bool if_recipocal)
{
      iDimID       = newNum;
      DimensionTag.assign(newTag);
      is_reciprocal =if_recipocal; 

     // boost::hash<std::string> hash_string;
      tagHash=hash_string(DimensionTag);
}


Logger& MDGeometryBasis::g_log=Kernel::Logger::get("MDWorkspaces");
 // default names for Dimensions used by default constructor; length should match the size of the MAX_MD_DIMENSIONS_NUM
 const char *DefaultDimTags[]={"q1","q2","q3","en","u1","u2","u3","u4","u5","u6","u7"};
 // Protected constructor;
MDGeometryBasis::MDGeometryBasis(unsigned int nDimensions,unsigned int nReciprocalDimensions):
n_total_dim(nDimensions),
n_reciprocal_dimensions(nReciprocalDimensions),
unit(1,1)
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
size_t
MDGeometryBasis::getDimHash(const std::string &tag)const
{
     
    int dim_num=findTag(tag, true);
    return DimensionIDs[dim_num].getDimHash();
}
//
bool 
MDGeometryBasis::calculateTagsCompartibility(const std::vector<std::string> &newTags)const
{
    // if size is different, tags are not compartible;
    if(newTags.size()!=this->getNumDims())return false;

    int ind;
    for(unsigned int i=0;i<newTags.size();i++){
        // if one new tag is not among the old one -- the tags lists are not compartible
        if((ind=getDimIDNum(newTags[i],false))<0)return false;
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

    if(n_total_dim<1||n_total_dim>MAX_MD_DIMS_POSSIBLE){
        g_log.error()<<"WorkspaceGeometry::reinit_class: Attemted to create workspace with "<<n_total_dim<<"  is out of the limits allowed\n";
        throw(std::out_of_range("WorkspaceGeometry::reinit_class: too many dimensions requested"));
     }
    if(this->n_reciprocal_dimensions<1||this->n_reciprocal_dimensions>3){
        g_log.error()<<"WorkspaceGeometry::reinit_class: number of reciprocal dimensions out of range (nr-dim<1||nr-dim>3)\n";
            throw(std::out_of_range("WorkspaceGeometry::reinit_class: number of reciprocal dimensions out of range (nr-dim<1||nr-dim>3)"));
    }
    // clear contents of previous map if it was any
    dim_list.clear();

    // default geometry assumes all resiprocal dimensions at the beginning and all other orthogonal dimensions after that.
    std::vector<DimensionID> DIMS(n_total_dim,DimensionID());
    size_t tagHash;
    bool  isReciprocal(true);
    for(i=0;i<n_total_dim;i++){
        if(i<nReciprocalDims){
            isReciprocal=true;
        }else{
            isReciprocal=false;
        }
        DIMS[i].setDimensionIDValues(i,tag_list[i],isReciprocal);
   
      
    }

    // initiate all real orts to 0
    for(i=0;i<nReciprocalDims;i++){
        this->lattice_ort[i].assign(3,0);
    }
 

    // default reciprocal lattice: cubic Define three orthogonal unit vectors. 
    this->buildCubicGeometry();

    // the WorkspaceGeometry has to have all their dimensions sorted properly. 
    //sort(DIMS.begin(),DIMS.end());
 
     
    for(i=0;i<n_total_dim;i++){
        tagHash = DIMS[i].getDimHash();
        dim_list[tagHash]=i;

        workspace_name+=tag_list[i];
    }

    std::stringstream  n_rec_dim;   
    n_rec_dim<<"_rcd"<<this->n_reciprocal_dimensions;
    workspace_name+=n_rec_dim.str();
 
    this->DimensionIDs=DIMS;

}
//
std::vector<std::string>
MDGeometryBasis::getBasisTags(void)const
{
    std::vector<std::string> tmp(this->getNumDims(),"");
    for(unsigned int i=0;i<this->getNumDims();i++){
        tmp[i].assign(this->DimensionIDs[i].getDimensionTag());
    }
    return tmp;
}

// copy constructr
MDGeometryBasis::MDGeometryBasis(const MDGeometryBasis &orgn){
    
    this->n_total_dim=orgn.n_total_dim;
    /// vector of dimensions id-s, specific for current architecture, the size of dimensions is n_total_dimensions,
    this->DimensionIDs=orgn.DimensionIDs;
 

    for(int i=0;i<3;i++){
        this->lattice_ort[i]=orgn.lattice_ort[i];
    }   

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
MDGeometryBasis::getTag(unsigned int dimNum)const
{
    if(dimNum>=this->n_total_dim){
        g_log.error()<<"WorkspaceGeometry::getOrt: Workspace has "<<this->n_total_dim<<" dimensions but the coordinate for Dimension N: "<<dimNum<<" reqested\n";
        throw(std::invalid_argument("MDGeometryBasis::getOrt: argument out of range")); 
    }
    return this->DimensionIDs[dimNum].getDimensionTag();
}
//
int 
MDGeometryBasis::getDimIDNum(const std::string &tag, bool do_throw)const
{
    int ind = findTag(tag,do_throw);
    if(ind>-1){
        return DimensionIDs[ind].getDimID();
    }
    return -1;
}
/*
int 
MDGeometryBasis::getDimNum(const std::string &tag, bool do_throw)const
{
    int ind = findTag(tag,do_throw);
    if(ind>0){
        return DimensionIDs[ind].getDimNum();
    }
    return -1;
}
*/
int
MDGeometryBasis::findTag(const std::string &tag, bool do_throw)const
{
   int rez=-1;
    std::map<size_t,int>::const_iterator it;
    size_t hash = hash_string(tag);
    it = this->dim_list.find(hash);
    if(it==dim_list.end()){
        if(do_throw){
             g_log.error()<<"MDGeometryBasis::getDimIDNum: the tag: "<<tag<<" does not belong to current geometry\n";
             throw(std::out_of_range("MDGeometryBasis::getDimRefNum: the tag requested does not belong to current geometry"));
         }
    }else{
        rez = it->second;
    }
    return rez;

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
