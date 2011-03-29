#ifndef MD_MANTID_ALGORITHMS_BOX_INTERPRETER
#define MD_MANTID_ALGORITHMS_BOX_INTERPRETER

/** A helper class to determine inner surface box boundaries from a composite set of implicit functions

 @author Owen Arnold, Tessella plc
 @date 21/01/2011

 Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

#include "MantidKernel/System.h"
#include <boost/smart_ptr.hpp>
#include <vector>
#include <functional>

namespace Mantid
{
//Forward declaration.
namespace API
{
class ImplicitFunction;
}

namespace MDAlgorithms
{
//Forward declaration.
class CompositeImplicitFunction;
class BoxImplicitFunction;


typedef std::vector<boost::shared_ptr<Mantid::MDAlgorithms::BoxImplicitFunction> > boxVector;

typedef std::vector<boost::shared_ptr<Mantid::API::ImplicitFunction> > functionVector;

class DLLExport BoxInterpreter: public std::unary_function<Mantid::API::ImplicitFunction*, std::vector<double> >
{
private:
  /// Recursively walk the composite tree and extract flattened vector of BoxImplicit functions.
  boxVector walkTree(CompositeImplicitFunction* compFunc) const;
public:

  /// Act as Functor.
  std::vector<double> operator()(Mantid::API::ImplicitFunction* implicitFunction) const;

  /// Explicit call to Functor execution.
  std::vector<double> Execute(Mantid::API::ImplicitFunction* implicitFunction) const;

  /// Get all the boxes extractable from the implicit function.
  boxVector getAllBoxes(Mantid::API::ImplicitFunction* implicitFunction) const;
};
}

}

#endif
