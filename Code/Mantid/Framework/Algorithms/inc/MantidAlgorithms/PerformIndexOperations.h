#ifndef MANTID_ALGORITHMS_PERFORMINDEXOPERATIONS_H_
#define MANTID_ALGORITHMS_PERFORMINDEXOPERATIONS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/DataProcessorAlgorithm.h"

namespace Mantid {
namespace Algorithms {

/** PerformIndexOperations : Crop and sum a workspace according to the parsed
  workspace index operations provided.

  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport PerformIndexOperations : public API::DataProcessorAlgorithm {
public:
  PerformIndexOperations();
  virtual ~PerformIndexOperations();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Process the workspace according to the Index operations provided.";
  }

  virtual int version() const;
  virtual const std::string category() const;

private:
  void init();
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_PERFORMINDEXOPERATIONS_H_ */
