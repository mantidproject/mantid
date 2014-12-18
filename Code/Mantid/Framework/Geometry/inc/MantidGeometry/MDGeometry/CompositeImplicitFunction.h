#ifndef MANTID_ALGORITHMS_COMPOSITEIMPLICITFUNCTION_H_
#define MANTID_ALGORITHMS_COMPOSITEIMPLICITFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/System.h"
#ifndef Q_MOC_RUN
# include <boost/shared_ptr.hpp>
#endif

namespace Mantid
{
namespace Geometry
{
/**

 This class represents a composite implicit function used for communicating and implementing an operation against
 an MDWorkspace.

 @author Owen Arnold, Tessella plc
 @date 01/10/2010

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

 File change history is stored at: <https://github.com/mantidproject/mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

class DLLExport CompositeImplicitFunction: public Mantid::Geometry::MDImplicitFunction
{
public:

  //---------------------------------- Override base-class methods---
  virtual bool isPointContained(const coord_t * coords);
  virtual bool isPointContained(const std::vector<coord_t> & coords);
  // Unhide base class methods (avoids Intel compiler warning)
  using MDImplicitFunction::isPointContained;
  //-----------------------------------------------------------------

  CompositeImplicitFunction();
  virtual ~CompositeImplicitFunction();
  bool addFunction(Mantid::Geometry::MDImplicitFunction_sptr constituentFunction);
  std::string getName() const;
  std::string toXMLString() const;
  int getNFunctions() const;
  static std::string functionName()
  {
    return "CompositeImplicitFunction";
  }
protected:
  std::vector<Mantid::Geometry::MDImplicitFunction_sptr > m_Functions;
  typedef std::vector<Mantid::Geometry::MDImplicitFunction_sptr >::const_iterator
      FunctionIterator;

};
}
}

#endif
