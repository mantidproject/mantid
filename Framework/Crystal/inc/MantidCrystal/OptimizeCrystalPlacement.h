/*
 * OptimizeCrystalPlacement.h
 *
 *  Created on: Jan 26, 2013
 *      Author: ruth
 */

#ifndef OPTIMIZECRYSTALPLACEMENT_H_
#define OPTIMIZECRYSTALPLACEMENT_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {

/** OptimizeCrystalPlacement

Description:
This algorithm basically indexes peaks with the crystal orientation matrix
stored in the peaks workspace.
The optimization is on the goniometer settings for the runs in the peaks
workspace and also the sample
orientation .
@author Ruth Mikkelson, SNS,ORNL
@date 01/26/2013

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
class DLLExport OptimizeCrystalPlacement : public API::Algorithm {
public:
  const std::string name() const override {
    return "OptimizeCrystalPlacement";
  };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm  optimizes goniometer settings  and sample "
           "orientation to better index the peaks.";
  }

  int version() const override { return 1; };

  const std::string category() const override {
    return "Crystal\\Corrections";
  };

private:
  void init() override;

  void exec() override;
};
} // namespace Crystal
} // namespace Mantid

#endif /* OPTIMIZECRYSTALPLACEMENT_H_ */
