#ifndef MANTID_DATAOBJECTS_ALGORITHMPARAMETER_H_
#define MANTID_DATAOBJECTS_ALGORITHMPARAMETER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include <string>
namespace Mantid
{
namespace API
{


/** @class AlgorithmParameter AlgorithmParameter.h API/MAntidAPI/AlgorithmParameter.h

    This class stores information about the parameters used by an algorithm.

    @author Dickon Champion, ISIS, RAL
    @date 21/01/2008
    
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
class DLLExport AlgorithmParameter
{
public:
  AlgorithmParameter();
  AlgorithmParameter(const std::string& , const std::string&, const std::string& , const bool& , const unsigned int&);
  AlgorithmParameter(const AlgorithmParameter&);
  AlgorithmParameter& operator=(const AlgorithmParameter&);
	virtual ~AlgorithmParameter();

private:
  /// The name of the parameter
  std::string m_name;
  /// The value of the parameter
  std::string m_value;
  /// The type of the parameter
  std::string m_type;
  /// flag defining if the parameter is a default or a user-defined parameter
  bool m_isDefault;
  unsigned int m_direction;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_DATAOBJECTS_ALGORITHMPARAMETER_H_*/
