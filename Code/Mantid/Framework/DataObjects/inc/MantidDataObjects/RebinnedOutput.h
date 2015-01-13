#ifndef MANTID_DATAOBJECTS_REBINNEDOUTPUT_H_
#define MANTID_DATAOBJECTS_REBINNEDOUTPUT_H_

#include "MantidAPI/ISpectrum.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid {
namespace DataObjects {

/** RebinnedOutput

  This class will handle the needs for 2D fractional overlap rebinning.
  The rebinning method requires the separate tracking of counts and
  fractional area. The workspace will always present the correct data to a
  2D display. Integration and rebinning will be handled via the fundamental
  algorithms.

  @date 2012-04-05

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
class DLLExport RebinnedOutput : public Workspace2D {
public:
  /// Class constructor.
  RebinnedOutput();
  /// Class destructor.
  virtual ~RebinnedOutput();

  /// Get the workspace ID.
  virtual const std::string id() const;

  /// Returns the fractional area
  virtual MantidVec &dataF(const std::size_t index);

  /// Returns the fractional area
  virtual const MantidVec &dataF(const std::size_t index) const;

  /// Create final representation
  void finalize(bool hasSqrdErrs = true);

  /// Returns a read-only (i.e. const) reference to the specified F array
  const MantidVec &readF(std::size_t const index) const;

  /// Set the fractional area array for a given index.
  void setF(const std::size_t index, const MantidVecPtr &F);

protected:
  /// Called by initialize() in MatrixWorkspace
  virtual void init(const std::size_t &NVectors, const std::size_t &XLength,
                    const std::size_t &YLength);

  /// A vector that holds the 1D vectors for the fractional area.
  std::vector<MantidVec> fracArea;

private:
  /// Private copy constructor. NO COPY ALLOWED
  RebinnedOutput(const RebinnedOutput &);
  /// Private copy assignment operator. NO ASSIGNMENT ALLOWED
  RebinnedOutput &operator=(const RebinnedOutput &);
};

/// shared pointer to the RebinnedOutput class
typedef boost::shared_ptr<RebinnedOutput> RebinnedOutput_sptr;
/// shared pointer to a const RebinnedOutput
typedef boost::shared_ptr<const RebinnedOutput> RebinnedOutput_const_sptr;

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_REBINNEDOUTPUT_H_ */
