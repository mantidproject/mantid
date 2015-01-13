/*
 * GoniometerAnglesFromPhiRotation.h
 *
 *  Created on: Apr 15, 2013
 *      Author: ruth
 */
/**
 Finds Goniometer angles for a 2nd run with the same chi and omega rotationa and
 only phi rotation changes by a specified amount.

 @author Ruth Mikkelson, SNS, ORNL
 @date 04/15/2013

 Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#ifndef GoniometerAnglesFromPhiRotation_H_
#define GoniometerAnglesFromPhiRotation_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/PeaksWorkspace.h"

namespace Mantid {
namespace Crystal {
class DLLExport GoniometerAnglesFromPhiRotation : public API::Algorithm {
public:
  GoniometerAnglesFromPhiRotation();
  ~GoniometerAnglesFromPhiRotation();

  /// Algorithm's name for identification
  const std::string name() const { return "GoniometerAnglesFromPhiRotation"; }

  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "The 2nd PeaksWorkspace is set up with the correct sample "
           "orientations and UB matrices";
  }

  /// Algorithm's version for identification
  int version() const { return 1; }

  /// Algorithm's category for identification
  const std::string category() const { return "Crystal"; }

private:
  /// Initialise the properties
  void init();
  Kernel::Matrix<double>
  getUBRaw(const Kernel::Matrix<double> &UB,
           const Kernel::Matrix<double> &GoniometerMatrix) const;

  bool CheckForOneRun(const DataObjects::PeaksWorkspace_sptr &Peaks,
                      Kernel::Matrix<double> &GoniometerMatrix) const;

  void IndexRaw(const DataObjects::PeaksWorkspace_sptr &Peaks,
                const Kernel::Matrix<double> &UBraw, int &Nindexed,
                double &AvErrIndexed, double &AvErrorAll,
                double tolerance) const;

  /// Run the algorithm
  void exec();
};

} // Crystal
} // Mantid

#endif /* GoniometerAnglesFromPhiRotation_H_ */
