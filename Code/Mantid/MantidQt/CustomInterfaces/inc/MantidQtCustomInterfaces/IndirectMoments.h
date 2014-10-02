#ifndef MANTID_CUSTOMINTERFACES_INDIRECTMOMENTS_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTMOMENTS_H_

#include "MantidQtCustomInterfaces/IndirectDataReductionTab.h"

#include "MantidKernel/System.h"

#include <QFont>

namespace MantidQt
{
namespace CustomInterfaces
{
  /** IndirectMoments : TODO: DESCRIPTION
    

    @author Samuel Jackson
    @date 13/08/2013

    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport IndirectMoments : public IndirectDataReductionTab
  {
    Q_OBJECT

  public:
    IndirectMoments(Ui::IndirectDataReduction& uiForm, QWidget * parent = 0);
    virtual ~IndirectMoments();

    virtual void setup();
    virtual void run();
    virtual bool validate();

  protected slots:
    // Handle when a file/workspace is ready for plotting
    void handleSampleInputReady(const QString&);
    /// Slot for when the min range on the range selector changes
    void minValueChanged(double min);
    /// Slot for when the min range on the range selector changes
    void maxValueChanged(double max);
    /// Slot to update the guides when the range properties change
    void updateProperties(QtProperty* prop, double val);

  };
} // namespace CustomInterfaces
} // namespace Mantid

#endif  /* MANTID_CUSTOMINTERFACES_INDIRECTMOMENTS_H_ */
