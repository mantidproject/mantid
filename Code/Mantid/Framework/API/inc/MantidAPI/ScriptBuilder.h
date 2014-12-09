#ifndef MANTID_API_SCRIPTBUILDER_H_
#define MANTID_API_SCRIPTBUILDER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/HistoryView.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidKernel/PropertyHistory.h"

#include <sstream>

namespace Mantid
{
namespace API
{

/** @class ScriptBuilder

    This class build a sttring which cana be executed as a python script.

    @author Samuel Jackson, ISIS, RAL
    @date 21/01/2008

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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


class MANTID_API_DLL ScriptBuilder
{
public:
  ScriptBuilder(boost::shared_ptr<HistoryView> view, std::string versionSpecificity = "old");
  virtual ~ScriptBuilder() {};

  /// build a python script from the history view
  const std::string build();
  
private:
  void writeHistoryToStream(std::ostringstream& os, std::vector<HistoryItem>::const_iterator& iter, int depth = 1);
  void buildChildren(std::ostringstream& os, std::vector<HistoryItem>::const_iterator& iter, int depth = 1);
  const std::string buildAlgorithmString(AlgorithmHistory_const_sptr algHistory);
  const std::string buildPropertyString(Mantid::Kernel::PropertyHistory_const_sptr propHistory);

  const std::vector<HistoryItem> m_historyItems;
  std::string m_output;
  std::string m_versionSpecificity;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_SCRIPTBUILDER_H_*/
