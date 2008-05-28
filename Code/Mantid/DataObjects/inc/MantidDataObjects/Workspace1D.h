#ifndef MANTID_DATAOBJECTS_WORKSPACE1D_H_
#define MANTID_DATAOBJECTS_WORKSPACE1D_H_

#include "MantidAPI/Workspace.h"
#include "MantidAPI/IErrorHelper.h"
#include "MantidDataObjects/Histogram1D.h"

namespace Mantid
{

namespace Kernel
{
  class Logger;
}
	
namespace DataObjects
{

/** Concrete workspace implementation. Data is a Histogram1D      	
    @author Laurent C Chapon, ISIS, RAL
    @date 26/09/2007 	
    
    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratories

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
class DLLExport Workspace1D : public API::Workspace, public Histogram1D
{

public:
  /// Typedef for the workspace_iterator to use with a Workspace1D
  typedef API::workspace_iterator<API::LocatedDataRef, Workspace1D> iterator;
  /// Typedef for the const workspace_iterator to use with a Workspace1D
  typedef API::workspace_iterator<const API::LocatedDataRef, const Workspace1D> const_iterator;  
  
  /**
  	Gets the name of the workspace type
  	\return Standard string name
  */
  virtual const std::string id() const {return "Workspace1D";}

  Workspace1D();
  virtual ~Workspace1D();
  // allocates space in a new workspace
  virtual void init(const int &NVectors, const int &XLength, const int &YLength);

  //section required for iteration
  ///Returns the number of single indexable items in the workspace
  virtual int size() const;
  //set blocksize to a very large number as 1D workspace has only one block
  ///Returns the size of each block of data returned by the dataX accessors
  virtual int blocksize() const;
  
  const int getHistogramNumber() const { return 1; }

  //inheritance redirections
  ///Returns the x data
  virtual std::vector<double>& dataX(int const index) { return Histogram1D::dataX(); }
  ///Returns the y data
  virtual std::vector<double>& dataY(int const index) { return Histogram1D::dataY(); }
  ///Returns the error data
  virtual std::vector<double>& dataE(int const index) { return Histogram1D::dataE(); }
  ///Returns the error data
  virtual std::vector<double>& dataE2(int const index) { return Histogram1D::dataE2(); }


  //inheritance redirections
  ///Returns non-const vector of the x data
  virtual std::vector<double>& dataX() { return Histogram1D::dataX(); }
  ///Returns non-const vector of the y data
  virtual std::vector<double>& dataY() { return Histogram1D::dataY(); }
  ///Returns non-const vector of the error data
  virtual std::vector<double>& dataE() { return Histogram1D::dataE(); }
  ///Returns non-const vector of the error data
  virtual std::vector<double>& dataE2() { return Histogram1D::dataE2(); }
  /// Returns the x data const
  virtual const std::vector<double>& dataX() const { return Histogram1D::dataX(); }  
  /// Returns the y data const
  virtual const std::vector<double>& dataY() const { return Histogram1D::dataY(); }
  /// Returns the error data const
  virtual const std::vector<double>& dataE() const { return Histogram1D::dataE(); }
  /// Returns the error data const
  virtual const std::vector<double>& dataE2() const { return Histogram1D::dataE2(); }

  /// Returns the x data const
  virtual const std::vector<double>& dataX(int const index) const {return dataX();}
  /// Returns the y data const
  virtual const std::vector<double>& dataY(int const index) const {return dataY();}
  /// Returns the error const
  virtual const std::vector<double>& dataE(int const index) const {return dataE();}
  /// Returns the error const
  virtual const std::vector<double>& dataE2(int const index) const {return dataE2();}

  ///Returns the ErrorHelper applicable for this detector
  virtual const API::IErrorHelper* errorHelper(int const index) const { return Histogram1D::errorHelper(); }
  ///Sets the ErrorHelper for this spectra
  virtual void setErrorHelper(int const index,API::IErrorHelper* errorHelper) { Histogram1D::setErrorHelper(errorHelper); }
   ///Sets the ErrorHelper for this spectra
  virtual void setErrorHelper(int const index,const API::IErrorHelper* errorHelper) { Histogram1D::setErrorHelper(errorHelper); }
  ///Returns the spectrum number to which this workspace refers
  virtual int spectraNo(int const index) const { return Histogram1D::spectraNo(); }
  ///Returns the spectrum number to which this workspace refers
  virtual int& spectraNo(int const index){ return Histogram1D::spectraNo(); }

  //Methods for getting data via python. Do not use for anything else!
  ///Returns the x data const
  virtual const std::vector<double>& getX(int const index) const {return dataX();}
  ///Returns the y data const
  virtual const std::vector<double>& getY(int const index) const {return dataY();}
  ///Returns the error const
  virtual const std::vector<double>& getE(int const index) const {return dataE();}
  ///Returns the error const
  virtual const std::vector<double>& getE2(int const index) const {return dataE2();}
  
private:
  /// Private copy constructor. NO COPY ALLOWED
  Workspace1D(const Workspace1D&);
  /// Private copy assignment operator. NO ASSIGNMENT ALLOWED
  Workspace1D& operator=(const Workspace1D&);

  /// Static reference to the logger class
  static Kernel::Logger &g_log;
};

  ///shared pointer to the Workspace1D class
  typedef boost::shared_ptr<Workspace1D> Workspace1D_sptr;

} // namespace DataObjects

} // namespace Mantid

#endif /*MANTID_DATAOBJECTS_WORKSPACE1D_H_*/
