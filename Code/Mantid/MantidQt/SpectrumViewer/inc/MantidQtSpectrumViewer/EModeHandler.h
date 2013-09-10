#ifndef EMODE_HANDLER_H
#define EMODE_HANDLER_H

#include "ui_ImageView.h"
#include "MantidQtSpectrumViewer/ImageDataSource.h"
#include "MantidQtSpectrumViewer/DllOptionIV.h"

/**
    @class EModeHandler 
  
      This manages the instrument type combo box (emode) and E Fixed controls 
    for the ImageView data viewer. 
 
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

class EXPORT_OPT_MANTIDQT_IMAGEVIEWER EModeHandler 
{
  public:

    /// Construct object to manage E Mode controls in the UI
    EModeHandler( Ui_ImageViewer* iv_ui );

    /// Get the E Mode to control units calculation, from the combo box
    int GetEMode();

    /// Set the E Mode to control units calculation, in the combo box
    void SetEMode( const int mode );

    /// Get the E Fixed value from the GUI
    double GetEFixed();

    /// Set the E Fixed value in the GUI
    void SetEFixed( const double efixed );

  private:
    Ui_ImageViewer* iv_ui;
};

} // namespace SpectrumView
} // namespace MantidQt 

#endif // EMODE_HANDLER_H
