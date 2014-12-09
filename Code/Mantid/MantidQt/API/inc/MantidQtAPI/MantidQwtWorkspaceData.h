#ifndef MANTIDQTAPI_MANTIDQWTWORKSPACEDATA_H
#define MANTIDQTAPI_MANTIDQWTWORKSPACEDATA_H
/**
  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

#include "qwt_data.h"
#include "DllOption.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
namespace Mantid
{
  namespace API
  {
    class MatrixWorkspace;
  }
}

/**
 * Base class for Workspace Qwt data types
 */
class EXPORT_OPT_MANTIDQT_API MantidQwtWorkspaceData : public QwtData
{
public:
  virtual void setLogScale(bool on) = 0;
  virtual bool logScale() const = 0;
  virtual void saveLowestPositiveValue(const double v) = 0;
  virtual size_t esize() const = 0;
  virtual double e(size_t i)const = 0;
  virtual double ex(size_t i)const = 0;
  virtual double getYMin() const = 0;
  virtual double getYMax() const = 0;
  virtual QString getXAxisLabel() const = 0;
  virtual QString getYAxisLabel() const = 0;

protected:
  // Assignment operator (virtualized).
  MantidQwtWorkspaceData& operator=(const MantidQwtWorkspaceData&); // required by QwtData base class
};

/**
 * Base class for MatrixWorkspace Qwt data types
 */
class EXPORT_OPT_MANTIDQT_API MantidQwtMatrixWorkspaceData : public MantidQwtWorkspaceData
{
public:
  /// Return a new data object of the same type but with a new workspace
  virtual MantidQwtMatrixWorkspaceData* copyWithNewSource(const Mantid::API::MatrixWorkspace & workspace) const = 0;

protected:
  // Assignment operator (virtualized).
  MantidQwtMatrixWorkspaceData& operator=(const MantidQwtMatrixWorkspaceData&); // required by QwtData base class
};

#endif
