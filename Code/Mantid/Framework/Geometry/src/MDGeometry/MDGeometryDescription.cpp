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

//
MantidMat const & 
MDGeometryDescription::getRotations()const
{
	return Rotations;
}


/// this extracts the size and shape of the current DND object
MDGeometryDescription::MDGeometryDescription(const MDGeometry &origin):
    nContributedPixels(0),
    Rotations(3,3,true)
{
     this->build_from_geometry(origin);
}


MDGeometryDescription::MDGeometryDescription(const MDGeometryBasis &basis):
    nContributedPixels(0),
    Rotations(3,3,true)
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
		this->data[ic].Tag          = it->getId();
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
nContributedPixels(0), Rotations(3,3,true)
{

  this->nDimensions = dimensions.size();
  this->data.resize(dimensions.size());

  if(rotationMatrix.size() != 9)
  {
    throw std::logic_error("Nine components are required for a rotation matrix.");
  }
 
  this->Rotations = MantidMat(rotationMatrix);

  int count = 0; 
  //To get this to work with the rest of MDGeometeryDescription. have to order certain dimensions in a specific fashion.
  DimensionVecIterator dimX = dimensions.end();
  DimensionVecIterator dimY = dimensions.end();
  DimensionVecIterator dimZ = dimensions.end();
  DimensionVecIterator dimT = dimensions.end();

  if(dimensionX.get() != NULL)
  {
    dimX = find_if(dimensions.begin(), dimensions.end(), findDimension(dimensionX));
    createDimensionDescription(*dimX, 0);
    count++;
  }
  if(dimensionY.get() != NULL)
  {
    dimY = find_if(dimensions.begin(), dimensions.end(), findDimension(dimensionY));
    createDimensionDescription(*dimY, 1);
    count++;
  }
  if(dimensionZ.get() != NULL)
  {
    dimZ = find_if(dimensions.begin(), dimensions.end(), findDimension(dimensionZ)); 
    createDimensionDescription(*dimZ, 2);
    count++;
  }
  if(dimensiont.get() != NULL)
  {
    dimT = find_if(dimensions.begin(), dimensions.end(), findDimension(dimensiont));
    createDimensionDescription(*dimT, 3);
    count++;
  }

  //Now process dimension that are not already mapped.
  DimensionVecIterator it = dimensions.begin();
  for(;it != dimensions.end();++it)
  { 
    if((it != dimX) && (it != dimY) && ( it != dimZ) && (it != dimT))
    {
      createDimensionDescription(*it, count);
      count++;
    }
    
  }
}

MDGeometryDescription::MDGeometryDescription(size_t numDims, size_t numRecDims):
    nContributedPixels(19531253125000),
    nDimensions(numDims),
    nReciprocalDimensions(numRecDims),
    Rotations(3,3,true)
{
    this->intit_default_slicing(nDimensions,nReciprocalDimensions);

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

 
}

//
void
MDGeometryDescription::build_from_geometry(const MDGeometry &origin)
{
	this->setRotationMatrix(origin.getRotations());
	
 
    this->nDimensions             = origin.getNumDims();
    this->nReciprocalDimensions   = origin.getNumReciprocalDims();
    std::vector<boost::shared_ptr<IMDDimension> > Dims = origin.getDimensions(false);
  
    DimensionDescription any;
    this->data.assign(nDimensions,any);    
  
    for(unsigned int i=0; i < this->nDimensions;i++){
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
//
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
void 
MDGeometryDescription::setPAxis(size_t i, const std::string &Tag) 
{

   this->check_index(i,"setPAxis");

// move existing dimension structure, described by the tag into new position, described by the index i;
   unsigned int ic(0),old_place_index(0);
   
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
MDGeometryDescription::isAxisNamePresent(size_t i)const
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
//
void
MDGeometryDescription::intit_default_slicing(size_t nDims, size_t nRecDims)
{
    if(nDims>MAX_MD_DIMS_POSSIBLE){
        throw(std::invalid_argument("SlicingProperty::intit_default_slicing: attemting to init more dimension that it is actually possible "));
    }
    nDimensions          = nDims;
    nReciprocalDimensions= nRecDims;

	this->Rotations.identityMatrix();

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
    defaults.cut_max = 5;
    defaults.nBins   = 50;
    defaults.AxisName.assign("");


    this->data.assign(nDimensions,defaults);
    
    for(i=0;i<nReciprocalDimensions;i++){ //
		this->data[i].isReciprocal = true;
	//	this->data[i].direction[i] = 1;
    }
  
  
    for(i=0;i<nDimensions;i++){
      data[i].Tag =def_tags[i]; 
      this->data[i].AxisName = def_tags[i]; //
   }

}


MDGeometryDescription::~MDGeometryDescription(void)
{
}
void 
MDGeometryDescription::check_index(size_t i,const char *fName)const
{
    if(i>=this->nDimensions){
        g_log.error()<<" index out of range for function: "<<fName<<std::endl;
        g_log.error()<<" Allowed nDims: "<<this->nDimensions<<" and requested are: "<<i<<std::endl;
        throw(std::invalid_argument("MDGeometryDescription: index out of range"));
    }
}

} // Geometry
} // Mantid
