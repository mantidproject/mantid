#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORGENERATENOTEBOOK_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORGENERATENOTEBOOK_H

/** @class DataProcessorGenerateNotebook

    This class creates ipython notebooks from the ISIS DataProcessorUI
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
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPostprocessingAlgorithm.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPreprocessingAlgorithm.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorProcessingAlgorithm.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorWhiteList.h"

#include <boost/tuple/tuple.hpp>
#include <map>
#include <set>
#include <sstream>
#include <string>

namespace MantidQt {
namespace MantidWidgets {

using RowData = std::vector<std::string>;
using GroupData = std::map<int, RowData>;
using TreeData = std::map<int, GroupData>;

std::vector<std::string> DLLExport
splitByCommas(const std::string &names_string);

std::string DLLExport plot1DString(const std::vector<std::string> &ws_names);

std::string DLLExport
tableString(const TreeData &treeData, const DataProcessorWhiteList &whitelist);

std::string DLLExport titleString(const std::string &wsName);

boost::tuple<std::string, std::string> DLLExport postprocessGroupString(
    const GroupData &rowMap, const DataProcessorWhiteList &whitelist,
    const DataProcessorProcessingAlgorithm &processor,
    const DataProcessorPostprocessingAlgorithm &postprocessor,
    const std::string &postprocessingOptions);

std::string DLLExport
plotsString(const std::vector<std::string> &output_ws,
            const std::string &stitched_wsStr,
            const DataProcessorProcessingAlgorithm &processor);

std::string DLLExport
getReducedWorkspaceName(const RowData &data,
                        const DataProcessorWhiteList &whitelist,
                        const std::string &prefix = "");

boost::tuple<std::string, std::string> DLLExport reduceRowString(
    const RowData &data, const std::string &instrument,
    const DataProcessorWhiteList &whitelist,
    const std::map<std::string, DataProcessorPreprocessingAlgorithm> &
        preprocessMap,
    const DataProcessorProcessingAlgorithm &processor,
    const std::map<std::string, std::string> &preprocessOoptionsMap,
    const std::string &processingOptions);

boost::tuple<std::string, std::string> DLLExport
loadWorkspaceString(const std::string &runStr, const std::string &instrument,
                    const DataProcessorPreprocessingAlgorithm &preprocessor,
                    const std::string &options);

std::string DLLExport
plusString(const std::string &input_name, const std::string &output_name,
           const DataProcessorPreprocessingAlgorithm &preprocessor,
           const std::string &options);

boost::tuple<std::string, std::string> DLLExport
loadRunString(const std::string &run, const std::string &instrument,
              const std::string &prefix);

std::string DLLExport
completeOutputProperties(const std::string &algName, size_t currentProperties);

class DLLExport DataProcessorGenerateNotebook {

public:
  DataProcessorGenerateNotebook(
      std::string name, const std::string instrument,
      const DataProcessorWhiteList &whitelist,
      const std::map<std::string, DataProcessorPreprocessingAlgorithm> &
          preprocessMap,
      const DataProcessorProcessingAlgorithm &processor,
      const DataProcessorPostprocessingAlgorithm &postprocessor,
      const std::map<std::string, std::string> preprocessingInstructionsMap,
      const std::string processingInstructions,
      const std::string postprocessingInstructions);
  virtual ~DataProcessorGenerateNotebook(){};

  std::string generateNotebook(const TreeData &data);

private:
  // The table ws name
  std::string m_wsName;
  // The instrument
  const std::string m_instrument;
  // The whitelist defining the number of columns, their names and how they
  // relate to the algorithm properties
  DataProcessorWhiteList m_whitelist;
  // The map indicating the columns that were pre-processed and their
  // corresponding pre-processing algorithms
  std::map<std::string, DataProcessorPreprocessingAlgorithm> m_preprocessMap;
  // The processing (reduction) algorithm
  DataProcessorProcessingAlgorithm m_processor;
  // The post-processing algorithm
  DataProcessorPostprocessingAlgorithm m_postprocessor;
  // A map containing pre-processing instructions displayed in the view via
  // hinting line edits
  std::map<std::string, std::string> m_preprocessingOptionsMap;
  // Options to reduction algorithm specified in the view via hinting line edit
  std::string m_processingOptions;
  // Options to post-processing algorithm specified in the view via hinting line
  // edit
  std::string m_postprocessingOptions;
};
}
}

#endif // MANTIDQTMANTIDWIDGETS_DATAPROCESSORGENERATENOTEBOOK_H
