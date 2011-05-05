#include "MantidGeometry/MDGeometry/MDBasisDimension.h"
#include <cfloat>

namespace Mantid
{
namespace Geometry
{

MDBasisDimension::MDBasisDimension(std::string id, bool isReciprocal, int columnNumber,const std::string &UnitID,const V3D &inDirection) :
m_id(id), m_isReciprocal(isReciprocal), m_columnNumber(columnNumber),
direction(inDirection)
{
	if(this->direction.norm2()<FLT_EPSILON){ // direction has to be derived from the column number
		 if(isReciprocal){
			 if(columnNumber<0||columnNumber>3){
					throw(std::invalid_argument("Reciprocal dimension's column numbers have to be 0,1 or 2"));
			 }else{
				// default length sould be 0 as these directions are orthogonal to real 3D spaceeters
       				direction[0] = 0;
					direction[1] = 0;
					direction[2] = 0;
					direction[columnNumber] = 1;
	
			 }
		 }else{
			// default length of non-reciprocal dimension sould be 0 as these directions are orthogonal to real 3D spaceeters
  				direction[0] = 0;
				direction[1] = 0;
				direction[2] = 0;
		 }

    }

	if(isReciprocal){
		if(UnitID==""){
			spUnit = Kernel::UnitFactory::Instance().create("MomentumTransfer");
		}else{
			spUnit = Kernel::UnitFactory::Instance().create(UnitID);
		}
		// the length of basis reciprocal dimension is not 1 for non-cubic reciprocal lattice
		 if(fabs(this->direction.norm2())<FLT_EPSILON){
			 throw(std::invalid_argument("The module of basis reciprocal dimension can not be 0"));
		 }
	 }else{
		 if(this->direction.norm2()>FLT_EPSILON){
				 throw(std::invalid_argument("The module of basis orthogonal dimension has to be 0"));
		 }
		 if(UnitID==""){
			this->spUnit =  Kernel::UnitFactory::Instance().create("DeltaE");
		 }else{
			this->spUnit = Kernel::UnitFactory::Instance().create(UnitID);
		 }
	 }
	
}

    bool MDBasisDimension::operator==(const MDBasisDimension &other) const
    {
		return this->m_id == other.m_id&&this->spUnit==other.spUnit&&this->direction==other.direction;
    }

    bool MDBasisDimension::operator!=(const MDBasisDimension &other) const
    {
      return !(*this == other);
    }

    bool MDBasisDimension::operator < (const MDBasisDimension &other) const
    {
      return this->m_id < other.m_id;
    }

    std::string MDBasisDimension::getId() const 
    {
      return this->m_id;
    }

    bool MDBasisDimension::getIsReciprocal() const 
    {
      return this->m_isReciprocal;
    }

    int MDBasisDimension::getColumnNumber() const 
    {
      return this->m_columnNumber;
    }
  }
}
