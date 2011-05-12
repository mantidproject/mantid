//#include "MDDataObjects/stdafx.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidGeometry/Math/Matrix.h"


using namespace Mantid::Kernel;

namespace Mantid{
  namespace Geometry{


    //----------------------------------------------------------------
Logger& MDGeometry::g_log=Kernel::Logger::get("MDWorkspaces");

void
MDGeometry::setRanges(MDGeometryDescription const &trf)
{
  size_t n_new_dims=trf.getNumDims();
  boost::shared_ptr<MDDimension> pDim;
  if(n_new_dims>m_basis.getNumDims()){
    g_log.error()<<" MDGeometry::setRanges transformation sets more ranges then already defined\n";
    throw(std::invalid_argument("Geometry::setRanges: Attempting to set more dimensions then are currently defined "));
  }

  std::vector<std::string> tag=trf.getDimensionsTags();

  // let's analyse the dimensions, which were mentioned in transformation matrix and set the ranges of these dimensions 
  // as requested
  for(size_t i=0;i<n_new_dims;i++){

    pDim=this->getDimension(tag[i]);
    DimensionDescription* descr = trf.pDimDescription(i);
    pDim->initialize(*descr);

  }
  this->n_expanded_dim=0;
  for(size_t i=0;i<m_basis.getNumDims();i++){
    pDim=this->getDimension(i);
    if(!pDim->getIntegrated()){
      this->n_expanded_dim++;
    }
  }
  // if rotations are present, the directions of new transformed reciprocal dimensions are set as proper transformation of MDGeometryBasis dimensions;
  MantidMat Rot = trf.getRotations();
  MantidMat test(3,3);
  test.identityMatrix();
  // set the directions of the reciprocal dimensions availible as proper rotations of MDGeometry basis
  if(!Rot.equals(test,FLT_EPSILON)){
	  V3D newDirection;
	  std::map<std::string,boost::shared_ptr<MDDimension> >::iterator itDim; //=this->dimensions_map.begin();
	  std::set<MDBasisDimension> recBasDims = this->m_basis.getReciprocalDimensions();

	  std::set<MDBasisDimension>::iterator itBasis = recBasDims.begin();
	  for(;itBasis!=recBasDims.end();itBasis++){
		  std::string dimTag = itBasis->getId();
		  itDim = this->dimensions_map.find(dimTag);
		  if(itDim==this->dimensions_map.end()){
			  g_log.error()<<" Can not find the dimension with ID: "<<dimTag<<" among reciprocal dimensions; MDGeometryBasis and MDGeometry are inconsistent\n";
			  throw(std::logic_error("MDGeometryBasis and MDGeometry are inconsistent"));
		  }
		  newDirection = Rot*itBasis->getDirection();
		  itDim->second->setDirection(newDirection);
	  }

  
  }
  

}
std::vector<boost::shared_ptr<IMDDimension> >
MDGeometry::getDimensions(bool sort_by_bais)const
{
    unsigned int i;
	std::vector<boost::shared_ptr<IMDDimension> > dims(this->getNumDims());

    if(sort_by_bais){ // sort by basis;
       std::vector<std::string> dimID = this->getBasisTags();
       std::map<std::string,boost::shared_ptr<MDDimension> >::const_iterator it;

	    for(i=0;i<this->getNumDims();i++){
            it = dimensions_map.find(dimID[i]);
            if(it == dimensions_map.end()){
                 g_log.error()<<" MDGeometry::getDimensions: dimension with tag: "<<dimID[i]<<" does not exist in current geometry\n";
                 throw(std::logic_error("Geometry::getDimension: wrong dimension tag"));
            }
      
            dims[i] = it->second; 
	    }
    }else{ // sort by geometry
	    for(i=0;i<this->getNumDims();i++){
		    dims[i] = theDimension[i];
	    }
    }
	return dims;

}
    //
void 
MDGeometry::initialize(const MDGeometryDescription &trf)
{
    std::vector<std::string> dimID = trf.getDimensionsTags();
    this->initialize(dimID);
    this->setRanges(trf);
    this->arrangeDimensionsProperly(dimID);

}

    //
void 
MDGeometry::initialize(const std::vector<std::string> &DimensionTags)
{


  bool congruent_geometries(true);



  // are the old geometry congruent to the new geometry? e.g the same nuber of dimensions and the same dimension tags;
  if(DimensionTags.size()!=m_basis.getNumDims()){
    congruent_geometries=false;
  }else{
    congruent_geometries=m_basis.checkIdCompartibility(DimensionTags);
  }

  if(!congruent_geometries){
    g_log.error()<<"builing geometry with the basis different from the current geometry is prohibited\n";
    throw(std::invalid_argument("builing geometry with the basis different from the current geometry is prohibited"));
    // m_basis( = .initializeBasis(DimensionTags,nReciprocalDims);

    //// clear old dimensions if any
    //for(i=0;i<this->theDimension.size();i++){
    //  if(this->theDimension[i]){
    //    delete this->theDimension[i];
    //    theDimension[i]=NULL;
    //  }
    //}
    //this->init_empty_dimensions();
  }else{
    this->arrangeDimensionsProperly(DimensionTags);
  }

}
//
void 
MDGeometry::arrangeDimensionsProperly(const std::vector<std::string> &tags)
{
  size_t n_new_dims=tags.size();
  if(n_new_dims>m_basis.getNumDims()){
    g_log.error()<<"Geometry::arrangeDimensionsProperly: Attempting to arrange more dimensions then are currently defined \n";
    throw(std::invalid_argument("Geometry::arrangeDimensionsProperly: Attempting to arrange more dimensions then are currently defined "));
  }


  // array to keep final expanded dimensions
  std::vector<boost::shared_ptr<MDDimension> > pExpandedDims(m_basis.getNumDims());    
  // array to keep final collapsed dimensions which sould be placed after expanded
  std::vector<boost::shared_ptr<MDDimension> > pCollapsedDims(m_basis.getNumDims());  
  // array to keep thd initial dimensions which were not mentioned in transformation
  std::vector<boost::shared_ptr<MDDimension> > pCurrentDims(this->theDimension);  

  size_t n_expanded_dimensions(0),n_collapsed_dimensions(0);
  boost::shared_ptr<MDDimension> pDim;
  std::map<std::string,boost::shared_ptr<MDDimension> >::iterator it;

  // let's sort dimensions as requested by the list of tags
  for( size_t i=0;i<n_new_dims;i++){

    // when dimension num we want to use next
    it = dimensions_map.find(tags[i]);
    if(it==dimensions_map.end()){
      g_log.error()<<" The dimension with tag "<<tags[i]<<" does not belong to current geometry\n";
      throw(std::invalid_argument("Geometry::arrangeDimensionsProperly: new dimension requested but this function can not add new dimensions"));
    }
    // get the dimension itself
    pDim     = it->second;
    // clear map for future usage;
    dimensions_map.erase(it);

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
  for(it=dimensions_map.begin();it!=dimensions_map.end();it++){
    pDim = it->second;

    if(pDim->getIntegrated()){ // this is collapsed dimension;
      pCollapsedDims[n_collapsed_dimensions]=pDim;
      n_collapsed_dimensions++;
    }else{
      pExpandedDims[n_expanded_dimensions]  =pDim;
      n_expanded_dimensions++;
    }
  }
  // invalidate map which is not nedded any more;
  dimensions_map.clear();

  this->n_expanded_dim=n_expanded_dimensions;
  // total number of dimensions should not change;
  if(n_expanded_dimensions+n_collapsed_dimensions!=m_basis.getNumDims()){
    g_log.error()<<"Geometry::arrangeDimensionsProperly: Dimensions: n_expanded+n_collapsed!= nTotal; serious logical error";
    throw(Exception::NotImplementedError("Geometry::arrangeDimensionsProperly: Dimensions: n_expanded+n_collapsed!= nTotal; serious logical error"));
  }

  size_t dimension_stride=1;
  // deal with expanded dimensions
  for(size_t i=0;i<this->n_expanded_dim;i++){
    pDim  = pExpandedDims[i];

    // store the dimension in the vector and the map
    this->theDimension[i]=pDim;
    dimensions_map[pDim->getDimensionTag()]=pDim;

    // set integral dimensions characteristics;
    this->theDimension[i]->setStride(dimension_stride); 
    dimension_stride     *= this->theDimension[i]->getNBins();

  }
  nGeometrySize = dimension_stride;

  // now with collapsed dimensions;
  size_t ind(n_expanded_dim);
  for(size_t i=0;i<n_collapsed_dimensions;i++)
  {
    pDim  = pCollapsedDims[i];

    this->theDimension[ind+i]=pDim;
    dimensions_map[pDim->getDimensionTag()]=pDim;

    this->theDimension[ind+i]->setStride(0);
  }

}
//
    boost::shared_ptr<IMDDimension>
      MDGeometry::getYDimension(void)const
    {
      if(m_basis.getNumDims()<2){
        throw(std::invalid_argument("No Y dimension is defined in this workspace"));
      }
      return theDimension[1];
    }
    //
    boost::shared_ptr<IMDDimension> 
      MDGeometry::getZDimension(void)const
    {
      if(m_basis.getNumDims()<3){
        throw(std::invalid_argument("No Z dimension is defined in this workspace"));
      }
      return theDimension[2];
    }
    //
    boost::shared_ptr<IMDDimension>
      MDGeometry::getTDimension(void)const
    {
      if(m_basis.getNumDims()<4){
        throw(std::invalid_argument("No T dimension is defined in this workspace"));
      }
      return theDimension[3];
    }
    //
    std::vector<boost::shared_ptr<IMDDimension> >
      MDGeometry::getIntegratedDimensions(void)const
    {
      std::vector<boost::shared_ptr<IMDDimension> > tmp;

      if(this->n_expanded_dim<m_basis.getNumDims())
      {
        size_t size = m_basis.getNumDims()-this->n_expanded_dim;
        tmp.resize(size);
        size_t ic(0);
        for(size_t i = this->n_expanded_dim;i<theDimension.size();i++){
          tmp[ic] = theDimension[i];
          ic++;
        }
      }
      return tmp;
    }

//  protected;
boost::shared_ptr<MDDimension>
MDGeometry::getDimension(size_t i)
    {
	  
      if(i>=m_basis.getNumDims()){
        g_log.error()<<"Geometry::getDimension: attemting to get the dimension N"<<i<<" but this is out of the dimensions range";
        throw(std::out_of_range("Geometry::getDimension: attemting to get the dimension with non-existing number"));
      }
      return theDimension[i];
}
//  protected;
boost::shared_ptr<MDDimension>
MDGeometry::getDimension(const std::string &tag,bool do_throw)
{
  boost::shared_ptr<MDDimension> pDim;
  std::map<std::string,boost::shared_ptr<MDDimension> >::const_iterator it;
  it = dimensions_map.find(tag);
  if(it == dimensions_map.end()){
    if(do_throw){
      g_log.error()<<" MDGeometry::getDimension: dimension with tag: "<<tag<<" does not exist in current geometry\n";
      throw(std::invalid_argument("Geometry::getDimension: wrong dimension tag"));
    }else{
		return pDim;
	}
  }
  pDim = it->second;

  return pDim;
}

boost::shared_ptr<const MDDimension>
MDGeometry::get_constDimension(unsigned int i)const
{
	  
      if(i>=m_basis.getNumDims()){
        g_log.error()<<"Geometry::getDimension: attemting to get the dimension N"<<i<<" but this is out of the dimensions range";
        throw(std::out_of_range("Geometry::getDimension: attemting to get the dimension with non-existing number"));
      }
      return theDimension[i];
}
boost::shared_ptr<const MDDimension>
MDGeometry::get_constDimension(const std::string &tag,bool do_throw)const
{
      boost::shared_ptr<MDDimension> pDim;
      std::map<std::string,boost::shared_ptr<MDDimension> >::const_iterator it;
      it = dimensions_map.find(tag);
      if(it == dimensions_map.end()){
        if(do_throw){
          g_log.error()<<" MDGeometry::getDimension: dimension with tag: "<<tag<<" does not exist in current geometry\n";
          throw(std::invalid_argument("Geometry::getDimension: wrong dimension tag"));
        }else{
			return pDim;
		}
      }
      pDim = it->second;

      return pDim;
}

MDGeometry::MDGeometry(const MDGeometryBasis &basis, const MDGeometryDescription &description):
n_expanded_dim(0), nGeometrySize(0), m_basis(basis)
{
  this->theDimension.resize(basis.getNumDims());
  this->init_empty_dimensions();
  // arrange dimensions in accordence with the descriptions and sets dimension ranges
  this->initialize(description);
}

MDGeometry::MDGeometry(const MDGeometryBasis &basis) :
n_expanded_dim(0), nGeometrySize(0), m_basis(basis)
{
  this->theDimension.resize(basis.getNumDims());
  this->init_empty_dimensions();
}
//
MantidMat 
MDGeometry::getRotations()const
{
	MantidMat rez(3,3);
	if(this->m_basis.getNumReciprocalDims()==1){
		rez.identityMatrix();
		return rez;
	}
	std::vector<V3D> geomBasis = this->m_basis.get_constRecBasis();
	if(geomBasis.size()==2){
		V3D third = geomBasis[0].cross_prod(geomBasis[1]);
		geomBasis.push_back(third);
	}

	UnitCell fakeCell(geomBasis[0].norm(),geomBasis[1].norm(),geomBasis[2].norm());
	rez = fakeCell.getUmatrix(this->get_constDimension(0)->getDirection(),this->get_constDimension(1)->getDirection());
	if(geomBasis[2].scalar_prod(this->get_constDimension(2)->getDirection())<0){ // this should not happen
		g_log.information()<<"MDGeometry::getRotations: MDGeometry transformed into left-handed coordinate system; this arrangement have never been tested\n";
		MantidMat reflection(3,3);
		reflection.identityMatrix();
		reflection[2][2] = -1;
		rez*=reflection;
	}
	return rez;
}

std::string MDGeometry::toXMLString() const
{
  MDGeometryBuilderXML xmlBuilder;
  // Add all dimensions.
  for(size_t i = 0; i <this->theDimension.size(); i++)
  {
    xmlBuilder.addOrdinaryDimension(theDimension[i]);
  }
  // Add mapping dimensions
  xmlBuilder.addXDimension(getXDimension());
  xmlBuilder.addYDimension(getYDimension());
  xmlBuilder.addZDimension(getZDimension());
  xmlBuilder.addTDimension(getTDimension());
  // Create the xml.
  return xmlBuilder.create();
}

void 
MDGeometry::init_empty_dimensions()
 {
   std::set<MDBasisDimension> recID=this->m_basis.getReciprocalDimensions();
   std::set<MDBasisDimension> nonRecID=this->m_basis.getNonReciprocalDimensions();
   
   std::set<MDBasisDimension>::iterator it;
   
   unsigned int nRec_count(0),i(0);
   dimensions_map.clear();
   std::string tag;

  
   for(it=recID.begin();it!=recID.end();++it){
      tag = it->getId();
      this->theDimension[i] = boost::shared_ptr<MDDimension>(new MDDimensionRes(tag,(rec_dim)nRec_count));
       nRec_count++;
      // initiate map to search dimensions by names
      dimensions_map[tag]=this->theDimension[i];
      i++;
   }
    for(it=nonRecID.begin();it!=nonRecID.end();++it){
      tag = it->getId();
      this->theDimension[i] = boost::shared_ptr<MDDimension>(new MDDimension(tag));

      // initiate map to search dimensions by names
      dimensions_map[tag]=this->theDimension[i];
      i++;
   }
     // all dimensions initiated by default constructor are integrated;
    this->n_expanded_dim=0;
	this->nGeometrySize =0;

}

MDGeometry::~MDGeometry(void)
{
}

std::vector<std::string> 
MDGeometry::getBasisTags(void)const 
{
  std::vector<std::string> tags(this->m_basis.getNumDims()); 
  std::set<MDBasisDimension> basisDimensions = this->m_basis.getBasisDimensions(); 
  std::set<MDBasisDimension>::const_iterator it = basisDimensions.begin();
  for(;it != basisDimensions.end(); ++it)
  {  
    int i=  it->getColumnNumber();
    tags[i] = it->getId();
  }
  return tags;
}
//
bool 
MDGeometry::operator == (const MDGeometry &other)const
{
	if(this->getGeometryExtend()!=other.getGeometryExtend())return false;

	size_t nDims = this->getNumDims();
	if(nDims!=other.getNumDims())return false;

	for(size_t i=0;i<nDims;i++){
		if(*(this->get_constDimension(i))!=*other.get_constDimension(i))return false;
	}
	return true;
}
//
bool 
MDGeometry::operator != (const MDGeometry &other)const
{
	if(this->getGeometryExtend()!=other.getGeometryExtend())return true;

	size_t nDims = this->getNumDims();
	if(nDims!=other.getNumDims())return true;

	for(size_t i=0;i<nDims;i++){
		if(*(this->get_constDimension(i))!=*other.get_constDimension(i))return true;
	}
	return false;
}

} // end namespaces;
}
