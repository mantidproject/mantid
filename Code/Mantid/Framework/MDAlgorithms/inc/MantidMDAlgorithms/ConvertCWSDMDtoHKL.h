#ifndef MANTID_MDALGORITHMS_CONVERTCWSDMDTOHKL_H_
#define MANTID_MDALGORITHMS_CONVERTCWSDMDTOHKL_H_

#include "MantidMDAlgorithms/DllConfig.h"
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"

namespace Mantid {
namespace MDAlgorithms {

/** ConvertCWSDMDtoHKL : TODO: DESCRIPTION

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_MDALGORITHMS_DLL ConvertCWSDMDtoHKL : public API::Algorithm {
public:
  ConvertCWSDMDtoHKL();
  virtual ~ConvertCWSDMDtoHKL();

  /// Algorithm's name
  virtual const std::string name() const { return "ConvertCWSDMDtoHKL"; }

  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Convert constant wavelength single crystal diffractomer's data"
           "in MDEventWorkspace and in unit of Q-sample to the HKL space "
           "by UB matrix.";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }

  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Diffraction;MDAlgorithms";
  }

private:
  /// Initialisation code
  void init();

  /// Execution code
  void exec();
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CONVERTCWSDMDTOHKL_H_ */
