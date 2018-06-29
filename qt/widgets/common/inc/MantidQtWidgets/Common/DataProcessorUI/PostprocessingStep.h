#ifndef MANTIDQTWIDGETS_POSTPROCESSINGSTEP
#define MANTIDQTWIDGETS_POSTPROCESSINGSTEP
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PostprocessingAlgorithm.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"
#include "MantidQtWidgets/Common/DataProcessorUI/WhiteList.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
#include <QString>
#include <QStringList>
#include <boost/optional.hpp>
#include <map>
namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
struct EXPORT_OPT_MANTIDQT_COMMON PostprocessingStep {
public:
  PostprocessingStep(QString options);
  PostprocessingStep(QString options, PostprocessingAlgorithm algorithm,
                     std::map<QString, QString> map);

  void postProcessGroup(const QString &outputWSName,
                        const QString &rowOutputWSPropertyName,
                        const WhiteList &whitelist, const GroupData &groupData);
  QString getPostprocessedWorkspaceName(
      const GroupData &groupData,
      boost::optional<size_t> sliceIndex = boost::optional<size_t>()) const;
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
