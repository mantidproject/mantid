#ifndef DIMENSION_PARAMETER_SET_H
#define DIMENSION_PARAMETER_SET_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>
#include "MantidMDAlgorithms/DimensionParameter.h"
namespace Mantid
{
namespace MDAlgorithms
{
/**

 Represents a collection of dimension parameter information

 @author Owen Arnold, Tessella plc
 @date 01/10/2010

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

class DLLExport DimensionParameterSet
{
private:
  std::vector<boost::shared_ptr<DimensionParameter>> m_dimensionParameters;
  boost::shared_ptr<DimensionParameter> m_dimX;
  boost::shared_ptr<DimensionParameter> m_dimY;
  boost::shared_ptr<DimensionParameter> m_dimZ;
  boost::shared_ptr<DimensionParameter> m_dimt;
  boost::shared_ptr<DimensionParameter> findDimension(unsigned int id);
  boost::shared_ptr<DimensionParameter> findDimensionThrow(unsigned int id);

public:
  void addDimensionParameter(DimensionParameter* dParameter);
  std::vector<boost::shared_ptr<DimensionParameter>> getDimensions();
  void setXDimension(unsigned int id);
  void setYDimension(unsigned int id);
  void setZDimension(unsigned int id);
  void settDimension(unsigned int id);

  boost::shared_ptr<DimensionParameter> getXDimension();
  boost::shared_ptr<DimensionParameter> getYDimension();
  boost::shared_ptr<DimensionParameter> getZDimension();
  boost::shared_ptr<DimensionParameter> gettDimension();
};
}
}

#endif
