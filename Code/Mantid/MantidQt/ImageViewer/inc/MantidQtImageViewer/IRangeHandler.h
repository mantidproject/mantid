#ifndef IRANGE_HANDLER_H
#define IRANGE_HANDLER_H

#include "MantidQtImageViewer/DllOptionIV.h"
#include "MantidQtImageViewer/ImageDataSource.h"

namespace MantidQt
{
namespace ImageView
{
/** An interface to the RangeHandler class, which manages the min, max and step
    range controls for the ImageView data viewer.

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

    Code Documentation is available at <http://doxygen.mantidproject.org>
 */
class EXPORT_OPT_MANTIDQT_IMAGEVIEWER IRangeHandler
{
public:
  /// Construct object to manage min, max and step controls in the UI
  IRangeHandler() {}
  virtual ~IRangeHandler() {}

  /// Configure min, max and step controls for the specified data source
  virtual void ConfigureRangeControls( ImageDataSource* data_source ) = 0;
  /// Get the range of data to display in the image, from GUI controls
  virtual void GetRange( double &min, double &max, double &step ) = 0;
};

} // namespace ImageView
} // namespace MantidQt 

#endif // IRANGE_HANDLER_H
