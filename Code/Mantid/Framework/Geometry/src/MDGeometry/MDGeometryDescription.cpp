#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include <algorithm>
#include <float.h>
#include <sstream>
#include <cfloat>


namespace Mantid{
    namespace Geometry{

 // get reference to logger for MD workspaces
    Kernel::Logger& MDGeometryDescription::g_log=Kernel::Logger::get("MDWorkspaces");

    	
//Helper Function object to find dimensions.
struct findDimension
{
   const Dimension_sptr m_dimension;
   findDimension(const Dimension_sptr dimension ): m_dimension( dimension ) 
   { 
   }
   bool operator () (const boost::shared_ptr<IMDDimension> obj ) const{ //overloaded operator to check the condition.
     return m_dimension->getDimensionId() == obj->getDimensionId();
   }
};
void 
MDGeometryDescription::set_proj_plain(const Geometry::V3D &u, const Geometry::V3D &v, const UnitCell &Lattice)
{

	MantidMat B = Lattice.getB();

	V3D e1 = B*u;
	V3D V  = B*v;
	V3D e3  =e1.cross_prod(V);
	e3.normalize();
	double norm2 = e3.norm2();
	if(norm2<FLT_EPSILON){
		g_log.error()<<"MDGeometryDescription::projection can not be defined by two parallel vectors: "<<u<<" and "<<v<<std::endl;
		throw(std::invalid_argument(" two parallel vectors do not define the projection lain"));
	}
	e1.normalize();
	V3D e2= e3.cross_prod(e1);

	MantidMat Transf(3,3);
	Transf.setColumn(0,e1);
	Transf.setColumn(1,e2);
	Transf.setColumn(2,e3);

	std::vector<double> view = Transf;
	Transf.Invert();
	this->rotations = Transf.get_vector();
	//bool real_change(false);
	//if(	proj_plain[0] != u){
	//	real_change   = true;
	//	proj_plain[0] = u;
	//};
	//if (proj_plain[1] != v){
	//	real_change   = true;
	//	proj_plain[1] = v;
	//}
	//if(real_change)direction_changed=true;
	//V3D beamAxis(0,0,1);
	//this->Rotations = Quat(z,beamAxis);
}
//
std::vector<double> 
MDGeometryDescription::getRotations()const
{

	return rotations;
}

///// the function returns the rotation vector which allows to transform vector inumber i into the basis;
//std::vector<double> 
//MDGeometryDescription::setRotations(unsigned int i,const std::vector<double> basis[3])
//{
//// STUB  !!! 
//    this->check_index(i,"rotations");
//    if(i>2){
//        return std::vector<double>(1,1);
//    }
//
//    std::vector<double> tmp;
//    tmp.assign(3,0);
//    tmp[i]=1;
// 
//    rotations.assign(i*3+i,1);
//    return tmp;
//}

/// this extracts the size and shape of the current DND object
MDGeometryDescription::MDGeometryDescription(const MDGeometry &origin):
direction_changed(false)
{
    this->build_from_geometry(origin);
}


MDGeometryDescription::MDGeometryDescription(const MDGeometryBasis &basis):
direction_changed(false)
{
	std::set<MDBasisDimension> basisDims = basis.getBasisDimensions();
	this->nDimensions           = 0 ;
	this->nReciprocalDimensions = 0;
	std::set<MDBasisDimension>::const_iterator it= basisDims.begin();
	for(;it != basisDims.end();it++){
		this->nDimensions++;
		if(it->getIsReciprocal())this->nReciprocalDimensions++;
	
	}
	// create default dimension descriptions;
	this->data.resize(this->nDimensions);
	unsigned int ic(0);
	for(it= basisDims.begin();it != basisDims.end();it++){
		this->data[ic].Tag = it->getId();
		this->data[ic].isReciprocal = it->getIsReciprocal();

		ic++;
	}
	// place dimension descriptions in the list of the descritions to the positions, specified by the basis;
	int num;
	for(it= basisDims.begin();it != basisDims.end();it++){
		num = it->getColumnNumber();
		this->setPAxis(num,it->getId());
	}

}

MDGeometryDescription::MDGeometryDescription(
      DimensionVec dimensions, 
      Dimension_sptr dimensionX, 
      Dimension_sptr dimensionY,  
      Dimension_sptr dimensionZ, 
      Dimension_sptr dimensiont,
      RotationMatrix rotationMatrix
):
direction_changed(false)
{
  this->nDimensions = dimensions.size();
  this->data.resize(dimensions.size());
  this->rotations = rotationMatrix;
  if(rotations.size() != 9)
  {
    throw std::logic_error("Nine components are required for a rotation matrix.");
  }

  //To get this to work with the rest of MDGeometeryDescription. have to order certain dimensions in a specific fashion.
  DimensionVecIterator dimX = find_if(dimensions.begin(), dimensions.end(), findDimension(dimensionX));
  DimensionVecIterator dimY = find_if(dimensions.begin(), dimensions.end(), findDimension(dimensionY));
  DimensionVecIterator dimZ = find_if(dimensions.begin(), dimensions.end(), findDimension(dimensionZ));
  DimensionVecIterator dimT = find_if(dimensions.begin(), dimensions.end(), findDimension(dimensiont));

  //Check dimensions;
  createDimensionDescription(*dimX, 0);
  createDimensionDescription(*dimY, 1);
  createDimensionDescription(*dimZ, 2);
  createDimensionDescription(*dimT, 3);

  //Now process dimension that are not already mapped.
  DimensionVecIterator it = dimensions.begin();
  int count = 4; // mappings take priority see above.
  while(it != dimensions.end())
  {
    if((it != dimX) && (it != dimY) && ( it != dimZ) && (it != dimT))
    {
      createDimensionDescription(*it, count);
      count++;
    }
    it++;
  }
}

void MDGeometryDescription::createDimensionDescription(Dimension_sptr dimension, const int i)
{
  this->data[i].data_shift = 0;
  this->data[i].cut_min = dimension->getMinimum();
  this->data[i].cut_max = dimension->getMaximum()*(1+FLT_EPSILON);
  this->data[i].nBins = dimension->getNBins();
  this->data[i].AxisName  = dimension->getName();
  this->data[i].isReciprocal = dimension->isReciprocal();
//  this->data[i].data_scale = dimension->getScale();
  this->data[i].Tag = dimension->getDimensionId();

  //Handle reciprocal dimensions.
  if(dimension->isReciprocal())
  {
	  this->data[i].direction = dimension->getDirection();
        
  }
}

//
void
MDGeometryDescription::build_from_geometry(const MDGeometry &origin)
{
	this->Rotations  = Quat();

	this->direction_changed       = false;
    this->nDimensions             = origin.getNumDims();
    this->nReciprocalDimensions   = origin.getNumReciprocalDims();
    std::vector<boost::shared_ptr<IMDDimension> > Dims = origin.getDimensions(false);
	
    unsigned int i,nr(0);

    DimensionDescription any;
    this->data.assign(nDimensions,any);    

    for(i=0;i<this->nDimensions;i++){
		this->createDimensionDescription(Dims[i],i);
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
size_t
MDGeometryDescription::getImageSize()const
{
   size_t data_size = 1;
   for(unsigned int i=0;i<nDimensions;i++){

	   if( this->data[i].nBins>1 ){
		   data_size*=this->data[i].nBins;
	   }
          
   }
	return data_size;
}
//****** SET *******************************************************************************
//void 
//MDGeometryDescription::renameTag(unsigned int num,const std::string &newID)
//{
//   this->check_index(num,"renameTag");
//   this->data[num].Tag = newID;
//}
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
    
    std::vector<std::string> def_tags;
    
    for(unsigned int i=0;i<nDims;i++){
      std::stringstream buf;
      if(i<nRecDims){
        buf<<"q"<<i+1;
      }else{
        buf<<"u"<<i-nRecDims+1;
      }
      def_tags.push_back(buf.str());

    }

    unsigned int i;
    DimensionDescription defaults;
    defaults.data_shift=0;
    defaults.cut_min =-1;
    defaults.cut_max = 1;
    defaults.nBins   = 1;
    defaults.AxisName.assign("");


    this->data.assign(nDimensions,defaults);
    
    for(i=0;i<nReciprocalDimensions;i++){ //
		this->data[i].isReciprocal = true;
		this->data[i].direction[i] = 1;
    }
  
  
    for(i=0;i<nDimensions;i++){
      data[i].Tag =def_tags[i]; 
      this->data[i].AxisName = def_tags[i]; //
   }

}
void 
MDGeometryDescription::setDirection(unsigned int i,const V3D &coord)
{
    this->check_index(i,"setDirection");
    if(i<3){
		this->data[i].direction =coord;
    }else{
         throw(std::invalid_argument("SlicingProperty::setDirection wrong parameter, index>=3 and attempting to set a coordinate of orthogonal dimension"));
 
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
        g_log.error()<<" Allowed nDims: "<<this->nDimensions<<" and requested are: "<<i<<std::endl;
        throw(std::invalid_argument("MDGeometryDescription: index out of range"));
    }
}

} // Geometry
} // Mantid
