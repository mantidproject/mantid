#ifndef REBINNEDSOURCESMANAGER_H_
#define REBINNEDSOURCESMANAGER_H_

#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "MantidQtAPI/WorkspaceObserver.h"
#include "MantidAPI/Workspace_fwd.h"

// Have to deal with ParaView warnings and Intel compiler the hard way.
#if defined(__INTEL_COMPILER)
#pragma warning disable 1170
#endif

#include <pqPipelineSource.h>
#include <pqPipelineFilter.h>
#include <vtkSMProxy.h>

#if defined(__INTEL_COMPILER)
#pragma warning enable 1170
#endif

#include <QWidget>
#include <QList>
#include <map>
#include <string>

namespace Mantid {
namespace Vates {
namespace SimpleGui {
/**
 *
 This class  keeps track of the MDEvent workspaces and associated rebinned
 workspaces. Rebinning requires temporary workspaces instead of
 the original MDEvent workspaces. This class switches between these types of
 sources.

 @date 21/01/2015

 Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS RebinnedSourcesManager
    : public QWidget,
      MantidQt::API::WorkspaceObserver {
  Q_OBJECT
public:
  RebinnedSourcesManager(QWidget *parent = nullptr);

  ~RebinnedSourcesManager() override;

  void checkSource(pqPipelineSource *source, std::string &inputWorkspace,
                   std::string &outputWorkspace, std::string algorithmType);

  void repipeRebinnedSource();

  void repipeOriginalSource(pqPipelineSource *rebinnedSource,
                            pqPipelineSource *originalSource);

  void getStoredWorkspaceNames(pqPipelineSource *source,
                               std::string &originalWorkspaceName,
                               std::string &rebinnedWorkspaceName);

  void registerRebinnedSource(pqPipelineSource *source);

  bool isRebinnedSourceBeingTracked(pqPipelineSource *source);

  /// Save the state of the manager to a Mantid project file
  std::string saveToProject();
  /// Load the state of the manager from a Mantid project file
  void loadFromProject(const std::string &lines);

signals:
  void switchSources(std::string rebinnedWorkspaceName, std::string sourceType);

  void triggerAcceptForNewFilters();

protected:
  void
  addHandle(const std::string &workspaceName,
            const boost::shared_ptr<Mantid::API::Workspace> workspace) override;

  void
  preDeleteHandle(const std::string &wsName,
                  const boost::shared_ptr<Mantid::API::Workspace>) override;

  void afterReplaceHandle(
      const std::string &workspaceName,
      const boost::shared_ptr<Mantid::API::Workspace> workspace) override;

private slots:
  void onRebinnedSourceDestroyed();

private:
  std::map<std::pair<std::string, std::string>, std::string>
      m_rebinnedWorkspaceAndSourceToOriginalWorkspace; ///< Holds a mapping from
                                                       ///(RebinnedWsName,
  /// RebinnedSourceName
  /// unique ID) to
  ///(OriginalWsName)
  std::map<std::string, std::pair<std::string, pqPipelineSource *>>
      m_newWorkspacePairBuffer; ///< Holds information for the name of a new,
  /// rebinned workspace vs an original workspace
  /// and source
  std::map<std::string, std::pair<std::string, pqPipelineSource *>>
      m_newRebinnedWorkspacePairBuffer; ///< Holds information for the name of a
  /// new, rebinned workspace vs an old
  /// rebinned workspace and source

  std::string m_tempPostfix;

  std::string m_tempPrefix;

  pqPipelineSource *m_inputSource;

  pqPipelineSource *m_rebinnedSource;

  std::vector<pqPipelineSource *>
  findAllRebinnedSourcesForWorkspace(const std::string &workspaceName);

  void swapSources(pqPipelineSource *source1, pqPipelineSource *source2);

  void rebuildPipeline(pqPipelineSource *source1, pqPipelineSource *source2);

  void processWorkspaceNames(std::string &inputWorkspace,
                             std::string &outputWorkspace,
                             pqPipelineSource *source,
                             std::string workspaceName,
                             const std::string &algorithmType);

  void untrackWorkspaces(const std::pair<std::string, std::string> &key);

  void copyProperties(pqPipelineFilter *filter1, pqPipelineFilter *filter2);

  static void copySafe(vtkSMProxy *dest, vtkSMProxy *source);

  void getWorkspaceInfo(pqPipelineSource *source, std::string &workspaceName,
                        std::string &workSpaceType);

  void removePipeline(pqPipelineSource *source);

  void deleteSpecificSource(pqPipelineSource *source);

  std::string getSourceName(pqPipelineSource *source);

  std::pair<std::string, std::string>
  createKeyPairForSource(pqPipelineSource *source);

  pqPipelineSource *goToPipelineBeginning(pqPipelineSource *source);

  bool
  doesSourceNeedToBeDeleted(const std::string &sourceName,
                            const std::vector<std::string> &trackedSources);
};

} // SimpleGui
} // Vates
} // Mantid

#endif
