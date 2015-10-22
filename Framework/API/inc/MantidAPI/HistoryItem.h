#ifndef MANTID_API_HISTORYITEM_H_
#define MANTID_API_HISTORYITEM_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/WorkspaceHistory.h"

namespace Mantid {
namespace API {

/** @class HistoryView

    This class wraps an algorithm history pointer to add additional
   functionality when creating a HistoryView.

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

class MANTID_API_DLL HistoryItem {
public:
  HistoryItem(AlgorithmHistory_const_sptr algHist);
  HistoryItem(const HistoryItem &A);
  virtual ~HistoryItem(){};

  bool isUnrolled() const { return m_unrolled; }
  void unrolled(bool unrolled) { m_unrolled = unrolled; }
  AlgorithmHistory_const_sptr getAlgorithmHistory() const {
    return m_algorithmHistory;
  }
  size_t numberOfChildren() const {
    return m_algorithmHistory->childHistorySize();
  }
  HistoryItem &operator=(const HistoryItem &A);

private:
  AlgorithmHistory_const_sptr m_algorithmHistory;
  bool m_unrolled;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_HISTORYITEM_H_*/
