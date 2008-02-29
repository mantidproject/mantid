#ifndef MANTID_KERNEL_GAUSSIANERRORHELPER_H_
#define MANTID_KERNEL_GAUSSIANERRORHELPER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include "MantidAPI/IErrorHelper.h"

namespace Mantid
{
namespace API
{


/** @class GaussianErrorHelper GaussianErrorHelper.h Kernel/GaussianErrorHelper.h

    A helper class for calculating the error values using Gaussian errors.
    WARNING:  This class is NOT thread safe.

    @author Russell Taylor, Tessella Support Services plc
    @date 24/01/2008
    
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
  class DLLExport GaussianErrorHelper : public IErrorHelper
{
public:
  /// A static method which retrieves the single instance of the GaussianErrorHelper
  static GaussianErrorHelper* Instance();

  ///Typedef for the value_type of the ErrorHelper
  typedef IErrorHelper::value_type value_type;

  ///Performs Gaussian addition
  const void plus (const value_type& lhs,const value_type& rhs, value_type& result) const;
  ///Performs Gaussian subtraction
  const void minus (const value_type& lhs,const value_type& rhs, value_type& result) const;
  ///Performs Gaussian multiplication
  const void multiply (const value_type& lhs,const value_type& rhs, value_type& result) const;
  ///Performs Gaussian division
  const void divide (const value_type& lhs,const value_type& rhs, value_type& result) const;

private:
	// Private constructor & destructor for singleton class
	GaussianErrorHelper();
	static GaussianErrorHelper* m_instance; ///< Pointer to the GaussianErrorHelper instance
  /// Static reference to the logger class
	static Kernel::Logger& g_log;

};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_GAUSSIANERRORHELPER_H_*/
