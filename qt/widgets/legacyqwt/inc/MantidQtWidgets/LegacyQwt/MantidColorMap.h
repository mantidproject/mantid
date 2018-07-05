#ifndef MANTIDCOLORMAP_H_
#define MANTIDCOLORMAP_H_

//---------------------------------------------
// Includes
//---------------------------------------------
#include "DllOption.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/GraphOptions.h"
#include "qwt_color_map.h"
#include <boost/shared_ptr.hpp>

/**
   The class inherits from QwtColorMap and implements reading a color color map
   from a file.
   There is also a mode which indicates the scale type.

   Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
*/
class EXPORT_OPT_MANTIDQT_LEGACYQWT MantidColorMap : public QwtColorMap {

public:
  MantidColorMap();
  explicit MantidColorMap(const QString &filename,
                          GraphOptions::ScaleType type);
  ~MantidColorMap() override;
  QwtColorMap *copy() const override;

  void changeScaleType(GraphOptions::ScaleType type);

  void setNthPower(double nth_power) { m_nth_power = nth_power; };

  double getNthPower() const { return m_nth_power; };

  bool loadMap(const QString &filename);

  static QString loadMapDialog(QString previousFile, QWidget *parent);

  void setNanColor(int r, int g, int b);

  void setupDefaultMap();

  QRgb rgb(const QwtDoubleInterval &interval, double value) const override;

  double normalize(const QwtDoubleInterval &interval, double value) const;

  unsigned char colorIndex(const QwtDoubleInterval &interval,
                           double value) const override;

  QVector<QRgb> colorTable(const QwtDoubleInterval &interval) const override;

  /**
   * Retrieve the scale type
   * @returns the current scale type
   */
  GraphOptions::ScaleType getScaleType() const { return m_scale_type; }

  /**
   * Retrieve the map name
   * @returns the map name
   */
  QString getName() const { return m_name; }

  /**
   * Retrieve the map name
   * @returns the map name
   */
  QString getFilePath() const { return m_path; }

  /**
   * Get the number of colors in this map
   */
  inline unsigned char getTopCIndex() const {
    return static_cast<unsigned char>(m_num_colors - 1);
  }

  /**
   * The maximum number of colors that any color map is allowed to use
   */
  static unsigned char getLargestAllowedCIndex() {
    return static_cast<unsigned char>(255);
  }

private:
  /// The scale choice
  mutable GraphOptions::ScaleType m_scale_type;

  /// An array of shared pointers to objects that define how the color should be
  /// painted on
  /// an OpenGL surface. QVector objects are implicitly shared so offer better
  /// performance than
  /// standard vectors
  QVector<QRgb> m_colors;

  /// The number of colors in this map
  short m_num_colors;

  /// Color to show for not-a-number
  QRgb m_nan_color;

  /// Cached NAN value
  double m_nan;

  /// the name of the color map
  QString m_name;

  /// the path to the map file
  QString m_path;

  double m_nth_power;
};

#endif // MANTIDCOLORMAP_H_
