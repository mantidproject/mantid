#include "stdafx.h"
#include "WorkspaceGeometry.h"
namespace Mantid
{
    namespace MDDataObjects
    {

// copy constructr
WorkspaceGeometry::WorkspaceGeometry(const WorkspaceGeometry &orgn){
    
    this->n_total_dim=orgn.n_total_dim;
    /// vector of dimensions id-s, specific for current architecture, the size of dimensions is n_total_dimensions,
    this->DimensionIDs=orgn.DimensionIDs;
 

    for(int i=0;i<3;i++){
        this->lattice_ort[i]=orgn.lattice_ort[i];
    }   

}
const std::vector<double> &
WorkspaceGeometry::getOrt(DimensionsID ID)const
{
    unsigned int id=(unsigned int)ID;
    if(id>=this->n_total_dim){
        std::stringstream err;
        err<<"WorkspaceGeometry::getOrt: Workspace has "<<this->n_total_dim<<" dimensions but the coordinate for Dimension N: "<<id<<" reqested\n";
        throw(std::invalid_argument(err.str()));
    }

    if(id<this->n_rsprcl_dim){
        return this->lattice_ort[id];
    }else{
        return this->unit;
    }
}
//
int 
WorkspaceGeometry::getDimRefNum(DimensionsID ID, bool nothrow)const
{
    if(ID<this->DimensionIDs[0]||ID>this->DimensionIDs[n_total_dim-1]){
        if(nothrow){
            return -1;
        }else{
            throw(std::out_of_range("WorkspaceGeometry::getDimRefNum: the ID is out of range for current geometry"));
        }
    }
    // are there any point of implementing a binary search here?
    for(unsigned int i=0;i<this->n_total_dim;i++){
        if(this->DimensionIDs[i]== ID){
            return i;
        }
    }
    throw(std::invalid_argument("WorkspaceGeometry::getDimRefNum: Logical Error, the gemetry's dimension ID-s are not arranged properly"));
}

//
DimensionsID WorkspaceGeometry::getDimensionID(unsigned int nDim)const
{
    if(nDim<0||nDim>=this->n_total_dim){
        std::stringstream ErrBuf;
        ErrBuf<<"WorkspaceGeometry::getDimensionID: Dimension N "<<nDim<<" is out of defined N="<<this->n_total_dim<<" Dimensions\n";
        throw(std::out_of_range(ErrBuf.str()));
    }
    return DimensionIDs[nDim];

}
//
void
WorkspaceGeometry::reinit_WorkspaceGeometry(const std::vector<DimensionsID> &ID)
{
    unsigned int n_reciprocal_dims(0),i,nDims((unsigned int)ID.size());
    for(i=0;i<nDims;i++){
        if(ID[i]<en){       n_reciprocal_dims++;
        }
    }

    if(nDims<1||nDims>MAX_NDIMS_POSSIBLE){
        std::stringstream ErrBuf;
        ErrBuf<<"WorkspaceGeometry::reinit_class: Attemted to create workspace with "<<nDims<<" which is out of the limits allowed";
        throw(std::length_error(ErrBuf.str()));
    }

    if(n_reciprocal_dims<1||n_reciprocal_dims>3){
            throw(std::out_of_range("WorkspaceGeometry::reinit_class: number of reciprocal dimensions out of range (nr-dim<1||nr-dim>3)"));
    }
    //if(n_rsprcl_dim>n_total_dim)n_rsprcl_dim=n_total_dim;
    this->n_total_dim =nDims;
    this->n_rsprcl_dim=n_reciprocal_dims;


    // default reciprocal lattice: cubic Define three orthogonal unit vectors. 
    this->buildCubicGeometry();

    // the WorkspaceGeometry has to have all their dimensions sorted properly. 
    std::vector<DimensionsID> ID0(ID);
    sort(ID0.begin(),ID0.end());


    this->DimensionIDs=ID0;

}
// Private constructor;
WorkspaceGeometry::WorkspaceGeometry(unsigned int nDimensions):
n_total_dim(nDimensions),
n_rsprcl_dim(3),
unit(1,1)
{
    if(nDimensions<4){
        throw(std::invalid_argument("This constructor can not be used to buid low dimension datasets geometry"));
    }

    // default geometry assumes all resiprocal dimensions at the beginning and all other orthogonal dimensions after that.
    std::vector<DimensionsID> DIMS(n_total_dim,eh);
    for(unsigned int i=0;i<n_total_dim;i++){
        DIMS[i]=(DimensionsID)i;
    }
    // initiate all real orts to 0
    this->lattice_ort[0].assign(3,0);
    this->lattice_ort[1].assign(3,0);
    this->lattice_ort[2].assign(3,0);

    this->reinit_WorkspaceGeometry(DIMS);

}
/** Build cubic geometry of three orthogonal vectors. 
*/
void
WorkspaceGeometry::buildCubicGeometry(void)
{
    // default reciprocal lattice: cubic Define n_rsprcl_dim orthogonal unit vectors. 
    unsigned int i;
    for(i=0;i<this->n_rsprcl_dim;i++){
        this->lattice_ort[i].assign(this->n_rsprcl_dim,0);
        this->lattice_ort[i].at(i)=1;
    }


}
WorkspaceGeometry::~WorkspaceGeometry(void){
}

}
}
