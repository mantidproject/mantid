#ifndef MANTID_MANTIDWIDGETS_MWVIEW_H_
#define MANTID_MANTIDWIDGETS_MWVIEW_H_

#include "MantidQtMantidWidgets/WidgetDllOption.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "WidgetDllOption.h"

#include <qwt_plot.h>
#include <qpainter.h>
#include "qwt_text.h"
#include <qwt_plot_spectrogram.h>
#include <qwt_raster_data.h>


namespace MantidQt {

namespace API {
class QwtRasterDataMW;
}

namespace MantidWidgets {


/** A viewer for a Matrix Workspace.
 *
 * Before drawing, it acquires a ReadLock to prevent
 * an algorithm from modifying the underlying workspace while it is
 * drawing.
 *
 * If no workspace is set, no drawing occurs (silently).

  @date 2016-02-05

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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS MWView : public QWidget {
  Q_OBJECT

public:

protected:

private:


};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_MANTIDWIDGETS_MWVIEW_H_ */
