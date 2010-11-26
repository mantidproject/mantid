#include "MantidGeometry/MDGeometry/MDBasisDimension.h"

namespace Mantid
{
  namespace Geometry
  {

    MDBasisDimension::MDBasisDimension(std::string id, bool isReciprocal, int columnNumber) : m_id(id), m_isReciprocal(isReciprocal), m_columnNumber(columnNumber)
    {
    }

    bool MDBasisDimension::operator==(const MDBasisDimension &other) const
    {
      return this->m_id == other.m_id;
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