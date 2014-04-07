#ifndef MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGPRESENTER_H_
#define MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGPRESENTER_H_

#include "MantidKernel/System.h"

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Muon/IALCPeakFittingView.h"

namespace MantidQt
{
namespace CustomInterfaces
{

  /** ALCPeakFittingPresenter : Presenter for Peak Fitting step of ALC interface.
    
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
  class MANTIDQT_CUSTOMINTERFACES_DLL ALCPeakFittingPresenter : public QObject
  {
    Q_OBJECT

  public:
    ALCPeakFittingPresenter(IALCPeakFittingView* view);

    void initialize();

    /// @param data :: Data to fit peaks in
    void setData(MatrixWorkspace_const_sptr data);

  private slots:
    /// Fit the data using the peaks from the view, and update them
    void fit();

  private:
    /// Associated view
    IALCPeakFittingView* const m_view;

    /// Data we are fitting peaks in
    MatrixWorkspace_const_sptr m_data;
  };


} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGPRESENTER_H_ */
