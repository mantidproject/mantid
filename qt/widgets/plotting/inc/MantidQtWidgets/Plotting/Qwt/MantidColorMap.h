// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDCOLORMAP_H_
#define MANTIDCOLORMAP_H_

//---------------------------------------------
// Includes
//---------------------------------------------
#include "MantidQtWidgets/Common/GraphOptions.h"
#include "MantidQtWidgets/Plotting/DllOption.h"
#include "qwt_color_map.h"

/**
   The class inherits from QwtColorMap and implements reading a color color map
   from a file.
   There is also a mode which indicates the scale type.
*/
class EXPORT_OPT_MANTIDQT_PLOTTING MantidColorMap : public QwtColorMap {

public:
  /// Define the possible scale types
  enum class ScaleType { Linear = 0, Log10, Power };

  static QString chooseColorMap(const QString &previousFile, QWidget *parent);
  static QString defaultColorMap();
  static QString exists(const QString &filename);

public:
  MantidColorMap();
  explicit MantidColorMap(const QString &filename,
                          MantidColorMap::ScaleType type);
  ~MantidColorMap() override;
  QwtColorMap *copy() const override;

  void changeScaleType(ScaleType type);

  void setNthPower(double nth_power) { m_nth_power = nth_power; };

  double getNthPower() const { return m_nth_power; };

  bool loadMap(const QString &filename);

  void setNanColor(int r, int g, int b);

  void setupDefaultMap();

  QRgb rgb(double vmin, double vmax, double value) const;
  std::vector<QRgb> rgb(double vmin, double vmax,
                        const std::vector<double> &values) const;
  QRgb rgb(const QwtDoubleInterval &interval, double value) const override;

  double normalize(const QwtDoubleInterval &interval, double value) const;

  unsigned char colorIndex(const QwtDoubleInterval &interval,
                           double value) const override;

  QVector<QRgb> colorTable(const QwtDoubleInterval &interval) const override;

  /**
   * Retrieve the scale type
   * @returns the current scale type
   */
  ScaleType getScaleType() const { return m_scale_type; }

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
  mutable ScaleType m_scale_type;

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
