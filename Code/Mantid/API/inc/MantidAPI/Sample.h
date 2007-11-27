#ifndef MANTID_DATAOBJECTS_SAMPLE_H_
#define MANTID_DATAOBJECTS_SAMPLE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/PropertyManager.h"

namespace Mantid
{
namespace API
{

using Kernel::Property;

/** @class Sample Sample.h DataObjects/Sample.h

    This class stores information about the sample used in a particular experimental run.
    This is mainly garnered from logfiles.

    @author Russell Taylor, Tessella Support Services plc
    @date 26/11/2007
    
    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
class DLLExport Sample
{
public:
	Sample();
	virtual ~Sample();
	
	void setName( const std::string &name );
	const std::string& getName() const;
	
	void addLogData( Property *p );
	Property* getLogData( const std::string &name ) const;
	const std::vector< Property* >& getLogData() const;
	
private:
  /// The name for the sample
  std::string m_name;
  /// Stores the information read in from the logfiles.
  Kernel::PropertyManager m_manager;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_DATAOBJECTS_SAMPLE_H_*/
