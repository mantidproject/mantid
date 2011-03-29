#ifndef MANTIDCOLORMAP_H_
#define MANTIDCOLORMAP_H_

//---------------------------------------------
// Includes
//---------------------------------------------
#include "qwt_color_map.h"
#include <boost/shared_ptr.hpp>
#include "MantidKernel/Logger.h"
#include "../../GraphOptions.h"

//---------------------------------------------
// Forward declarations
//---------------------------------------------
class GLColor;

/**
   The class inherits from QwtColorMap and implements reading a color color map from a file. 
   There is also a mode which indicates the scale type. 

   Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
   
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
   
   File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
*/
class MantidColorMap : public QwtColorMap
{

public:
  /// Default constructor
  MantidColorMap();

  /// Constructor with a type parameter
  explicit MantidColorMap(const QString & filename, GraphOptions::ScaleType type);

  /// (virtual) Destructor
  virtual ~MantidColorMap();

  /// Create a clone of the color map
  QwtColorMap* copy() const;

  /// Change the scale type
  void changeScaleType(GraphOptions::ScaleType type);

  /** 
   * Retrieve the scale type
   * @returns the current scale type
   */
  GraphOptions::ScaleType getScaleType() const 
  {
    return m_scale_type;
  }  
  
  /// Load a color map file
  bool loadMap(const QString & filename);
  
  /// Setup a default color map. This is used if a file is not available
  void setupDefaultMap();
  
  /// Compute an rgb value for the given data value and interval
  QRgb rgb(const QwtDoubleInterval & interval, double value) const;

  /// Compute fraction for the given value and range using the current scale type
  double normalize(const QwtDoubleInterval &interval, double value) const;

  /// Compute a color index for the given data value and interval
  unsigned char colorIndex (const QwtDoubleInterval &interval, double value) const;

  /// Compute a lookup table
  QVector<QRgb> colorTable(const QwtDoubleInterval & interval) const;

  /// Return a GLColor object that can be assigned to an instrument actor
  boost::shared_ptr<GLColor> getColor(unsigned char index) const;

  /**
   * Get the number of colors in this map
   */
  inline unsigned char getTopCIndex() const
  {
    return static_cast<unsigned char>(m_num_colors - 1);
  }
  
  /** 
   * The maximum number of colors that any color map is allowed to use
   */
  static unsigned char getLargestAllowedCIndex()
  {
    return static_cast<unsigned char>(255);
  }

private:

  /// The scale choice
  mutable GraphOptions::ScaleType m_scale_type;

  /// An array of shared pointers to objects that define how the color should be painted on
  /// an OpenGL surface. QVector objects are implicitly shared so offer better performance than
  /// standard vectors
  QVector<boost::shared_ptr<GLColor> > m_colors;

  /// The number of colors in this map
  short m_num_colors;

};



#endif //MANTIDCOLORMAP_H_
