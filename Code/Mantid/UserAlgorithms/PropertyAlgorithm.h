#ifndef USER_ALGORITHMS_PROPERTYALGORITHM_H_
#define USER_ALGORITHMS_PROPERTYALGORITHM_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** An example algorithm showing the use of properties.

    @author Roman Tolchenov, ISIS, RAL
    @date 01/05/2008

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class PropertyAlgorithm : public API::Algorithm
{
public:
  ///no arg constructor
  PropertyAlgorithm() : API::Algorithm() {}
  ///virtual destructor
  virtual ~PropertyAlgorithm() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "PropertyAlgorithm";}
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return (1);}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Examples";}

private:
  ///Initialisation code
  void init();
  ///Execution code
  void exec();
};

} // namespace Algorithm
} // namespace Mantid

#endif /*USER_ALGORITHMS_PROPERTYALGORITHM_H_*/

