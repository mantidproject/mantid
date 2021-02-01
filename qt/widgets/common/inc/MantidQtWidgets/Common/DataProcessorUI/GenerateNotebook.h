// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

/** @class GenerateNotebook

    This class creates ipython notebooks from the ISIS DataProcessorUI
    (Polref) interface
    */

#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsMap.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PostprocessingStep.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PreprocessingAlgorithm.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ProcessingAlgorithm.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"
#include "MantidQtWidgets/Common/DataProcessorUI/WhiteList.h"

#include <QStringList>
#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>
#include <map>
#include <set>
#include <sstream>
#include <string>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

QStringList DLLExport splitByCommas(const QString &namesString);

QString DLLExport plot1DString(const QStringList &ws_names);

QString DLLExport tableString(const TreeData &treeData, const WhiteList &whitelist);

QString DLLExport titleString(const QString &wsName);

boost::tuple<QString, QString> DLLExport postprocessGroupString(const GroupData &rowMap,
                                                                const ProcessingAlgorithm &processor,
                                                                const PostprocessingStep &postprocessingStep);

QString DLLExport plotsString(const GroupData &groupData, const QString &stitched_wsStr,
                              const ProcessingAlgorithm &processor);

QString DLLExport reduceRowString(const RowData_sptr &data, const QString &instrument, const WhiteList &whitelist,
                                  const std::map<QString, PreprocessingAlgorithm> &preprocessMap,
                                  const ProcessingAlgorithm &processor,
                                  const ColumnOptionsMap &globalPreprocessingOptionsMap);

boost::tuple<QString, QString> DLLExport loadWorkspaceString(const QString &runStr, const QString &instrument,
                                                             const PreprocessingAlgorithm &preprocessor,
                                                             const QString &options);

QString DLLExport preprocessString(const QString &input_name1, const QString &input_name2, const QString &output_name,
                                   const PreprocessingAlgorithm &preprocessor, const QString &options);

boost::tuple<QString, QString> DLLExport loadRunString(const QString &run, const QString &instrument,
                                                       const QString &prefix, const QString &outputName = QString());

QString DLLExport completeOutputProperties(const QString &algName, size_t currentProperties);

class DLLExport GenerateNotebook {

public:
  GenerateNotebook(const QString &name, const QString &instrument, WhiteList whitelist,
                   std::map<QString, PreprocessingAlgorithm> preprocessMap, const ProcessingAlgorithm &processor,
                   boost::optional<PostprocessingStep> postprocessingStep,
                   ColumnOptionsMap preprocessingInstructionsMap);
  virtual ~GenerateNotebook() = default;

  QString generateNotebook(const TreeData &data);

private:
  bool hasPostprocessing() const;

  // The table ws name
  const QString m_wsName;
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
  boost::optional<PostprocessingStep> m_postprocessingStep;
  // A map containing pre-processing instructions displayed in the view via
  // hinting line edits
  ColumnOptionsMap m_preprocessingOptionsMap;
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt