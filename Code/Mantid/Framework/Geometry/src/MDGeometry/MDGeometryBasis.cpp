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
//
MDGeometryBasis::MDGeometryBasis(unsigned int nDimensions, unsigned int nReciprocalDimensions) :
n_total_dim(nDimensions), n_reciprocal_dimensions(nReciprocalDimensions), m_mdBasisDimensions()

{
 // this constructor is dummy where all data have to be owerwtirren
  this->check_nDims(n_total_dim, n_reciprocal_dimensions);
  m_mdBasisDimensions.insert(MDBasisDimension("q0", true, 0));
}
//
void
MDGeometryBasis::init(const std::set<MDBasisDimension>& mdBasisDimensions,boost::shared_ptr<UnitCell> theSample)
{ 
	spSample = theSample;

    m_mdBasisDimensions.clear();
    m_mdBasisDimensions = mdBasisDimensions;
    this->n_total_dim   = mdBasisDimensions.size();

    this->n_reciprocal_dimensions = 0;
    std::set<MDBasisDimension>::const_iterator it = mdBasisDimensions.begin();
    for( ;it != mdBasisDimensions.end(); ++it){  
        checkInputBasisDimensions(*(it)); // Check that duplicate column numbers have not been used.
        // check if column number is smaller then the number of dimensions
        if(it->getColumnNumber() >= (int)this->n_total_dim){
            g_log.error()<<" the number of the dimension with id: "<<it->getId()<<" is "<<it->getColumnNumber() <<" and it is higher then the number of dimensions\n";
            throw(std::invalid_argument(" the dimension number is higher then total number of dimensions"));
        }
      if((*it).getIsReciprocal()){
          n_reciprocal_dimensions++;
        }
    }
    this->check_nDims(n_total_dim , n_reciprocal_dimensions);
}

MDGeometryBasis::MDGeometryBasis(const std::set<MDBasisDimension>& mdBasisDimensions,boost::shared_ptr<UnitCell> theSample):
n_reciprocal_dimensions(0), m_mdBasisDimensions(mdBasisDimensions)
{
   this->init(mdBasisDimensions,theSample);
}

    void MDGeometryBasis::checkInputBasisDimensions(const MDBasisDimension&  dimension)
    {
      std::set<MDBasisDimension>::const_iterator it = m_mdBasisDimensions.begin();
      for( ;it != m_mdBasisDimensions.end(); ++it){
        if(dimension != *it && it->getColumnNumber()==dimension.getColumnNumber())
        {
          //Record the error and throw.
          g_log.error()<<" two duplicated column numbers found for dimensions id:"<<it->getId()<< " and Id"<<dimension.getId()<<std::endl; 
          throw(std::logic_error("Cannot have duplicated column numbers"));
        }
      }
    }

    void 
    MDGeometryBasis::check_nDims(unsigned int nDimensions,unsigned int nReciprocalDimensions)
    {
      if(nReciprocalDimensions<1||nReciprocalDimensions>3){
        g_log.error()<<"MDGeometryBasis::MDGeometryBasis(unsigned int nDimensions,unsigned int nReciprocalDimensions): number of reciprocal dimensions can vary from 1 to 3 but attempted "<<nReciprocalDimensions<<std::endl;
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
MDGeometryBasis::checkIdCompartibility(const std::vector<std::string> &newTags)const
{
     std::set<MDBasisDimension>::const_iterator it = m_mdBasisDimensions.begin();
     std::set<std::string> existingTags;
     for(;it!=m_mdBasisDimensions.end();it++){
            existingTags.insert(it->getId());
     }

     for(unsigned int i=0;i<newTags.size();i++){
         std::set<std::string>::const_iterator its = existingTags.find(newTags[i]);
          if(its==existingTags.end()){
                return false;
          }
     }
     return true;
}

    std::set<MDBasisDimension> MDGeometryBasis::getReciprocalDimensions() const
    {
      std::set<MDBasisDimension> reciprocalDims;
      std::set<MDBasisDimension>::const_iterator it = m_mdBasisDimensions.begin();
      for( ;it != m_mdBasisDimensions.end(); ++it)
      {  
        if((*it).getIsReciprocal())
        {
          reciprocalDims.insert((*it));
        }
      }
      return reciprocalDims;
    }

    std::set<MDBasisDimension> MDGeometryBasis::getNonReciprocalDimensions() const
    {
      std::set<MDBasisDimension> nonReciprocalDims;
      std::set<MDBasisDimension>::const_iterator it = m_mdBasisDimensions.begin();
      for( ;it != m_mdBasisDimensions.end(); ++it)
      {  
        if(!(*it).getIsReciprocal())
        {
          nonReciprocalDims.insert((*it));
        }
      }
      return nonReciprocalDims;
    }

    std::set<MDBasisDimension> MDGeometryBasis::getBasisDimensions() const
    {
      return  std::set<MDBasisDimension>(this->m_mdBasisDimensions);
    }

    MDGeometryBasis::~MDGeometryBasis(void)
    {
    }

  }
}
