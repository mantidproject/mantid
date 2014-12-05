#ifndef MANTID_CUSTOMINTERFACES_IALCBASELINEMODELLINGMODEL_H_
#define MANTID_CUSTOMINTERFACES_IALCBASELINEMODELLINGMODEL_H_

#include "MantidKernel/System.h"

#include "MantidQtCustomInterfaces/DllConfig.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IFunction.h"

#include <QObject>

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{

  /** IALCBaselineModellingModel : Model interface for ALC BaselineModelling step
    
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
  class MANTIDQT_CUSTOMINTERFACES_DLL IALCBaselineModellingModel : public QObject
  {
    Q_OBJECT

  public:
    typedef std::pair<double, double> Section;

    /**
     * @return Function produced by the last fit
     */
    virtual IFunction_const_sptr fittedFunction() const = 0;

    /**
     * @return Corrected data produced by the last fit
     */
    virtual MatrixWorkspace_const_sptr correctedData() const = 0;

    /**
     * @return Current data used for fitting
     */
    virtual MatrixWorkspace_const_sptr data() const = 0;

    /**
     * Perform a fit using current data and specified function and sections. Modified values returned
     * by fittedFunction and correctedData.
     * @param function :: Function to fit
     * @param sections :: Data sections to include in the fit
     */
    virtual void fit(IFunction_const_sptr function, const std::vector<Section>& sections) = 0;

  signals:

    // Signals emitted when various properties get changed
    void dataChanged();
    void fittedFunctionChanged();
    void correctedDataChanged();
  };

} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTID_CUSTOMINTERFACES_IALCBASELINEMODELLINGMODEL_H_ */
