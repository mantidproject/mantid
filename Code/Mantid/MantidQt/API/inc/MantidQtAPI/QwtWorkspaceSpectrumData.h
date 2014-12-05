#ifndef MANTIDQTAPI_QWTWORKSPACESPECTRUMDATA_H
#define MANTIDQTAPI_QWTWORKSPACESPECTRUMDATA_H

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtAPI/MantidQwtWorkspaceData.h"
#include "DllOption.h"

#include <boost/shared_ptr.hpp>
#include <QString>

namespace MantidQt
{

  // Enumerate how to handle distributions
  enum DistributionFlag
  {
    DistributionDefault = 0, // Use preferences value
    DistributionTrue, // Force distribution plotting
    DistributionFalse // Disable distribution plotting
  };

}

//=================================================================================================
//=================================================================================================
/**  This class implements QwtData with direct access to a spectrum in a MatrixWorkspace.
 */
class EXPORT_OPT_MANTIDQT_API QwtWorkspaceSpectrumData : public MantidQwtMatrixWorkspaceData
{
public:

  QwtWorkspaceSpectrumData(const Mantid::API::MatrixWorkspace & workspace, int specIndex,
                           const bool logScale, const bool plotAsDistribution);

  //! @return Pointer to a copy (virtual copy constructor)
  virtual QwtWorkspaceSpectrumData *copy() const;
  /// Return a new data object of the same type but with a new workspace
  virtual QwtWorkspaceSpectrumData* copyWithNewSource(const Mantid::API::MatrixWorkspace & workspace) const;

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
  double e(size_t i)const;
  /// Returns the x position of the error bar for the i-th data point (bin)
  double ex(size_t i)const;
  /// Number of error bars to plot
  size_t esize()const;

  double getYMin() const;
  double getYMax() const;
  /// Return the label to use for the X axis
  QString getXAxisLabel() const;
  /// Return the label to use for the Y axis
  QString getYAxisLabel() const;

  bool isHistogram() const { return m_isHistogram; }
  bool dataIsNormalized() const { return m_dataIsNormalized; }

  /// Inform the data that it is to be plotted on a log y scale
  void setLogScale(bool on);
  bool logScale()const{return m_logScale;}
  void saveLowestPositiveValue(const double v);
  bool setAsDistribution(bool on = true);

protected:
  // Assignment operator (virtualized). MSVC not happy with compiler generated one
  QwtWorkspaceSpectrumData& operator=(const QwtWorkspaceSpectrumData&);

private:

  friend class MantidMatrixCurve;

  /// Spectrum index in the workspace
  int m_spec;
  /// Copy of the X vector
  Mantid::MantidVec m_X;
  /// Copy of the Y vector
  Mantid::MantidVec m_Y;
  /// Copy of the E vector
  Mantid::MantidVec m_E;

  /// A caption for the X axis
  QString m_xTitle;
  /// A caption for the Y axis
  QString m_yTitle;

  /// Is the spectrum a histogram?
  bool m_isHistogram;
  /// If true the data already has the bin widths divided in
  bool m_dataIsNormalized;
  /// This field can be set true for a histogram workspace. If it's true x(i) returns (X[i]+X[i+1])/2
  bool m_binCentres;
  /// Indicates that the data is plotted on a log y scale
  bool m_logScale;
  /// lowest positive y value
  mutable double m_minPositive;
  /// Is plotting as distribution
  bool m_isDistribution;

};
#endif
