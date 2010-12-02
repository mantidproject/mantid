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
MDGeometryBasis::MDGeometryBasis(unsigned int nDimensions,unsigned int nReciprocalDimensions):
n_total_dim(nDimensions),n_reciprocal_dimensions(nReciprocalDimensions)
{
	
    this->check_nDims(n_total_dim , n_reciprocal_dimensions);
	m_mdBasisDimensions.insert(MDBasisDimension("q0",true,0));
}
//
void
MDGeometryBasis::init(const std::set<MDBasisDimension>& mdBasisDimensions, const UnitCell &cell)
{
	m_cell = cell;
	m_mdBasisDimensions.clear();
	m_mdBasisDimensions = mdBasisDimensions;

   this->n_total_dim = mdBasisDimensions.size();
      std::set<MDBasisDimension>::const_iterator it = mdBasisDimensions.begin();
      for(it;it != mdBasisDimensions.end(); ++it)
      {  
        checkInputBasisDimensions(*(it)); // Check that duplicate column numbers have not been used.

        if((*it).getIsReciprocal())
        {
          n_reciprocal_dimensions++;
        }
      }
      this->check_nDims(n_total_dim , n_reciprocal_dimensions);
}

MDGeometryBasis::MDGeometryBasis(const std::set<MDBasisDimension>& mdBasisDimensions, const UnitCell &cell) : m_cell(cell), m_mdBasisDimensions(mdBasisDimensions), n_reciprocal_dimensions(0)
{
   this->init(mdBasisDimensions,cell);
}

    void MDGeometryBasis::checkInputBasisDimensions(const MDBasisDimension&  dimension)
    {
      std::set<MDBasisDimension>::const_iterator it = m_mdBasisDimensions.begin();
      for(it;it != m_mdBasisDimensions.end(); ++it){
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
      MDGeometryBasis::checkIdCompartibility(const std::vector<std::string> &newTags)const
    {
      for(unsigned int i=0;i<newTags.size();i++){

        MDBasisDimension tDim(newTags[i],true,-1);
        std::set<MDBasisDimension>::const_iterator it = m_mdBasisDimensions.find(tDim);
        if(it==m_mdBasisDimensions.end()){
          return false;
        }
      }
      return true;
    }

    std::set<MDBasisDimension> MDGeometryBasis::getReciprocalDimensions() const
    {
      std::set<MDBasisDimension> reciprocalDims;
      std::set<MDBasisDimension>::const_iterator it = m_mdBasisDimensions.begin();
      for(it;it != m_mdBasisDimensions.end(); ++it)
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
      for(it;it != m_mdBasisDimensions.end(); ++it)
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
