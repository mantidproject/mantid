#ifndef MANTID_KERNEL_STATUSCODE_H_
#define MANTID_KERNEL_STATUSCODE_H_

namespace Mantid
{
namespace Kernel
{
/** @class StatusCode StatusCode.h Kernel/StatusCode.h

    This class is used for returning status codes from appropriate routines.

    @author Russell Taylor, Tessella Support Services plc
    @author Based on the Gaudi class of the same name (see http://proj-gaudi.web.cern.ch/proj-gaudi/)
    @date 12/09/2007
    
    Copyright &copy; 2007 ???RAL???

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
*/
// RJT: Minimal implementation at this point.
  class DLLExport StatusCode 
  {
  public:
    enum {
      FAILURE,
      SUCCESS,
      RECOVERABLE
    };

    // Gaudi has a default constructor which sets the status to success.
    // I am going to require that that status is explicitly given in the constructor.
//    StatusCode();
    
    /// Constructor.
    StatusCode( unsigned long code, bool checked = false );
    
    /** Test for a status code of FAILURE.
    * N.B. This is a specific type of failure where there aren't any more
    * appropriate staus codes. To test for any failure use :
    * if ( !StatusCode.isSuccess() ) ...
    */
    bool isFailure() const;
    
  protected:     // Why protected & not private?
	unsigned long   d_code;      ///< The status code
  // RJT: Don't use these next two variables at present, but including them means I can leave the constructors unchanged
	mutable bool    m_checked;   ///< If the Status code has been checked
	int m_severity;              ///< The seriousness of an error (unused)
    
  };
  
  inline StatusCode::StatusCode( unsigned long code, bool checked ) : 
	  d_code(code),m_checked(checked), m_severity(0) {}
  
  inline bool StatusCode::isFailure() const {
    m_checked = true;
    return (d_code != SUCCESS );
  }

} // namespace Kernel  
} // namespace Mantid

#endif /*MANTID_KERNEL_STATUSCODE_H_*/
