#ifndef MANTID_ALGORITHMS_RESETNEGATIVES_H_
#define MANTID_ALGORITHMS_RESETNEGATIVES_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

/** ResetNegatives : Reset negative values to something else.

  @date 2012-02-01

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ResetNegatives : public API::Algorithm {
public:
  ResetNegatives();
  virtual ~ResetNegatives();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Reset negative values to something else.";
  }

  virtual int version() const;
  virtual const std::string category() const;

private:
  void init();
  void exec();
  void pushMinimum(API::MatrixWorkspace_const_sptr minWS,
                   API::MatrixWorkspace_sptr wksp, API::Progress &prog);
  void changeNegatives(API::MatrixWorkspace_const_sptr minWS,
                       const double value, API::MatrixWorkspace_sptr wksp,
                       API::Progress &prog);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_RESETNEGATIVES_H_ */
