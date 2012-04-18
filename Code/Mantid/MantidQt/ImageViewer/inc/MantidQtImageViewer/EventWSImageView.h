#ifndef EVENT_WS_IMAGE_VIEW_H
#define EVENT_WS_IMAGE_VIEW_H

#include "MantidQtImageViewer/DllOptionIV.h"

#include "MantidDataObjects/EventWorkspace.h"

/**
    @class EventWSDataSource 
  
       This is the top level class for showing an event workspace
    using an ImageViewer.
 
    @author Dennis Mikkelson 
    @date   2012-04-18 
     
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
#include "MantidQtImageViewer/ImageView.h"

using namespace Mantid;
using namespace DataObjects;
using namespace Kernel;

namespace MantidQt
{
namespace ImageView
{

class EXPORT_OPT_MANTIDQT_IMAGEVIEWER EventWSImageView 
{
  public:

    /// Construct an image viewer for the specifed EventWorkspace
    EventWSImageView( EventWorkspace_sptr ev_ws );

   ~EventWSImageView();

  private:
    ImageView *image_view;
};

} // namespace MantidQt 
} // namespace ImageView 

#endif // EVENT_WS_IMAGE_VIEW_H
