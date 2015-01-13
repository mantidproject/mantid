#ifndef MANTID_API_HISTORYVIEW_H_
#define MANTID_API_HISTORYVIEW_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/HistoryItem.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidKernel/DateAndTime.h"

#include <list>
#include <vector>

namespace Mantid {
namespace API {

/** @class HistoryView

    This class builds a view of the algorithm history by "unrolling" parent
   algorithms.

    @author Samuel Jackson, ISIS, RAL
    @date 21/01/2008

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */

class MANTID_API_DLL HistoryView {
public:
  HistoryView(const WorkspaceHistory &wsHist);
  virtual ~HistoryView(){};

  void unroll(size_t index);
  void unrollAll();
  void roll(size_t index);
  void rollAll();
  void filterBetweenExecDate(Mantid::Kernel::DateAndTime start,
                             Mantid::Kernel::DateAndTime end =
                                 Mantid::Kernel::DateAndTime::getCurrentTime());
  const std::vector<HistoryItem> getAlgorithmsList() const;
  size_t size() const { return m_historyItems.size(); }

private:
  void unroll(std::list<HistoryItem>::iterator it);
  void roll(std::list<HistoryItem>::iterator it);

  const WorkspaceHistory m_wsHist;
  std::list<HistoryItem> m_historyItems;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_HISTORYVIEW_H_*/
