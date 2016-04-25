#ifndef MANTID_CUSTOMINTERFACES_DATAPROCESSORGENERATENOTEBOOK_H
#define MANTID_CUSTOMINTERFACES_DATAPROCESSORGENERATENOTEBOOK_H

/** @class DataProcessorGenerateNotebook

    This class creates ipython notebooks from the ISIS Reflectometry
    (Polref) interface

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

#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataPostprocessorAlgorithm.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataPreprocessorAlgorithm.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorAlgorithm.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorWhiteList.h"
#include "MantidQtCustomInterfaces/Reflectometry/QDataProcessorTableModel.h"

#include <boost/tuple/tuple.hpp>
#include <map>
#include <set>
#include <sstream>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

std::vector<std::string>
    DLLExport splitByCommas(const std::string &names_string);

std::string DLLExport plot1DString(const std::vector<std::string> &ws_names);

std::string DLLExport tableString(QDataProcessorTableModel_sptr model,
                                  const DataProcessorWhiteList &whitelist,
                                  const std::set<int> &rows);

std::string DLLExport titleString(const std::string &wsName);

boost::tuple<std::string, std::string> DLLExport postprocessGroupString(
    const std::set<int> &rows, const std::string &instrument,
    QDataProcessorTableModel_sptr model,
    const DataProcessorWhiteList &whitelist,
    const std::map<std::string, DataPreprocessorAlgorithm> &preprocessMap,
    const DataProcessorAlgorithm &processor,
    const DataPostprocessorAlgorithm &postprocessor);

std::string DLLExport plotsString(const std::vector<std::string> &output_ws,
                                  const std::string &stitched_wsStr,
                                  const DataProcessorAlgorithm &processor);

std::string DLLExport getWorkspaceName(
    int rowNo, QDataProcessorTableModel_sptr model,
    const DataProcessorWhiteList &whitelist,
    const std::map<std::string, DataPreprocessorAlgorithm> &preprocessMap,
    const DataProcessorAlgorithm &processor, bool prefix);

boost::tuple<std::string, std::string> DLLExport reduceRowString(
    const int rowNo, const std::string &instrument,
    QDataProcessorTableModel_sptr model,
    const DataProcessorWhiteList &whitelist,
    const std::map<std::string, DataPreprocessorAlgorithm> &preprocessMap,
    const DataProcessorAlgorithm &processor);

boost::tuple<std::string, std::string>
loadWorkspaceString(const std::string &runStr, const std::string &instrument,
                    const DataPreprocessorAlgorithm &preprocessor);

std::string DLLExport plusString(const std::string &input_name,
                                 const std::string &output_name,
                                 const DataPreprocessorAlgorithm &preprocessor);

boost::tuple<std::string, std::string>
    DLLExport loadRunString(const std::string &run,
                            const std::string &instrument,
                            const std::string &prefix);

std::string DLLExport completeOutputProperties(const std::string &algName,
                                               size_t currentProperties);

class DLLExport DataProcessorGenerateNotebook {

public:
  DataProcessorGenerateNotebook(
      std::string name, QDataProcessorTableModel_sptr model,
      const std::string instrument, const DataProcessorWhiteList &whitelist,
      const std::map<std::string, DataPreprocessorAlgorithm> &preprocessMap,
      const DataProcessorAlgorithm &processor,
      const DataPostprocessorAlgorithm &postprocessor);
  virtual ~DataProcessorGenerateNotebook(){};

  std::string generateNotebook(std::map<int, std::set<int>> groups,
                               std::set<int> rows);

private:
  std::string m_wsName;
  QDataProcessorTableModel_sptr m_model;
  const std::string m_instrument;
  DataProcessorWhiteList m_whitelist;
  std::map<std::string, DataPreprocessorAlgorithm> m_preprocessMap;
  DataProcessorAlgorithm m_processor;
  DataPostprocessorAlgorithm m_postprocessor;
};
}
}

#endif // MANTID_CUSTOMINTERFACES_DATAPROCESSORGENERATENOTEBOOK_H
