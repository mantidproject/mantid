#ifndef MANTID_CRYSTAL_INTEGRATEPEAKSUSINGCLUSTERS_H_
#define MANTID_CRYSTAL_INTEGRATEPEAKSUSINGCLUSTERS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDWorkspace.h"

namespace Mantid {
namespace Crystal {

/** IntegratePeaksUsingClusters : Uses clustering to integrate peaks.

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport IntegratePeaksUsingClusters : public API::Algorithm {
public:
  IntegratePeaksUsingClusters();
  virtual ~IntegratePeaksUsingClusters();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Integrate single crystal peaks using connected component analysis";
  }

  virtual int version() const;
  virtual const std::string category() const;

private:
  Mantid::API::MDNormalization getNormalization();
  void init();
  void exec();
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_INTEGRATEPEAKSUSINGCLUSTERS_H_ */
