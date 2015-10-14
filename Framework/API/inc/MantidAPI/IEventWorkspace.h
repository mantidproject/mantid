#ifndef MANTID_API_IEVENTWORKSPACE_H_
#define MANTID_API_IEVENTWORKSPACE_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IEventList.h"
#include "MantidAPI/IEventWorkspace_fwd.h"

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
  IEventWorkspace() : MatrixWorkspace() {}

  /// Returns a clone of the workspace
  std::unique_ptr<IEventWorkspace> clone() const {
    return std::unique_ptr<IEventWorkspace>(doClone());
  }

  /// Return the workspace typeID
  virtual const std::string id() const { return "IEventWorkspace"; }
  virtual std::size_t getNumberEvents() const = 0;
  virtual double getTofMin() const = 0;
  virtual double getTofMax() const = 0;
  virtual Mantid::Kernel::DateAndTime getPulseTimeMax() const = 0;
  virtual Mantid::Kernel::DateAndTime getPulseTimeMin() const = 0;
  virtual Mantid::Kernel::DateAndTime
  getTimeAtSampleMax(double tofOffset = 0) const = 0;
  virtual Mantid::Kernel::DateAndTime
  getTimeAtSampleMin(double tofOffset = 0) const = 0;
  virtual EventType getEventType() const = 0;
  virtual IEventList *getEventListPtr(const std::size_t workspace_index) = 0;
  virtual void generateHistogram(const std::size_t index, const MantidVec &X,
                                 MantidVec &Y, MantidVec &E,
                                 bool skipError = false) const = 0;

  virtual void clearMRU() const = 0;

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  IEventWorkspace(const IEventWorkspace &other) : MatrixWorkspace(other) {}
  /// Protected copy assignment operator. Assignment not implemented.
  IEventWorkspace &operator=(const IEventWorkspace &other);

  virtual const std::string toString() const;

private:
  virtual IEventWorkspace *doClone() const = 0;
};
}
}

#endif // MANTID_API_IEVENTWORKSPACE_H_
