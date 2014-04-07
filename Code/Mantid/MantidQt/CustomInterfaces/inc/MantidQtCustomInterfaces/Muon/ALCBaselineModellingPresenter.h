#ifndef MANTIDQT_CUSTOMINTERFACES_ALCBASELINEMODELLINGPRESENTER_H_
#define MANTIDQT_CUSTOMINTERFACES_ALCBASELINEMODELLINGPRESENTER_H_

#include "MantidKernel/System.h"

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Muon/IALCBaselineModellingView.h"

#include <QObject>

namespace MantidQt
{
namespace CustomInterfaces
{

  /** ALCBaselineModellingPresenter : Presenter for ALC Baseline Modelling step
    
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
  class MANTIDQT_CUSTOMINTERFACES_DLL ALCBaselineModellingPresenter : public QObject
  {
    Q_OBJECT

  public:
    ALCBaselineModellingPresenter(IALCBaselineModellingView* view);

    void initialize();

    /// @param data :: Data to fit peaks in
    void setData(MatrixWorkspace_const_sptr data);

    /// @return Corrected data calculated after the last fit
    MatrixWorkspace_const_sptr correctedData() const { return m_correctedData; }

  private slots:
    /// Perform fit
    void fit();

  private:
    /// Returns a filtered copy of m_data, where all uninteresting points where disabled.
    /// Unintersing points are ones which are not included in any of the sections specified in the
    /// view. Disabled here means "won't be used when fitting".
    /// @return A copy of m_data which we can pass to Fit algorithm
    MatrixWorkspace_sptr filteredData() const;

    /// Associated view
    IALCBaselineModellingView* const m_view;

    /// Data we are fitting the baseline to
    MatrixWorkspace_const_sptr m_data;

    /// Corrected data of the last fit
    MatrixWorkspace_const_sptr m_correctedData;
  };

} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTIDQT_CUSTOMINTERFACES_ALCBASELINEMODELLINGPRESENTER_H_ */
