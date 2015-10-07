#ifndef MANTID_CRYSTAL_PEAKINTENSITYVSRADIUS_H_
#define MANTID_CRYSTAL_PEAKINTENSITYVSRADIUS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Crystal {

/** Calculate the integrated intensity of peaks vs integration radius.

  @date 2011-12-02

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport PeakIntensityVsRadius : public API::Algorithm {
public:
  PeakIntensityVsRadius();
  virtual ~PeakIntensityVsRadius();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Calculate the integrated intensity of peaks vs integration radius.";
  }

  virtual int version() const;
  virtual const std::string category() const;

private:
  void init();
  void exec();
  std::map<std::string, std::string> validateInputs();
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_PEAKINTENSITYVSRADIUS_H_ */
