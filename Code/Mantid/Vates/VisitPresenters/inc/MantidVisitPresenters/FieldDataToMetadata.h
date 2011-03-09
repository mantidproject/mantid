#ifndef FIELDDATA_TO_METADATA_H_
#define FIELDDATA_TO_METADATA_H_

#include <functional>
#include <string>
#include "MantidKernel\System.h"

class vtkFieldData;
namespace Mantid
{
namespace VATES
{

/**
 * Functor Converts fielddata of type vtkFieldData to metadata (std::string).
 *
 @author Owen Arnold, Tessella plc
 @date 09/02/2011

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

class DLLExport FieldDataToMetadata : public std::binary_function<vtkFieldData*, std::string, std::string>
{
public:
  /// Act as Functor.
  std::string operator()(vtkFieldData* fieldData, std::string id) const;

  /// Explicit call to Functor execution.
  std::string execute(vtkFieldData* fieldData,const std::string& id) const;
};

}
}

#endif
