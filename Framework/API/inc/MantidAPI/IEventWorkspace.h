#ifndef MANTID_API_IEVENTWORKSPACE_H_
#define MANTID_API_IEVENTWORKSPACE_H_

#include "MantidAPI/IEventList.h"
#include "MantidAPI/IEventWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {

namespace API {

/** This class provides an interface to an EventWorkspace.

  @author Martyn Gigg, Tessella plc
  @date 13/08/2010

  Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_API_DLL IEventWorkspace : public MatrixWorkspace {
public:
  IEventWorkspace(
      const Parallel::StorageMode storageMode = Parallel::StorageMode::Cloned)
      : MatrixWorkspace(storageMode) {}
  IEventWorkspace &operator=(const IEventWorkspace &) = delete;
  /// Returns a clone of the workspace
  IEventWorkspace_uptr clone() const { return IEventWorkspace_uptr(doClone()); }
  /// Returns a default-initialized clone of the workspace
  IEventWorkspace_uptr cloneEmpty() const {
    return IEventWorkspace_uptr(doCloneEmpty());
  }

  IEventList &getSpectrum(const size_t index) override = 0;
  const IEventList &getSpectrum(const size_t index) const override = 0;

  /// Return the workspace typeID
  const std::string id() const override { return "IEventWorkspace"; }
  virtual std::size_t getNumberEvents() const = 0;
  virtual double getTofMin() const = 0;
  virtual double getTofMax() const = 0;
  virtual Mantid::Types::Core::DateAndTime getPulseTimeMax() const = 0;
  virtual Mantid::Types::Core::DateAndTime getPulseTimeMin() const = 0;
  virtual Mantid::Types::Core::DateAndTime
  getTimeAtSampleMax(double tofOffset = 0) const = 0;
  virtual Mantid::Types::Core::DateAndTime
  getTimeAtSampleMin(double tofOffset = 0) const = 0;
  virtual EventType getEventType() const = 0;
  void generateHistogram(const std::size_t index, const MantidVec &X,
                         MantidVec &Y, MantidVec &E,
                         bool skipError = false) const override = 0;

  virtual void updateAllX() = 0;

  virtual void clearMRU() const = 0;

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  IEventWorkspace(const IEventWorkspace &) = default;

  const std::string toString() const override;

private:
  IEventWorkspace *doClone() const override = 0;
  IEventWorkspace *doCloneEmpty() const override = 0;
};
} // namespace API
} // namespace Mantid

#endif // MANTID_API_IEVENTWORKSPACE_H_
