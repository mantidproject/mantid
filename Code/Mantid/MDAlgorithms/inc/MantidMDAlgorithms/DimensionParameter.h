#ifndef DIMENSION_PARAMETER_H
#define DIMENSION_PARAMETER_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace MDAlgorithms
{
/**

 Memento of a Dimension.

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

  class DimensionParameterIntegration;
  class DLLExport DimensionParameter
  {
  private:
    unsigned int m_id;
    std::string m_name;
    double m_upperBounds;
    double m_lowerBounds;
    boost::shared_ptr<DimensionParameterIntegration> m_integration;

  public:
    DimensionParameter(unsigned int id, std::string name, double upperBounds, double lowerBounds, boost::shared_ptr<DimensionParameterIntegration> integration);
    boost::shared_ptr<DimensionParameterIntegration> getIntegration() const;
    double getUpperBound() const;
    double getLowerBound() const;
    std::string getName() const;
    unsigned int getId() const;
  };
}
}

#endif
