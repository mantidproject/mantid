#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/TripleRef.h"
#include "MantidAPI/TripleIterator.h"
#include "MantidAPI/TripleIteratorCode.h"
#include "MantidAPI/WorkspaceProperty.h"

DECLARE_WORKSPACE(Workspace2D)

namespace Mantid
{
namespace DataObjects
{
/// Constructor
Workspace2D::Workspace2D()
{ }

/// Copy constructor
Workspace2D::Workspace2D(const Workspace2D& A) :
    data(A.data)
{ }

/// Assignment operator
Workspace2D& 
Workspace2D::operator=(const Workspace2D& A)
{
  if (this!=&A)
    {
      data=A.data;
    }
  return *this;
}

 ///Destructor
Workspace2D::~Workspace2D()
{}

/**
    Set the histogram count.
    \todo FIX this can't be right since we have not dimensioned the internal arrays
    \param nhist The number of histograms
  */
void 
Workspace2D::setHistogramNumber(const int nhist)
{
  if (nhist<0) 
    throw std::invalid_argument("Workspace2D::setHistogramNumber, invalid histograms number <0");
  data.resize(nhist);
}

/**
  Get the amount of memory used by the 2D workspace
  \todo to be changed
*/
long int 
Workspace2D::getMemorySize() const 
{
  return 0;
}

/**
    Set the x values
    \param histnumber :: Index to the histogram
    \param Vec :: Vec to set [Should be typedef]
   */
void 
Workspace2D::setX(const int histnumber, const std::vector<double>& Vec)
{
  if (histnumber<0 || histnumber>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::setX, histogram number out of range");
  
  data[histnumber].dataX()=Vec;
  return;
}

  /**
    Set the x values
    \param histnumber :: Index to the histogram
    \param Vec :: Shared ptr base object
   */
void 
Workspace2D::setX(const int histnumber, const Histogram1D::RCtype::ptr_type& Vec)
{
  if (histnumber<0 || histnumber>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::setX, histogram number out of range");
  
  data[histnumber].setX(Vec);
  return;
}

 /**
    Set the x values
    \param histnumber :: Index to the histogram
    \param PA :: Reference counted histogram
   */
void 
Workspace2D::setX(const int histnumber, const Histogram1D::RCtype& PA)
{
  if (histnumber<0 || histnumber>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::setX, histogram number out of range");

  data[histnumber].setX(PA);
  return;
}

/**
    Sets the data in the workspace
	\param histnumber The histogram to be set
	\param Vec A vector containing the data	
*/
void 
Workspace2D::setData(const int histnumber, const std::vector<double>& Vec)
{
  if (histnumber<0 || histnumber>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::setDAta, histogram number out of range");

  data[histnumber].dataY()=Vec;
}

/**
    Sets the data in the workspace (including errors)
	\param histnumber The histogram to be set
	\param Vec A vector containing the data	
	\param VecErr A vector containing the corresponding errors
*/
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

/**
    Sets the data in the workspace
	\param histnumber The histogram to be set
	\param PY A reference counted data range	
*/
void 
Workspace2D::setData(const int histnumber, const Histogram1D::RCtype& PY)
{
  if (histnumber<0 || histnumber>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::setX, histogram number out of range");

    data[histnumber].setData(PY);
}

/**
    Sets the data in the workspace
	\param histnumber The histogram to be set
	\param PY A reference counted data range	
	\param PE A reference containing the corresponding errors
*/
void Workspace2D::setData(const int histnumber, const Histogram1D::RCtype& PY, 
			  const Histogram1D::RCtype& PE)
{
  if (histnumber<0 || histnumber>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::setX, histogram number out of range");

  data[histnumber].setData(PY,PE);
  return;
}


/**
    Sets the data in the workspace
	\param histnumber The histogram to be set
	\param PY A reference counted data range	
	\param PE A reference containing the corresponding errors
*/
void Workspace2D::setData(const int histnumber, const Histogram1D::RCtype::ptr_type& PY, 
			  const Histogram1D::RCtype::ptr_type& PE)
{
  if (histnumber<0 || histnumber>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::setX, histogram number out of range");

  data[histnumber].setData(PY,PE);
  return;
}


/** Gets the number of histograms
	\return Integer
*/
const int 
Workspace2D::getHistogramNumber() const
{
  return static_cast<const int>(data.size());
}

/**
	Get the x data of a specified histogram
	\param histnumber The number of the histogram
	\return A vector of doubles containing the x data
*/
const std::vector<double>& 
Workspace2D::getX(int histnumber) const
{
  if (histnumber<0 || histnumber>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::getX, histogram number out of range");

  return data[histnumber].dataX();
}

/**
	Get the y data of a specified histogram
	\param histnumber The number of the histogram
	\return A vector of doubles containing the y data
*/
const std::vector<double>& 
Workspace2D::getY(int histnumber) const
{
  if (histnumber<0 || histnumber>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::getY, histogram number out of range");

  return data[histnumber].dataY();
}

/**
	Get the error data for a specified histogram
	\param histnumber The number of the histogram
	\return A vector of doubles containing the error data
*/
const std::vector<double>& 
Workspace2D::getE(int histnumber) const
{
  if (histnumber<0 || histnumber>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::getY, histogram number out of range");

  return data[histnumber].dataE();
}

/// get pseudo size
int Workspace2D::size() const 
{ 
  int retVal = data.size();
  //if not empty
  if (retVal > 0)
  {
    //get the first entry multiply by its size
    retVal *= data[0].size();
  }
  return retVal; 
} 

///get the size of each vector
int Workspace2D::blocksize() const
{
  int retVal = 1000000000;
  //if not empty
  if (data.size() > 0)
  {
    //set the reteurn value to the length of the first vector
    retVal = data[0].size();
  }
  return retVal; 
}

std::vector<double>& Workspace2D::dataX(int const index)
{
  if (index<0 || index>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::dataX, histogram number out of range");

  return data[index].dataX();
}
///Returns the y data
std::vector<double>& Workspace2D::dataY(int const index)
{
  if (index<0 || index>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::dataY, histogram number out of range");

  return data[index].dataY();
}
///Returns the error data
std::vector<double>& Workspace2D::dataE(int const index)
{
  if (index<0 || index>=static_cast<int>(data.size()))
    throw std::range_error("Workspace2D::dataE, histogram number out of range");

  return data[index].dataE();
}

} // namespace DataObjects
} //NamespaceMantid


///\cond TEMPLATE
template DLLExport class Mantid::API::triple_iterator<Mantid::DataObjects::Workspace2D>;

template DLLExport class Mantid::API::WorkspaceProperty<Mantid::DataObjects::Workspace2D>;
///\endcond TEMPLATE
