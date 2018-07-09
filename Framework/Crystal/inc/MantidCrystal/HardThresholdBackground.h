#ifndef MANTID_CRYSTAL_HARDTHRESHOLDBACKGROUND_H_
#define MANTID_CRYSTAL_HARDTHRESHOLDBACKGROUND_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidCrystal/BackgroundStrategy.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {

/** HardThresholdBackground : Implementation of BackgroundStrategy using a fixed
  background signal value as the threshold.

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
class DLLExport HardThresholdBackground : public BackgroundStrategy {
public:
  /// Contructor
  HardThresholdBackground(const double thresholdSignal,
                          const Mantid::API::MDNormalization normalization);

  /// Overriden isBackground
  bool isBackground(Mantid::API::IMDIterator *iterator) const override;

  /// Overriden configureIterator.
  void
  configureIterator(Mantid::API::IMDIterator *const iterator) const override;

  /// Virtual constructor
  HardThresholdBackground *clone() const override;

private:
  /// Cutoff
  double m_thresholdSignal;
  /// Normalization
  Mantid::API::MDNormalization m_normalization;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_HARDTHRESHOLDBACKGROUND_H_ */
