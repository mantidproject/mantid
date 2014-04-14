#ifndef MANTIDQT_CUSTOMINTERFACES_IALCPEAKFITTINGVIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_IALCPEAKFITTINGVIEW_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace.h"

#include "MantidQtCustomInterfaces/DllConfig.h"

#include <QObject>
#include <qwt_data.h>

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{

  /** IALCPeakFittingView : Interface for ALC Peak Fitting step view.
    
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class MANTIDQT_CUSTOMINTERFACES_DLL IALCPeakFittingView : public QObject
  {
    Q_OBJECT

  public:
    /// @return Composite peaks function selected by user
    virtual std::string function() const = 0;

  public slots:
    /// Performs any necessary initialization
    virtual void initialize() = 0;

    /// Update the data curve displayed
    /// @param data :: New curve data
    virtual void setDataCurve(const QwtData& data) = 0;

    /// Update the fitted curve displayed
    /// @param data :: New curve data
    virtual void setFittedCurve(const QwtData& data) = 0;

    /// Set function displayed
    /// @param newFunction :: New function to display
    virtual void setFunction(const std::string& newFunction) = 0;

  signals:
    /// Request to perform peak fitting
    void fitRequested();

  };


} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTIDQT_CUSTOMINTERFACES_IALCPEAKFITTINGVIEW_H_ */
