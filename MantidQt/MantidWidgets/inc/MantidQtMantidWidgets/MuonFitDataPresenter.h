#ifndef MANTID_MANTIDWIDGETS_MUONFITDATAPRESENTER_H_
#define MANTID_MANTIDWIDGETS_MUONFITDATAPRESENTER_H_

#include "WidgetDllOption.h"
#include "MantidKernel/make_unique.h"
#include "MantidQtMantidWidgets/IMuonFitDataView.h"

namespace MantidQt {
namespace MantidWidgets {

/** MuonFitDataPresenter : Select runs, periods, groups to fit

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS MuonFitDataPresenter
    {
public:
  /// constructor
  explicit MuonFitDataPresenter(IMuonFitDataView *view);
  void setAvailableGroups(const QStringList &groupNames);
  QStringList getChosenGroups() const;
  void setNumPeriods(size_t numPeriods);
  QStringList getChosenPeriods() const;

private:
  /// Pointer to view
  IMuonFitDataView *m_view;
  /// Number of periods
  size_t m_numPeriods;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_MANTIDWIDGETS_MUONFITDATAPRESENTER_H_ */