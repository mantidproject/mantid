#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORGENERATENOTEBOOK_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORGENERATENOTEBOOK_H

/** @class GenerateNotebook

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
#include "MantidQtWidgets/Common/DataProcessorUI/PostprocessingStep.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PreprocessingAlgorithm.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ProcessingAlgorithm.h"
#include "MantidQtWidgets/Common/DataProcessorUI/WhiteList.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"

#include <boost/tuple/tuple.hpp>
#include <QStringList>
#include <map>
#include <set>
#include <sstream>
#include <string>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

QStringList DLLExport splitByCommas(const QString &namesString);

QString DLLExport plot1DString(const QStringList &ws_names);

QString DLLExport
tableString(const TreeData &treeData, const WhiteList &whitelist);

QString DLLExport titleString(const QString &wsName);

boost::tuple<QString, QString> DLLExport
postprocessGroupString(const GroupData &rowMap, const WhiteList &whitelist,
                       const ProcessingAlgorithm &processor,
                       const PostprocessingStep &postprocessingStep);

QString DLLExport plotsString(const QStringList &output_ws,
                              const QString &stitched_wsStr,
                              const ProcessingAlgorithm &processor);

QString DLLExport getReducedWorkspaceName(const RowData &data,
                                          const WhiteList &whitelist,
                                          const QString &prefix = "");

boost::tuple<QString, QString> DLLExport
reduceRowString(const RowData &data, const QString &instrument,
                const WhiteList &whitelist,
                const std::map<QString, PreprocessingAlgorithm> &preprocessMap,
                const ProcessingAlgorithm &processor,
                const std::map<QString, QString> &preprocessOoptionsMap,
                const QString &processingOptions);

boost::tuple<QString, QString> DLLExport
loadWorkspaceString(const QString &runStr, const QString &instrument,
                    const PreprocessingAlgorithm &preprocessor,
                    const QString &options);

QString DLLExport
plusString(const QString &input_name, const QString &output_name,
           const PreprocessingAlgorithm &preprocessor, const QString &options);

boost::tuple<QString, QString> DLLExport
loadRunString(const QString &run, const QString &instrument,
              const QString &prefix);

QString DLLExport
completeOutputProperties(const QString &algName, size_t currentProperties);

class DLLExport GenerateNotebook {

public:
  GenerateNotebook(
      QString name, const QString instrument, const WhiteList &whitelist,
      const std::map<QString, PreprocessingAlgorithm> &preprocessMap,
      const ProcessingAlgorithm &processor,
      const PostprocessingStep &postprocessingStep,
      const std::map<QString, QString> preprocessingInstructionsMap,
      const QString processingInstructions);
  virtual ~GenerateNotebook(){};

  QString generateNotebook(const TreeData &data);

private:
  // The table ws name
  QString m_wsName;
  // The instrument
  const QString m_instrument;
  // The whitelist defining the number of columns, their names and how they
  // relate to the algorithm properties
  WhiteList m_whitelist;
  // The map indicating the columns that were pre-processed and their
  // corresponding pre-processing algorithms
  std::map<QString, PreprocessingAlgorithm> m_preprocessMap;
  // The processing (reduction) algorithm
  ProcessingAlgorithm m_processor;
  // The post-processing algorithm
  PostprocessingStep m_postprocessingStep;
  // A map containing pre-processing instructions displayed in the view via
  // hinting line edits
  std::map<QString, QString> m_preprocessingOptionsMap;
  // Options to reduction algorithm specified in the view via hinting line edit
  QString m_processingOptions;
  // Options to post-processing algorithm specified in the view via hinting line
  // edit
};
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_DATAPROCESSORGENERATENOTEBOOK_H
