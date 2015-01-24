#ifndef USER_ALGORITHMS_HELLOWORLDALGORITHM_H_
#define USER_ALGORITHMS_HELLOWORLDALGORITHM_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** Algorithm basic test class.

    @author Matt Clarke, ISIS, RAL
    @date 09/11/2007

    Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class HelloWorldAlgorithm : public API::Algorithm {
public:
  /// no arg constructor
  HelloWorldAlgorithm() : API::Algorithm() {}
  /// virtual destructor
  virtual ~HelloWorldAlgorithm() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "HelloWorldAlgorithm"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return (1); }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Examples"; }
  virtual const std::string summary() const {
    return "Summary of this algorithm - Outputs Hello World!.";
  }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

} // namespace Algorithm
} // namespace Mantid

#endif /*USER_ALGORITHMS_HELLOWORLDALGORITHM_H_*/
