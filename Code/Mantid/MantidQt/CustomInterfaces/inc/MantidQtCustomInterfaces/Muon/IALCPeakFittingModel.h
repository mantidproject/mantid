#ifndef MANTID_CUSTOMINTERFACES_IALCPEAKFITTINGMODEL_H_
#define MANTID_CUSTOMINTERFACES_IALCPEAKFITTINGMODEL_H_

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidKernel/System.h"

#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <QObject>

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{

  /** IALCPeakFittingModel : ALC peak fitting step model interface.
    
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
  class MANTIDQT_CUSTOMINTERFACES_DLL IALCPeakFittingModel : public QObject
  {
    Q_OBJECT

  public:
    /**
     * @return Last fitted peaks
     */
    virtual IFunction_const_sptr fittedPeaks() const = 0;

    /**
     * @return Data we are fitting peaks to
     */
    virtual MatrixWorkspace_const_sptr data() const = 0;

    /**
     * Fit specified peaks to the data of the model
     * @param peaks :: Function representing peaks to fit
     */
    virtual void fitPeaks(IFunction_const_sptr peaks) = 0;

  signals:

    /// Signal to inform that the fitting was done and fitted peaks were updated
    void fittedPeaksChanged();

    /// Signal to inform that data was set
    void dataChanged();
  };


} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTID_CUSTOMINTERFACES_IALCPEAKFITTINGMODEL_H_ */
