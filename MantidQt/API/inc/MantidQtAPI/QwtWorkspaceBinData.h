#ifndef MANTIDQTAPI_QWTWORKSPACEBINDATA_H
#define MANTIDQTAPI_QWTWORKSPACEBINDATA_H

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidQtAPI/MantidQwtWorkspaceData.h"
#include "DllOption.h"

#include <boost/shared_ptr.hpp>

#include <QString>

//=================================================================================================
//=================================================================================================
/**  This class implements QwtData with direct access to a spectrum in a
 * MatrixWorkspace.
 */
class EXPORT_OPT_MANTIDQT_API QwtWorkspaceBinData
    : public MantidQwtMatrixWorkspaceData {
public:
  QwtWorkspaceBinData(const Mantid::API::MatrixWorkspace &workspace,
                      int binIndex, const bool logScale);

  //! @return Pointer to a copy (virtual copy constructor)
  virtual QwtWorkspaceBinData *copy() const;

  /// Return a new data object of the same type but with a new workspace
  virtual QwtWorkspaceBinData *
  copyWithNewSource(const Mantid::API::MatrixWorkspace &workspace) const;

  //! @return Size of the data set
  virtual size_t size() const;

  /**
  Return the x value of data point i
  @param i :: Index
  @return x X value of data point i
  */
  virtual double x(size_t i) const;
  /**
  Return the y value of data point i
  @param i :: Index
  @return y Y value of data point i
  */
  virtual double y(size_t i) const;

  /// Returns the error of the i-th data point
  double e(size_t i) const;
  /// Returns the x position of the error bar for the i-th data point (bin)
  double ex(size_t i) const;
  /// Number of error bars to plot
  size_t esize() const;

  double getYMin() const;
  double getYMax() const;
  /// Return the label to use for the X axis
  QString getXAxisLabel() const;
  /// Return the label to use for the Y axis
  QString getYAxisLabel() const;

  /// Inform the data that it is to be plotted on a log y scale
  void setLogScale(bool on);
  bool logScale() const { return m_logScale; }
  void saveLowestPositiveValue(const double v);

  // Set offsets for and enables waterfall plots
  void setXOffset(const double x);
  void setYOffset(const double y);
  void setWaterfallPlot(bool on);

protected:
  // Assignment operator (virtualized). MSVC not happy with compiler generated
  // one
  QwtWorkspaceBinData &
  operator=(const QwtWorkspaceBinData &); // required by QwtData base class

private:
  /// Initialize the object
  void init(const Mantid::API::MatrixWorkspace &workspace);

  /// The column index of the current data
  int m_binIndex;
  /// Copy of the X vector
  Mantid::MantidVec m_X;
  /// Copy of the Y vector
  Mantid::MantidVec m_Y;
  /// Copy of the E vector
  Mantid::MantidVec m_E;

  /// A title for the X axis
  QString m_xTitle;
  /// A title for the Y axis
  QString m_yTitle;

  /// Indicates that the data is plotted on a log y scale
  bool m_logScale;

  /// lowest y value
  double m_minY;

  /// lowest positive y value
  double m_minPositive;

  /// highest y value
  double m_maxY;

  /// Indicates whether or not waterfall plots are enabled
  bool m_isWaterfall;

  /// x-axis offset for waterfall plots
  double m_offsetX;

  /// y-axis offset for waterfall plots
  double m_offsetY;
};
#endif
