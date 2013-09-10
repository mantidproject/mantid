#ifndef MATRIX_WS_IMAGE_VIEW_H
#define MATRIX_WS_IMAGE_VIEW_H

#include "MantidQtSpectrumViewer/DllOptionIV.h"
#include "MantidAPI/MatrixWorkspace.h"

#include "MantidQtSpectrumViewer/ImageView.h"

/**
    @class MatrixWSDataSource 
  
       This is the top level class for showing a matrix workspace
    using an ImageViewer.
 
    @author Dennis Mikkelson 
    @date   2012-05-08 
     
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
namespace ImageView
{

class EXPORT_OPT_MANTIDQT_IMAGEVIEWER MatrixWSImageView 
{
  public:

    /// Construct an image viewer for the specifed MatrixWorkspace
    MatrixWSImageView( Mantid::API::MatrixWorkspace_const_sptr mat_ws );

   ~MatrixWSImageView();

  private:
    ImageView *image_view;
};

} // namespace MantidQt 
} // namespace ImageView 

#endif // MATRIX_WS_IMAGE_VIEW_H
