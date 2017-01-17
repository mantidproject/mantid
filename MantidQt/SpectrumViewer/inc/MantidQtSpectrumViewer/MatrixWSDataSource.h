#ifndef MATRIX_WS_DATA_SOURCE_H
#define MATRIX_WS_DATA_SOURCE_H

#include <cstddef>

#include "MantidQtSpectrumViewer/DataArray.h"
#include "MantidQtSpectrumViewer/SpectrumDataSource.h"
#include "MantidQtSpectrumViewer/DllOptionSV.h"

#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Mantid {
namespace Geometry {
class IComponent;
class Instrument;
}
namespace API {
class SpectrumInfo;
}
}

namespace MantidQt {
namespace SpectrumView {
class EModeHandler;

/**
    @class MatrixWSDataSource

    This class provides a concrete implementation of an SpectrumDataSource
    that gets it's data from a matrix workspace.

    @author Dennis Mikkelson
    @date   2012-05-08

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
class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER MatrixWSDataSource
    : public SpectrumDataSource {
public:
  /// Construct a DataSource object around the specifed MatrixWorkspace
  MatrixWSDataSource(Mantid::API::MatrixWorkspace_const_sptr matWs);

  ~MatrixWSDataSource() override;

  bool hasData(const std::string &wsName,
               const boost::shared_ptr<Mantid::API::Workspace> ws) override;

  /// Get the smallest 'x' value covered by the data
  double getXMin() override;

  /// Get the largest 'x' value covered by the data
  double getXMax() override;

  /// Get the largest 'y' value covered by the data
  double getYMax() override;

  /// Get the total number of rows of data
  size_t getNRows() override;

  /// Get DataArray covering full range of data in x, and y directions
  DataArray_const_sptr getDataArray(bool isLogX) override;

  /// Get DataArray covering restricted range of data
  DataArray_const_sptr getDataArray(double xMin, double xMax, double yMin,
                                    double yMax, size_t nRows, size_t nCols,
                                    bool isLogX) override;

  /// Set the class that gets the emode & efixed info from the user.
  void setEModeHandler(EModeHandler *emodeHandler);

  /// Get a list containing pairs of strings with information about x,y
  std::vector<std::string> getInfoList(double x, double y) override;

  /// Get a pointer to the workspace
  Mantid::API::MatrixWorkspace_const_sptr getWorkspace() const {
    return m_matWs;
  }

private:
  Mantid::API::MatrixWorkspace_const_sptr m_matWs;
  EModeHandler *m_emodeHandler;
  boost::shared_ptr<const Mantid::Geometry::Instrument> m_instrument;
  boost::shared_ptr<const Mantid::Geometry::IComponent> m_source;
  boost::shared_ptr<const Mantid::Geometry::IComponent> m_sample;
  const Mantid::API::SpectrumInfo &m_spectrumInfo;
};

typedef boost::shared_ptr<MatrixWSDataSource> MatrixWSDataSource_sptr;
typedef boost::shared_ptr<const MatrixWSDataSource>
    MatrixWSDataSource_const_sptr;

} // namespace SpectrumView
} // namespace MantidQt

#endif // MATRIX_WS_DATA_SOURCE_H
