#ifndef MANTID_DATAOBJECTS_WORKSPACE2D_H_
#define MANTID_DATAOBJECTS_WORKSPACE2D_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Histogram1D.h"

namespace Mantid
{

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
namespace Kernel
{
  class Logger;
//  class Axis;
}
	
namespace DataObjects
{
/** \class Workspace2D
    
    Concrete workspace implementation. Data is a vector of Histogram1D.
    Since Histogram1D have share ownership of X, Y or E arrays, 
    duplication is avoided for workspaces for example with identical time bins.     	
    
    \author Laurent C Chapon, ISIS, RAL
    \date 26/09/2007

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

    This file is part of Mantid.
    
    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.
    
    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/ 	
class DLLExport Workspace2D : public API::Workspace
{
public:
  /// Typedef for the workspace_iterator to use with a Workspace2D
  typedef API::workspace_iterator<API::LocatedDataRef, Workspace2D> iterator;
  /// Typedef for the const workspace_iterator to use with a Workspace2D
  typedef API::workspace_iterator<const API::LocatedDataRef, const Workspace2D> const_iterator;  
  
  /**
  Gets the name of the workspace type
  \return Standard string name
   */
  virtual const std::string id() const {return "Workspace2D";}

  Workspace2D();
  virtual ~Workspace2D();

  virtual void init(const int &NVectors, const int &XLength, const int &YLength);
  
  virtual void setX(int const, const std::vector<double>&);
  virtual void setData(int const, const std::vector<double>&);
  virtual void setData(int const, const std::vector<double>&, const std::vector<double>&);
  virtual void setData(int const, const std::vector<double>&, const std::vector<double>&,
    const std::vector<double>&);
  virtual void setX(int const, const Histogram1D::RCtype&);
  virtual void setX(int const, const Histogram1D::RCtype::ptr_type&);
  virtual void setData(int const, const Histogram1D::RCtype&);
  virtual void setData(int const, const Histogram1D::RCtype&, const Histogram1D::RCtype&);
  virtual void setData(int const, const Histogram1D::RCtype&, const Histogram1D::RCtype&, 
    const Histogram1D::RCtype&);
  virtual void setData(int const, const Histogram1D::RCtype::ptr_type&, const Histogram1D::RCtype::ptr_type&);
  virtual void setData(int const, const Histogram1D::RCtype::ptr_type&, const Histogram1D::RCtype::ptr_type&,
    const Histogram1D::RCtype::ptr_type&);

  
  /// Returns the histogram number
  const int getHistogramNumber() const;

  //section required for iteration
  virtual int size() const;       
  virtual int blocksize() const;
  virtual std::vector<double>& dataX(int const index);
  ///Returns the y data
  virtual std::vector<double>& dataY(int const index);
  ///Returns the error data
  virtual std::vector<double>& dataE(int const index);
  ///Returns the error data
  virtual std::vector<double>& dataE2(int const index);


  //Get methods return the histogram number 
  /// Returns the x data const
  virtual const std::vector<double>& dataX(int const index) const;
  /// Returns the y data const
  virtual const std::vector<double>& dataY(int const index) const;
  /// Returns the error const 
  virtual const std::vector<double>& dataE(int const index) const;
  /// Returns the error const 
  virtual const std::vector<double>& dataE2(int const index) const;

  virtual long int getMemorySize() const;

  ///Returns the ErrorHelper applicable for this spectra
  virtual const API::IErrorHelper* errorHelper(int const index) const;
  ///Sets the ErrorHelper for this spectra
  virtual void setErrorHelper(int const index,API::IErrorHelper* errorHelper);
  ///Sets the ErrorHelper for this spectra
  virtual void setErrorHelper(int const index,const API::IErrorHelper* errorHelper);

  //Methods for getting data via python. Do not use for anything else!
  ///Returns the x data const
  virtual const std::vector<double>& getX(int const index) const;
  ///Returns the y data const
  virtual const std::vector<double>& getY(int const index) const;
  ///Returns the error const
  virtual const std::vector<double>& getE(int const index) const;
  ///Returns the error const
  virtual const std::vector<double>& getE2(int const index) const;

protected:
  /// The number of vectors in the workspace
  int m_noVectors;
  
private:
  /// Private copy constructor. NO COPY ALLOWED
  Workspace2D(const Workspace2D&);
  /// Private copy assignment operator. NO ASSIGNMENT ALLOWED
  Workspace2D& operator=(const Workspace2D&);

  virtual const int getHistogramNumberHelper() const;
  
  /// A vector that holds the 1D histograms 
  std::vector<Histogram1D> data;

  /// Static reference to the logger class
  static Kernel::Logger &g_log;
};

///shared pointer to the Workspace2D class
typedef boost::shared_ptr<Workspace2D> Workspace2D_sptr;

} // namespace DataObjects
} // Namespace Mantid 
#endif /*MANTID_DATAOBJECTS_WORKSPACE2D_H_*/
