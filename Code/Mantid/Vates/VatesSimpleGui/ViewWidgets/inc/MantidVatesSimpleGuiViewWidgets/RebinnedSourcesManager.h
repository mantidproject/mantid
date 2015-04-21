#ifndef REBINNEDSOURCESMANAGER_H_
#define REBINNEDSOURCESMANAGER_H_

#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "MantidQtAPI/WorkspaceObserver.h"
#include "MantidAPI/Workspace.h"

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
#include <map>
#include <string>

namespace Mantid
{
  namespace Vates
  {
    namespace SimpleGui
    {
      /**
       *
       This class  keeps track of the MDEvent workspaces and associated rebinned workspaces. Rebinning requires temporary workspaces instead of
       the original MDEvent workspaces. This class switches between these types of sources.

       @date 21/01/2015

       Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
      class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS RebinnedSourcesManager :public QWidget, MantidQt::API::WorkspaceObserver
      {
        Q_OBJECT
        public:
          RebinnedSourcesManager(QWidget* parent = 0);

          ~RebinnedSourcesManager();

          void checkSource(pqPipelineSource* source, std::string& inputWorkspace, std::string& outputWorkspace,  std::string algorithmType);

          void repipeRebinnedSource(std::string rebinnedSource, std::string& sourceToBeDeleted);

          void repipeOriginalSource(std::string rebinnedSource, std::string originalSource);

          void getStoredWorkspaceNames(pqPipelineSource* source, std::string& originalWorkspaceName, std::string& rebinnedWorkspaceName);

          bool isRebinnedSource(std::string name);

          bool doesWorkspaceBelongToRebinnedSource(std::string workspaceName);

          void registerRebinnedSource(pqPipelineSource* source);

        signals:
          void switchSources(std::string rebinnedWorkspaceName,  std::string sourceType);

          void triggerAcceptForNewFilters();
        protected:
          void addHandle(const std::string &workspaceName, const boost::shared_ptr<Mantid::API::Workspace> workspace);

          void preDeleteHandle(const std::string &wsName, const boost::shared_ptr<Mantid::API::Workspace> );

          void afterReplaceHandle(const std::string &workspaceName, const boost::shared_ptr<Mantid::API::Workspace> workspace);

        private slots:
          void onRebinnedSourceDestroyed();

        private:
          std::map<std::string, std::string> m_originalWorkspaceToRebinnedWorkspace; ///< Holds the mapping from the original source to the rebinned source

          std::map<std::string, std::string> m_rebinnedWorkspaceToOriginalWorkspace; ///< Holds the mapping from the rebinned source to the original source

          std::map<std::string, std::string> m_rebinnedWorkspaceToRebinnedWorkspace; ///< Holds information from a rebinned source to another temproary source which replaces it.

          std::string m_tempPostfix;

          std::string m_tempPrefix;

          pqPipelineSource* getSourceForWorkspace(std::string workspaceName);

          void swapSources(std::string source1, std::string source2);

          void rebuildPipeline(pqPipelineSource* source1, pqPipelineSource* source2);

          void processWorkspaceNames(std::string& inputWorkspace, std::string& outputWorkspace, std::string workspaceName, std::string algorithmType);

          void untrackWorkspaces(std::string rebinnedSource);

          void copyProperties(pqPipelineFilter* filter1, pqPipelineFilter* filter2);

          static void copySafe(vtkSMProxy* dest, vtkSMProxy* source);

          void getWorkspaceInfo(pqPipelineSource* source, std::string& workspaceName, std::string& workSpaceType);

          void removePipeline(pqPipelineSource* source);
      };

    } // SimpleGui
  } // Vates
} // Mantid

#endif 
