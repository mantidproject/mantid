// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTAPI_MANTIDQWTWORKSPACEDATA_H
#define MANTIDQTAPI_MANTIDQWTWORKSPACEDATA_H

#include "MantidQtWidgets/Plotting/DllOption.h"
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
class EXPORT_OPT_MANTIDQT_PLOTTING MantidQwtWorkspaceData : public QwtData {
public:
  MantidQwtWorkspaceData(bool logScaleY);
  MantidQwtWorkspaceData(const MantidQwtWorkspaceData &data);
  MantidQwtWorkspaceData &operator=(const MantidQwtWorkspaceData & /*data*/);

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
class EXPORT_OPT_MANTIDQT_PLOTTING MantidQwtMatrixWorkspaceData
    : public MantidQwtWorkspaceData {
public:
  MantidQwtMatrixWorkspaceData(bool logScaleY);
  /// Return a new data object of the same type but with a new workspace
  virtual MantidQwtMatrixWorkspaceData *
  copyWithNewSource(const Mantid::API::MatrixWorkspace &workspace) const = 0;
};

#endif
