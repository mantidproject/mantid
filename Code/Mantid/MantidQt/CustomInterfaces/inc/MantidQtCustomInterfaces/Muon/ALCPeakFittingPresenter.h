#ifndef MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGPRESENTER_H_
#define MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGPRESENTER_H_

#include "MantidKernel/System.h"

#include "MantidQtCustomInterfaces/DllConfig.h"

#include "MantidQtCustomInterfaces/Muon/IALCPeakFittingView.h"
#include "MantidQtCustomInterfaces/Muon/IALCPeakFittingModel.h"

namespace MantidQt
{
namespace CustomInterfaces
{

  /** ALCPeakFittingPresenter : Presenter for Peak Fitting step of ALC interface.
    
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
  class MANTIDQT_CUSTOMINTERFACES_DLL ALCPeakFittingPresenter : public QObject
  {
    Q_OBJECT

  public:
    ALCPeakFittingPresenter(IALCPeakFittingView* view, IALCPeakFittingModel* model);

    void initialize();

  private slots:
    /// Fit the data using the peaks from the view, and update them
    void fit();

    /// Executed when user selects a function in a Function Browser
    void onCurrentFunctionChanged();

    /// Executed when Peak Picker if moved/resized
    void onPeakPickerChanged();

    /// Executed when user changes parameter in Function Browser
    void onParameterChanged(const QString& funcIndex);

    void onFittedPeaksChanged();
    void onDataChanged();

  private:
    /// Associated view
    IALCPeakFittingView* const m_view;

    /// Associated model
    IALCPeakFittingModel* const m_model;
  };


} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGPRESENTER_H_ */
