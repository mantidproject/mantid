#ifndef DIMENSION_PARAMETER_NO_INTEGRATION_H
#define DIMENSION_PARAMETER_NO_INTEGRATION_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include "MantidKernel/System.h"
#include "MantidMDAlgorithms/DimensionParameterIntegration.h"
namespace Mantid
{
namespace MDAlgorithms
{
/**
 Null object for no integration.

 @author Owen Arnold, Tessella plc
 @date 05/10/2010

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

class DimensionParameter;
class DLLExport DimensionParameterNoIntegration : public DimensionParameterIntegration
{
private:

public:
  DimensionParameterNoIntegration();
  double getUpperLimit() const;
  double getLowerLimit() const;
  bool isIntegrated() const;
};
}
}

#endif
