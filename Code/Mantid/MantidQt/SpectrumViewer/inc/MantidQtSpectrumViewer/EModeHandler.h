#ifndef EMODE_HANDLER_H
#define EMODE_HANDLER_H

#include "ui_SpectrumView.h"
#include "MantidQtSpectrumViewer/SpectrumDataSource.h"
#include "MantidQtSpectrumViewer/DllOptionSV.h"

/**
    @class EModeHandler

    This manages the instrument type combo box (emode) and E Fixed controls
    for the SpectrumView data viewer.

    @author Dennis Mikkelson
    @date   2012-10-12

    Copyright © 2012 ORNL, STFC Rutherford Appleton Laboratories

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

    Code Documentation is available at
                 <http://doxygen.mantidproject.org>
 */

namespace MantidQt
{
namespace SpectrumView
{

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER EModeHandler
{
  public:

    /// Construct object to manage E Mode controls in the UI
    EModeHandler( Ui_SpectrumViewer* sv_ui );

    /// Get the E Mode to control units calculation, from the combo box
    int getEMode();

    /// Set the E Mode to control units calculation, in the combo box
    void setEMode( const int mode );

    /// Get the E Fixed value from the GUI
    double getEFixed();

    /// Set the E Fixed value in the GUI
    void setEFixed( const double efixed );

  private:
    Ui_SpectrumViewer* m_svUI;

};

} // namespace SpectrumView
} // namespace MantidQt

#endif // EMODE_HANDLER_H
