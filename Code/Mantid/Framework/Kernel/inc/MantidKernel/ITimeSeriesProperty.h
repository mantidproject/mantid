#ifndef MANTID_KERNEL_ITIMESERIESPROPERTY_H_
#define MANTID_KERNEL_ITIMESERIESPROPERTY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

namespace Mantid
{
namespace Kernel
{
/** A non-templated interface to a TimeSeriesProperty.

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
*/
class ITimeSeriesProperty
{
public:
  /// Deletes the series of values in the property
  virtual void clear() = 0;
  /// Virtual destructor
  virtual ~ITimeSeriesProperty() {}
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_ITIMESERIESPROPERTY_H_*/
