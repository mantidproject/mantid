// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTAPI_QWTWORKSPACEBINDATA_H
#define MANTIDQTAPI_QWTWORKSPACEBINDATA_H

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidQtWidgets/Plotting/DllOption.h"
#include "MantidQtWidgets/Plotting/Qwt/MantidQwtWorkspaceData.h"

#include <boost/shared_ptr.hpp>

#include <QString>

//=================================================================================================
//=================================================================================================
/**  This class implements QwtData with direct access to a spectrum in a
 * MatrixWorkspace.
 */
class EXPORT_OPT_MANTIDQT_PLOTTING QwtWorkspaceBinData
    : public MantidQwtMatrixWorkspaceData {
public:
  QwtWorkspaceBinData(const Mantid::API::MatrixWorkspace &workspace,
                      int binIndex, const bool logScale);

  //! @return Pointer to a copy (virtual copy constructor)
  QwtWorkspaceBinData *copy() const override;

  /// Return a new data object of the same type but with a new workspace
  QwtWorkspaceBinData *copyWithNewSource(
      const Mantid::API::MatrixWorkspace &workspace) const override;

  //! @return Size of the data set
  size_t size() const override;
  /// Return the label to use for the X axis
  QString getXAxisLabel() const override;
  /// Return the label to use for the Y axis
  QString getYAxisLabel() const override;

protected:
  // Assignment operator.
  QwtWorkspaceBinData &operator=(const QwtWorkspaceBinData & /*rhs*/);
  /**
  Return the x value of data point i
  @param i :: Index
  @return x X value of data point i
  */
  double getX(size_t i) const override;
  /**
  Return the y value of data point i
  @param i :: Index
  @return y Y value of data point i
  */
  double getY(size_t i) const override;

  /// Returns the error of the i-th data point
  double getE(size_t i) const override;
  /// Returns the x position of the error bar for the i-th data point (bin)
  double getEX(size_t i) const override;

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
};
#endif
