#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Exception.h"

DECLARE_WORKSPACE(Workspace2D)

namespace Mantid
{
namespace DataObjects
{
  
Workspace2D::Workspace2D()
  /// Constructor
{ }

Workspace2D::Workspace2D(const Workspace2D& A) :
    data(A.data)
  /// Copy constructor
{ }

Workspace2D& 
Workspace2D::operator=(const Workspace2D& A)
{
  if (this!=&A)
    {
      data=A.data;
    }
  return *this;
}

Workspace2D::~Workspace2D()
{}

void 
Workspace2D::setHistogramNumber(const int nhist)
  /*!
    Set the histogram count.
    \todo FIX this can't be right since we have not dimensioned the internal arrays
  */
{
  if (nhist<0) 
    throw std::invalid_argument("Workspace2D::setHistogramNumber, invalid histograms number <0");
  data.resize(nhist);
}

long int 
Workspace2D::getMemorySize() const 
{ //to be changed
  return 0;
}

void 
Workspace2D::setX(const int histnumber, const std::vector<double>& Vec)
  /*!
    Set the x values
    \param histnumber :: Index to the histogram
    \param Vec :: Vec to set [Should be typedef]
   */
{
  if (histnumber<0 || histnumber>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::setX, histogram number out of range");
  
  data[histnumber].dataX()=Vec;
  return;
}

void 
Workspace2D::setX(const int histnumber, const Histogram1D::RCtype::ptr_type& Vec)
  /*!
    Set the x values
    \param histnumber :: Index to the histogram
    \param Vec :: Shared ptr base obect
   */
{
  if (histnumber<0 || histnumber>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::setX, histogram number out of range");
  
  data[histnumber].setX(Vec);
  return;
}

void 
Workspace2D::setData(const int histnumber, const std::vector<double>& Vec)
{
  if (histnumber<0 || histnumber>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::setDAta, histogram number out of range");

  data[histnumber].dataY()=Vec;
}

void 
Workspace2D::setData(const int histnumber, const std::vector<double>& Vec, 
		     const std::vector<double>& VecErr)
{
  if (histnumber<0 || histnumber>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::setDAta, histogram number out of range");

  data[histnumber].dataY()=Vec;
  data[histnumber].dataE()=VecErr;
  return;
}

void 
Workspace2D::setX(const int histnumber, const Histogram1D::RCtype& PA)
{
  if (histnumber<0 || histnumber>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::setX, histogram number out of range");

  data[histnumber].setX(PA);
  return;
}

void 
Workspace2D::setData(const int histnumber, const Histogram1D::RCtype& PY)
{
  if (histnumber<0 || histnumber>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::setX, histogram number out of range");

    data[histnumber].setData(PY);
}

void Workspace2D::setData(const int histnumber, const Histogram1D::RCtype& PY, 
			  const Histogram1D::RCtype& PE)
{
  if (histnumber<0 || histnumber>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::setX, histogram number out of range");

  data[histnumber].setData(PY,PE);
  return;
}

const int 
Workspace2D::getHistogramNumber() const
{
  return static_cast<const int>(data.size());
}

const std::vector<double>& 
Workspace2D::getX(int histnumber) const
{
  if (histnumber<0 || histnumber>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::getX, histogram number out of range");

  return data[histnumber].dataX();
}

const std::vector<double>& 
Workspace2D::getY(int histnumber) const
{
  if (histnumber<0 || histnumber>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::getY, histogram number out of range");

  return data[histnumber].dataY();
}

const std::vector<double>& 
Workspace2D::getE(int histnumber) const
{
  if (histnumber<0 || histnumber>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::getY, histogram number out of range");

  return data[histnumber].dataE();
}

} // namespace DataObjects
} //NamespaceMantid
