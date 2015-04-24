#ifndef MANTID_ALGORITHMS_EXTRACTSPECTRA_H_
#define MANTID_ALGORITHMS_EXTRACTSPECTRA_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Algorithms {

/** Extracts specified spectra from a workspace and places them in a new
  workspace.


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
class DLLExport ExtractSpectra : public API::Algorithm {
public:
  ExtractSpectra();
  virtual ~ExtractSpectra();

  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;
  virtual const std::string summary() const;

private:
  void init();
  void exec();
  void execEvent();

  void checkProperties();
  std::size_t getXMin(const int wsIndex = 0);
  std::size_t getXMax(const int wsIndex = 0);
  void cropRagged(API::MatrixWorkspace_sptr outputWorkspace, int inIndex,
                  int outIndex);

  /// The input workspace
  API::MatrixWorkspace_sptr m_inputWorkspace;
  DataObjects::EventWorkspace_sptr eventW;
  /// The bin index to start the cropped workspace from
  std::size_t m_minX;
  /// The bin index to end the cropped workspace at
  std::size_t m_maxX;
  /// The spectrum index to start the cropped workspace from
  specid_t m_minSpec;
  /// The spectrum index to end the cropped workspace at
  specid_t m_maxSpec;
  /// Flag indicating whether the input workspace has common boundaries
  bool m_commonBoundaries;
  /// Flag indicating whether we're dealing with histogram data
  bool m_histogram;
  /// Flag indicating whether XMin and/or XMax has been set
  bool m_croppingInX;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_EXTRACTSPECTRA_H_ */