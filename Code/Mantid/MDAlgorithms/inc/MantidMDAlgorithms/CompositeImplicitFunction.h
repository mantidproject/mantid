#ifndef MANTID_ALGORITHMS_COMPOSITEIMPLICITFUNCTION_H_
#define MANTID_ALGORITHMS_COMPOSITEIMPLICITFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>
#include "MantidAPI/ImplicitFunction.h"

namespace Mantid
{
namespace API
{
class Point3D;
}
namespace MDAlgorithms
{
/**

 This class represents a composite implicit function used for communicating and implementing an operation against
 an MDWorkspace.

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

class DLLExport CompositeImplicitFunction: public Mantid::API::ImplicitFunction
{
public:
  CompositeImplicitFunction();

  ~CompositeImplicitFunction();
  void addFunction(boost::shared_ptr<Mantid::API::ImplicitFunction> constituentFunction);
  bool evaluate(const Mantid::API::Point3D* pPoint3D) const;
  std::string getName() const;
  std::string toXMLString() const;
  int getNFunctions() const;
  bool operator==(const CompositeImplicitFunction &other) const;
  bool operator!=(const CompositeImplicitFunction &other) const;
  static std::string functionName()
  {
    return "CompositeImplicitFunction";
  }
  std::vector<boost::shared_ptr<Mantid::API::ImplicitFunction> > getFunctions() const;
protected:
  std::vector<boost::shared_ptr<Mantid::API::ImplicitFunction> > m_Functions;
  typedef std::vector<boost::shared_ptr<Mantid::API::ImplicitFunction> >::const_iterator
      FunctionIterator;

};
}
}

#endif
