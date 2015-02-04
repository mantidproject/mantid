#ifndef SOURCESMANAGER_H_
#define SOURCESMANAGER_H_

#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "MantidQtAPI/WorkspaceObserver.h"

#include <pqPipelineSource.h>
#include <pqPipelineFilter.h>
#include <vtkSMProxy.h>

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
       This class  keeps track of the MDEvent workspaces and associated temporary MDHisto workspaces. Rebinning requires temporary MDHisto workspaces instead of
       the MDEvent workspaces. This class switches between these types of sources.

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
      class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS SourcesManager :public QWidget, MantidQt::API::WorkspaceObserver
      {
        Q_OBJECT
        public:
          SourcesManager(QWidget* parent = 0);

          ~SourcesManager();

          void checkSource(pqPipelineSource* source, std::string& inputWorkspace, std::string& outputWorkspace);

          void repipeTemporarySource(std::string temporarySource, std::string originalSource);

          void repipeOriginalSource(std::string temporarySource, std::string originalSource);

          void getStoredWorkspaceNames(pqPipelineSource* source, std::string& originalWorkspaceName, std::string& temporaryWorkspaceName);

          void registerTemporarySource(pqPipelineSource* source);

        signals:
          void switchSourcesFromEventToHisto(std::string histoWorkspaceName, std::string eventWorkspaceName);

          void triggerAcceptForNewFilters();
        protected:
          void addHandle(const std::string &workspaceName, const boost::shared_ptr<Mantid::API::Workspace> workspace);

        private slots:
          void onTemporarySourceDestroyed();

          void setSourceVisibility(pqPipelineSource* source, bool visibile);

        private:
          std::map<std::string, std::string> m_eventWorkspaceToHistoWorkspace; ///< Holds the mapping from the Event workspace name to the temporary Histo workspace name

          std::map<std::string, std::string> m_histoWorkspaceToEventWorkspace; ///< Holds the mapping from temporary  Histo workspace name to the Event workspace name

          std::string m_tempPostfix;

          pqPipelineSource* getSourceForWorkspace(std::string workspaceName);

          void swapSources(std::string source1, std::string source2);

          void rebuildPipeline(pqPipelineSource* source1, pqPipelineSource* source2);

          void processMDHistoWorkspace(std::string& inputWorkspace, std::string& outputWorkspace, std::string workspaceName);

          void processMDEventWorkspace(std::string& inputWorkspace, std::string& outputWorkspace, std::string workspaceName);

          void removeUnusedTemporaryWorkspaces();

          void untrackWorkspaces(std::string temporarySource);

          void removeTemporaryWorkspace(std::string temporaryWorkspace);

          void compareToSources(std::string workspaceName);

          void updateRebuiltPipeline(pqPipelineFilter* filter);

          void copyProperties(pqPipelineFilter* filter1, pqPipelineFilter* filter2);

          static void copySafe(vtkSMProxy* dest, vtkSMProxy* source);
      };

    } // SimpleGui
  } // Vates
} // Mantid

#endif 
