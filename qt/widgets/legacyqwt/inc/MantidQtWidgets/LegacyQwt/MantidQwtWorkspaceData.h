#ifndef MANTIDQTAPI_MANTIDQWTWORKSPACEDATA_H
#define MANTIDQTAPI_MANTIDQWTWORKSPACEDATA_H
/**
  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

#include "DllOption.h"
#include "qwt_data.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
namespace Mantid {
namespace API {
class MatrixWorkspace;
}
} // namespace Mantid

/**
 * Base class for Workspace Qwt data types
 */
class EXPORT_OPT_MANTIDQT_LEGACYQWT MantidQwtWorkspaceData : public QwtData {
public:
  MantidQwtWorkspaceData(bool logScaleY);
  MantidQwtWorkspaceData(const MantidQwtWorkspaceData &data);
  MantidQwtWorkspaceData &operator=(const MantidQwtWorkspaceData &);

  virtual QString getXAxisLabel() const = 0;
  virtual QString getYAxisLabel() const = 0;

  double x(size_t i) const override;
  double y(size_t i) const override;
  virtual size_t esize() const;
  virtual double e(size_t i) const;
  virtual double ex(size_t i) const;
  bool isPlottable() const;
  virtual void setLogScaleY(bool on);
  virtual bool logScaleY() const;
  void setMinimumPositiveValue(const double v);
  virtual double getYMin() const;
  virtual double getYMax() const;
  virtual void setXOffset(const double x);
  virtual void setYOffset(const double y);
  virtual void setWaterfallPlot(bool on);
  virtual bool isWaterfallPlot() const;
  double offsetY() const { return m_offsetY; }

  void calculateYMinAndMax() const;

protected:
  virtual double getX(size_t i) const = 0;
  virtual double getY(size_t i) const = 0;
  virtual double getE(size_t i) const = 0;
  virtual double getEX(size_t i) const = 0;

private:
  enum class DataStatus : uint8_t { Undefined, NotPlottable, Plottable };

  /// Indicates that the data is plotted on a log y scale
  bool m_logScaleY;

  /// lowest y value
  mutable double m_minY;

  /// lowest positive y value
  mutable double m_minPositive;

  /// highest y value
  mutable double m_maxY;

  /// True if data is 'sensible' to plot
  mutable DataStatus m_plottable;

  /// Indicates whether or not waterfall plots are enabled
  bool m_isWaterfall;

  /// x-axis offset for waterfall plots
  double m_offsetX;

  /// y-axis offset for waterfall plots
  double m_offsetY;
};

/**
 * Base class for MatrixWorkspace Qwt data types
 */
class EXPORT_OPT_MANTIDQT_LEGACYQWT MantidQwtMatrixWorkspaceData
    : public MantidQwtWorkspaceData {
public:
  MantidQwtMatrixWorkspaceData(bool logScaleY);
  /// Return a new data object of the same type but with a new workspace
  virtual MantidQwtMatrixWorkspaceData *
  copyWithNewSource(const Mantid::API::MatrixWorkspace &workspace) const = 0;
};

#endif
