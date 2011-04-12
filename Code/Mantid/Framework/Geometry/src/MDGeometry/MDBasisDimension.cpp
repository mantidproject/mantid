#include "MantidGeometry/MDGeometry/MDBasisDimension.h"
#include <cfloat>

namespace Mantid
{
namespace Geometry
{

MDBasisDimension::MDBasisDimension(std::string id, bool isReciprocal, int columnNumber,const V3D &inDirection,const std::string &UnitID) :
m_id(id), m_isReciprocal(isReciprocal), m_columnNumber(columnNumber),
direction(inDirection)
{
	if(this->direction.norm2()<FLT_EPSILON){ // direction has to be derived from the column number
		 if(isReciprocal){
			 if(columnNumber<0||columnNumber>3){
					throw(std::invalid_argument("Reciprocal dimension's column numbers have to be 0,1 or 2"));
			 }else{
				// default length sould be 1 though there are no real lattice with such parameters
					this->direction[columnNumber]=1;
			 }
		 }else{
			 // an ortogonal direction is always 1 in direction[0];
			 this->direction[0]=1;
		 }

    }

	if(isReciprocal){
		 spUnit = Kernel::UnitFactory::Instance().create("MomentumTransfer");
		
		 if(this->direction.norm2()!= 1){
			 throw(std::invalid_argument("The module of basis reciprocal dimension has to be 1"));
		 }
	 }else{
		 if(abs(this->direction.norm()-this->direction[0])>FLT_EPSILON){
			 throw(std::invalid_argument("The basis orthogonal dimension have to be directed along 0 in V3D"));

		 }
		 this->spUnit = Kernel::UnitFactory::Instance().create(UnitID);
	
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
