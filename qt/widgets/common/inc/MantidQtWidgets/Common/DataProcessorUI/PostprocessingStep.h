#ifndef MANTIDQTWIDGETS_POSTPROCESSINGSTEP
#define MANTIDQTWIDGETS_POSTPROCESSINGSTEP
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtWidgets/Common/DataProcessorUI/WhiteList.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PostprocessingAlgorithm.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
#include <QString>
#include <QStringList>
#include <map>
namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
struct EXPORT_OPT_MANTIDQT_COMMON PostprocessingStep {
public:
  PostprocessingStep(const QString &options);
  PostprocessingStep(const QString &options, PostprocessingAlgorithm algorithm,
                     std::map<QString, QString> map);

  void postProcessGroup(const QString &processorPrefix,
                        const WhiteList &whitelist, const GroupData &groupData);
  QString getPostprocessedWorkspaceName(const WhiteList &whitelist,
                                        const GroupData &groupData,
                                        const QString &prefix = "");
  QString getReducedWorkspaceName(const WhiteList &whitelist,
                                  const QStringList &data,
                                  const QString &prefix = "");
  QString m_options;
  PostprocessingAlgorithm m_algorithm;
  std::map<QString, QString> m_map;

private:
  static void removeIfExists(QString const &workspaceName);
  static bool workspaceExists(QString const &workspaceName);
  static void removeWorkspace(QString const &workspaceName);
  void ensureRowSizeMatchesColumnCount(const WhiteList &columns,
                                       const QStringList &row);
};
}
}
}
#endif // MANTIDQTWIDGETS_POSTPROCESSINGSTEP
