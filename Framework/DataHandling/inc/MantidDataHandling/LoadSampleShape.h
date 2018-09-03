#ifndef DATAHANDLING_LOAD_SHAPE_H_
#define DATAHANDLING_LOAD_SHAPE_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {
/**  Load Shape into an instrument of a workspace

     The following file types are supported

       STL file with suffix .stl


@author Karl Palmen ISIS;
@date 26/02/2018

Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class DLLExport LoadSampleShape : public Mantid::API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadSampleShape"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The algorithm loads a shape into the instrument of a workspace "
           "at the sample.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  /// Related algorithms
  const std::vector<std::string> seeAlso() const override {
    return {"CreateSampleShape", "CopySample", "SetSampleMaterial"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "DataHandling\\Instrument";
  }

private:
  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;
};

} // end namespace DataHandling
} // namespace Mantid

#endif /* DATAHANDLING_LOAD_SHAPE_H_ */
