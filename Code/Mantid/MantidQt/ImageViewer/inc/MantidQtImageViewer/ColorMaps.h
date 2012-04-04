#ifndef COLOR_MAPS_H
#define COLOR_MAPS_H

#include <vector>
#include <QColor>
#include "MantidQtImageViewer/DllOptionIV.h"

/**
    @class ColorMaps 
  
       This class provides convenient access to several useful color maps
    for the ImageView data viewer.
 
    @author Dennis Mikkelson 
    @date   2012-04-03 
     
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

class EXPORT_OPT_MANTIDQT_IMAGEVIEWER ColorMaps
{

public:
  static void getDefaultMap( std::vector<QRgb> & color_table );

};

} // namespace MantidQt 
} // namespace ImageView 


#endif  // COLOR_MAPS_H
