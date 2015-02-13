#include "MantidVatesSimpleGuiViewWidgets/SourcesManager.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidQtAPI/WorkspaceObserver.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/Logger.h"

// Have to deal with ParaView warnings and Intel compiler the hard way.
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1170
#endif
#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqObjectBuilder.h>
#include <pqPipelineFilter.h>
#include <pqPipelineSource.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>
#include <vtkSMPropertyHelper.h>
#include <pqServerManagerModel.h>
#include <pqUndoStack.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMProxy.h>
#include <vtkSMPropertyIterator.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMInputProperty.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMProxyListDomain.h>
#include <QList>

#if defined(__INTEL_COMPILER)
  #pragma warning enable 1170
#endif

#include "boost/shared_ptr.hpp"
#include <Poco/ActiveResult.h>



namespace Mantid
{
  namespace Vates
  {
    namespace SimpleGui
    {

    namespace
    {
      Mantid::Kernel::Logger g_log("SourcesManager");
    }

      SourcesManager::SourcesManager(QWidget* parent) : QWidget(parent), m_tempPostfix("_tempvsi")
      {
        observeAdd();
      }

      SourcesManager::~SourcesManager()
      {
      }

      /**
       * Checks if a temporary MDHisto workspace was added and invokes a replacement procedure
       * @param workspaceName Name of the workspace.
       * @param workspace A pointer to the added workspace.
       */
      void SourcesManager::addHandle(const std::string &workspaceName, Mantid::API::Workspace_sptr workspace)
      {
        if (m_temporaryWorkspaceToOriginalWorkspace.count(workspaceName) > 0 || m_temporaryWorkspaceToTemporaryWorkspace.count(workspaceName) > 0)
        {
          std::string sourceType;
          Mantid::API::IMDEventWorkspace_sptr eventWorkspace = boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(workspace);
          Mantid::API::IMDHistoWorkspace_sptr histoWorkspace = boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(workspace);

          if(eventWorkspace)
          {
            sourceType = "MDEW Source";
          }
          else if(histoWorkspace)
          {
            sourceType = "MDHW Source";
          }
          else
          {
            return;
          }

          emit switchSources(workspaceName, sourceType);
        }
      }

      /**
       * Check if the sources are valid.
       * @param source The pipeline source.
       * @param inputWorkspace Reference for the name of the input workspace.
       * @param outputWorkspace Reference for the name of the output workspace.
       * @param algorithmType The type of the algorithm which will be used to create the temporary source.
       */
      void SourcesManager::checkSource(pqPipelineSource* source, std::string& inputWorkspace, std::string& outputWorkspace, std::string algorithmType)
      {
        std::string workspaceName;
        std::string workspaceType;

        getWorkspaceInfo(source, workspaceName, workspaceType);

        bool isHistoWorkspace = workspaceType.find("MDHistoWorkspace")!=std::string::npos;
        bool isEventWorkspace = workspaceType.find("MDEventWorkspace")!=std::string::npos;

        // Check if it is a Histo or Event workspace, if it is neither, then don't do anything
        if (isHistoWorkspace || isEventWorkspace)
        {
          processWorkspaceNames(inputWorkspace, outputWorkspace, workspaceName, algorithmType);
        }
      }

      /**
       * Get workspace name and type
       * @param workspaceName Reference to workspace name.
       * @param workspaceType Reference to workspace type.
       */
      void SourcesManager::getWorkspaceInfo(pqPipelineSource* source, std::string& workspaceName, std::string& workspaceType)
      {
        // Make sure that the input source exists. Note that this can happen when there is no active view
        if (!source)
        {
          return;
        }

        // Update the source/filter
        vtkSMProxy* proxy = source->getProxy();
        proxy->UpdateVTKObjects();
        proxy->UpdatePropertyInformation();
        source->updatePipeline();

        // Crawl up to the source level 
        pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(source);

        while (filter)
        {
          source = filter->getInput(0);
          filter = qobject_cast<pqPipelineFilter*>(source);
        }

        proxy = source->getProxy();

        // Ensure that the source is either an MDEvent source or an MDHisto source
        if (!QString(proxy->GetXMLName()).contains("MDEW Source") &&
            !QString(proxy->GetXMLName()).contains("MDHW Source"))
        {
          return;
        }

        // Check if the source has an underlying event workspace or histo workspace
        workspaceName = vtkSMPropertyHelper(source->getProxy(), "WorkspaceName", true).GetAsString();

        workspaceType = vtkSMPropertyHelper(source->getProxy(), "WorkspaceTypeName", true).GetAsString();

      }

      /**
       * Creates the pipeline for the temporary source.
       * @param temporarySource The name of the temporary source.
       * @param sourceToBeDeleted The name of the sources which needs to be removed from the pipeline browser.
       */
      void SourcesManager::repipeTemporarySource(std::string temporarySource, std::string& sourceToBeDeleted)
      {
        // We need to check if the source from which we receive our filters is the original source or 
        // a temporary source.
        if (m_temporaryWorkspaceToTemporaryWorkspace.count(temporarySource) == 0)
        {
          std::string originalSource = m_temporaryWorkspaceToOriginalWorkspace[temporarySource];

          // Swap with the original source
          swapSources(originalSource, temporarySource);

          sourceToBeDeleted = originalSource;
        }
        else
        {
          std::string oldTemporarySource = m_temporaryWorkspaceToTemporaryWorkspace[temporarySource];
          std::string originalSource = m_temporaryWorkspaceToOriginalWorkspace[oldTemporarySource];

          // Swap with the other temporary source
          swapSources(oldTemporarySource, temporarySource);
          
          sourceToBeDeleted = oldTemporarySource;

          m_originalWorkspaceToTemporaryWorkspace.insert(std::pair<std::string, std::string>(originalSource, temporarySource));
          m_temporaryWorkspaceToOriginalWorkspace.insert(std::pair<std::string, std::string>(temporarySource, originalSource));

          // Unregister the connection between the two temporary sources.
          m_temporaryWorkspaceToTemporaryWorkspace.erase(temporarySource);
        }
      }

      /**
       * Creates the pipeline for the original source.
       * @param temporarySource The name of the temporary source.
       * @param originalSource The name of the original source.
       */
      void SourcesManager::repipeOriginalSource(std::string temporarySource, std::string originalSource)
      {
        // Swap source from temporary source to original source.
        swapSources(temporarySource, originalSource);

        m_originalWorkspaceToTemporaryWorkspace.erase(originalSource);
        m_temporaryWorkspaceToOriginalWorkspace.erase(temporarySource);
      }

      /**
       * Swap the sources at the bottom level of the pipeline.
       * @param source1 First source.
       * @param source2 Second source.
       */
      void SourcesManager::swapSources(std::string source1, std::string source2)
      {
        pqPipelineSource* src1= getSourceForWorkspace(source1);
        pqPipelineSource* src2 = getSourceForWorkspace(source2);

        if (!src1 || !src2)
        {
          throw std::runtime_error("VSI error: Either the original or temporary source don't seem to exist.");
        }

        // Check that both sources contain non-empty data sets

        // Check if the original source has a filter if such then repipe otherwise we are done
        if ((src1->getAllConsumers()).size() <= 0)
        {
          return;
        }

        // Rebuild pipeline
        rebuildPipeline(src1, src2);

        // Render the active view to make the changes visible.
        pqActiveObjects::instance().activeView()->render();
      }

      /**
       * Get the stored workspace names assoicated with a source.
       * @param source The name of the source.
       * @param originalWorkspaceName The name of the original workspace.
       * @param temporaryWorkspaceName The name of the temporary workspace.
       */
      void SourcesManager::getStoredWorkspaceNames(pqPipelineSource* source, std::string& originalWorkspaceName, std::string& temporaryWorkspaceName)
      {
        if (!source)
        {
          return;
        }

        // Get the underlying workspace name and type
        std::string workspaceName;
        std::string workspaceType;
        getWorkspaceInfo(source, workspaceName, workspaceType);

        // The input can either be a temporary source or a 
        if (m_temporaryWorkspaceToOriginalWorkspace.count(workspaceName) > 0)
        {
          originalWorkspaceName = m_temporaryWorkspaceToOriginalWorkspace[workspaceName];
          temporaryWorkspaceName = workspaceName;
        } else if (m_originalWorkspaceToTemporaryWorkspace.count(workspaceName) > 0)
        {
          originalWorkspaceName = workspaceName;
          temporaryWorkspaceName = m_originalWorkspaceToTemporaryWorkspace[workspaceName];
        }
      }

      /**
       * Get the desired source
       * @param workspaceName The workspace name associated with the source.
       */
      pqPipelineSource* SourcesManager::getSourceForWorkspace(std::string workspaceName)
      {
        pqServer *server = pqActiveObjects::instance().activeServer();
        pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
        QList<pqPipelineSource *> sources;
        QList<pqPipelineSource *>::Iterator source;
        sources = smModel->findItems<pqPipelineSource *>(server);

        for (source = sources.begin(); source != sources.end(); ++source)
        {

          pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(*source);

          if (!filter)
          {
            std::string wsName(vtkSMPropertyHelper((*source)->getProxy(),
                                                  "WorkspaceName", true).GetAsString());
            if (!wsName.empty())
            {
              if (wsName == workspaceName)
              {
                return (*source);
              }
            }
          }
        }
        return NULL;
      }

      /**
       * Process the workspaces names for the original source and the input source
       * @param inputWorkspace Reference to the input workpspace.
       * @param outputWorkspace Reference to the output workspace.
       * @param workspaceName The name of the workspace of the current source.
       * @param algorithmType The algorithm which creates the temporary source.
       */
      void SourcesManager::processWorkspaceNames(std::string& inputWorkspace, std::string& outputWorkspace, std::string workspaceName, std::string algorithmType)
      {
        // If the workspace is the original workspace
        if (workspaceName.find(m_tempPostfix) == std::string::npos)
        {
          inputWorkspace = workspaceName;
          outputWorkspace = workspaceName + algorithmType + m_tempPostfix;

          // Record the workspace
          m_originalWorkspaceToTemporaryWorkspace.insert(std::pair<std::string, std::string>(inputWorkspace, outputWorkspace));
          m_temporaryWorkspaceToOriginalWorkspace.insert(std::pair<std::string, std::string>(outputWorkspace, inputWorkspace));
        } // If the workspace is temporary and was created with the same algorithm as the currently selected one.
        else if (workspaceName.find(algorithmType) != std::string::npos) 
        {
          if (m_temporaryWorkspaceToOriginalWorkspace.count(workspaceName) > 0)
          {
            inputWorkspace = m_temporaryWorkspaceToOriginalWorkspace[workspaceName];
            outputWorkspace = workspaceName;
          }
        }
        else // If the workspace is temporary but was not created with the same algorithm as the currently selected one.
        {
          if (m_temporaryWorkspaceToOriginalWorkspace.count(workspaceName) > 0)
          {
            inputWorkspace = m_temporaryWorkspaceToOriginalWorkspace[workspaceName];
            outputWorkspace = inputWorkspace + algorithmType + m_tempPostfix;

            // Map the new temporary workspace name to the old temporary workspace name
            m_temporaryWorkspaceToTemporaryWorkspace.insert(std::pair<std::string, std::string>(outputWorkspace, workspaceName));
          }
        }
      }

      /**
       * Stop keeping tabs on the specific workspace pair
       * @param temporaryWorspace The name of the temporary workspace.
       */
      void SourcesManager::untrackWorkspaces(std::string temporaryWorkspace)
      {
        std::string originalWorkspace = m_temporaryWorkspaceToOriginalWorkspace[temporaryWorkspace];

        // Remove the mapping ofthe temporary workspace to the original workspace.
        if (m_temporaryWorkspaceToOriginalWorkspace.count(temporaryWorkspace) > 0)
        {
          m_temporaryWorkspaceToOriginalWorkspace.erase(temporaryWorkspace);
        }

        // Remove the mapping of the original workspace to the temporary workspace, if the mapping is still intact.
        if (m_originalWorkspaceToTemporaryWorkspace.count(originalWorkspace) > 0 && m_originalWorkspaceToTemporaryWorkspace[originalWorkspace] == temporaryWorkspace)
        {
          m_originalWorkspaceToTemporaryWorkspace.erase(originalWorkspace);
        }
      }

      /**
       * Register the temporary source. Specifically, connect to the destroyed signal of the temporary source.
       * @param source The temporary source.
       */
      void SourcesManager::registerTemporarySource(pqPipelineSource* source)
      {
        if (!source)
        {
          return;
        }

        QObject::connect(source, SIGNAL(destroyed()),
                         this, SLOT(onTemporarySourceDestroyed()));
      }

      /**
       * React to the deletion of a temporary source.
       */
      void SourcesManager::onTemporarySourceDestroyed()
      {
        removeUnusedTemporaryWorkspaces();
      }

      /**
       * Remove unused temporary workspaces, by comparing the workspaces against the sources.
       */
      void SourcesManager::removeUnusedTemporaryWorkspaces()
      {
        // Iterate through all workspaces and check for ones ending with the tempIdentifier
        std::set<std::string> workspaceNames = Mantid::API::AnalysisDataService::Instance().getObjectNames();

        for (std::set<std::string>::iterator it = workspaceNames.begin(); it != workspaceNames.end(); ++it)
        {
          // Only look at the temporary files
          if (it->find(m_tempPostfix) != std::string::npos)
          {
              compareToSources(*it);
          }
        }
      }

       /**
        * Compare if the workspace name exists among the sources. If it doesnt't exist, remove it.
        * @param workspaceName The name of the workspace
        */
       void SourcesManager::compareToSources(std::string workspaceName)
       {
          pqServer *server = pqActiveObjects::instance().activeServer();
          pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
          QList<pqPipelineSource *> sources = smModel->findItems<pqPipelineSource *>(server);

          for (QList<pqPipelineSource *>::Iterator source = sources.begin(); source != sources.end(); ++source)
          {
            const QString srcProxyName = (*source)->getProxy()->GetXMLGroup();

            if (srcProxyName == QString("sources"))
            {
              std::string name(vtkSMPropertyHelper((*source)->getProxy(),
                                                    "WorkspaceName", true).GetAsString());

              // If the temporary workspace has a source equivalent, then exit
              if (name==workspaceName)
              {
                return;
              }
            }
          }

          // There is no source which corresponds to the workspace, hence delete and unregister the workspace.
          removeTemporaryWorkspace(workspaceName);
          untrackWorkspaces(workspaceName);
       }

        /**
         * Removes the temporary workspace from memory.
         * @param temporaryWorkspace The name of the temporary workspace.
         */
        void SourcesManager::removeTemporaryWorkspace(std::string temporaryWorkspace)
        {
          Mantid::VATES::ADSWorkspaceProvider<Mantid::API::IMDHistoWorkspace> adsHistoWorkspaceProvider;
          Mantid::VATES::ADSWorkspaceProvider<Mantid::API::IMDEventWorkspace> adsEventWorkspaceProvider;

          if (adsHistoWorkspaceProvider.canProvideWorkspace(temporaryWorkspace))
          {
            adsHistoWorkspaceProvider.disposeWorkspace(temporaryWorkspace);
          }
          else if (adsEventWorkspaceProvider.canProvideWorkspace(temporaryWorkspace))
          {
            adsEventWorkspaceProvider.disposeWorkspace(temporaryWorkspace);
          }
        }

        /**
         * Rebuild the pipeline for the new source
         * @param source1 The old source.
         * @param source2 The new source.
         */
        void SourcesManager::rebuildPipeline(pqPipelineSource* source1, pqPipelineSource* source2)
        {
          // Step through all the filters in old pipeline and reproduce them
          pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
          pqPipelineFilter* filter1 = qobject_cast<pqPipelineFilter*>(source1->getConsumer(0));

          vtkSMProxy* proxy1 = NULL;
          pqPipelineSource* newPipelineElement = NULL;
          pqPipelineFilter* newFilter = NULL;

          pqPipelineSource* endOfSource2Pipeline = source2;

          while(filter1)
          {
            proxy1 = filter1->getProxy();

            // Move source2 to its end.
            while (endOfSource2Pipeline->getNumberOfConsumers() > 0)
            {
             endOfSource2Pipeline = endOfSource2Pipeline->getConsumer(0);
            }

            if (QString(proxy1->GetXMLName()).contains("ScaleWorkspace"))
            {
              // Build the source
              newPipelineElement = builder->createFilter("filters","MantidParaViewScaleWorkspace", endOfSource2Pipeline);
            } else if (QString(proxy1->GetXMLName()).contains("Cut"))
            {
              newPipelineElement = builder->createFilter("filters", "Cut", endOfSource2Pipeline);
            }

            newFilter = qobject_cast<pqPipelineFilter*>(newPipelineElement);

            // Copy the properties from the old filter to the new filter.
            copyProperties(filter1, newFilter);

            if (filter1->getNumberOfConsumers() > 0)
            {
              filter1 = qobject_cast<pqPipelineFilter*>(filter1->getConsumer(0));
            }
            else 
            {
              filter1 = NULL;
            }
          }
          emit triggerAcceptForNewFilters();
        }

        /**
         * Copy the properties of the old filter to the new filter.
         * @param filter1 The old filter.
         * @param filter2 The new filter.
         */
        void SourcesManager::copyProperties(pqPipelineFilter* filter1, pqPipelineFilter* filter2)
        {
          vtkSMProxy* proxy1 = filter1->getProxy();
          vtkSMProxy* proxy2 = filter2->getProxy();

          copySafe(proxy2, proxy1);
        }

        /**
         * This method is taken from a newer version of pqCopyReaction, which contains a bug fix 
         * for copying CutFilter properties. This is the correct way to copy proxy properties.
         * @param dest Destination proxy.
         * @param source Source proxy.
         */
        void SourcesManager::copySafe(vtkSMProxy* dest, vtkSMProxy* source)
        {
          if (dest && source)
          {
          BEGIN_UNDO_SET("Copy Properties");
          dest->Copy(source, "vtkSMProxyProperty");

            // handle proxy properties.
            vtkSMPropertyIterator* destIter = dest->NewPropertyIterator();
            for (destIter->Begin(); !destIter->IsAtEnd(); destIter->Next())
            {
              if (vtkSMInputProperty::SafeDownCast(destIter->GetProperty()))
              {
              // skip input properties.
              continue;
              }

              vtkSMProxyProperty* destPP =  vtkSMProxyProperty::SafeDownCast(destIter->GetProperty());
              vtkSMProxyProperty* srcPP = vtkSMProxyProperty::SafeDownCast(source->GetProperty(destIter->GetKey()));

              if (!destPP || !srcPP || srcPP->GetNumberOfProxies() > 1)
              {
              // skip non-proxy properties since those were already copied.
              continue;
              }

              vtkSMProxyListDomain* destPLD = vtkSMProxyListDomain::SafeDownCast(destPP->FindDomain("vtkSMProxyListDomain"));
              vtkSMProxyListDomain* srcPLD = vtkSMProxyListDomain::SafeDownCast(srcPP->FindDomain("vtkSMProxyListDomain"));

              if (!destPLD || !srcPLD)
              {
                // we copy proxy properties that have proxy list domains.
                continue;
              }

              if (srcPP->GetNumberOfProxies() == 0)
              {
                destPP->SetNumberOfProxies(0);
                continue;
              }

              vtkSMProxy* srcValue = srcPP->GetProxy(0);
              vtkSMProxy* destValue = NULL;

              // find srcValue type in destPLD and that's the proxy to use as destValue.
              for (unsigned int cc=0; srcValue != NULL && cc < destPLD->GetNumberOfProxyTypes(); cc++)
              {
                if (srcValue->GetXMLName() && destPLD->GetProxyName(cc) &&
                    strcmp(srcValue->GetXMLName(), destPLD->GetProxyName(cc)) == 0 &&
                    srcValue->GetXMLGroup() && destPLD->GetProxyGroup(cc) &&
                    strcmp(srcValue->GetXMLGroup(), destPLD->GetProxyGroup(cc)) == 0)
                {
                  destValue = destPLD->GetProxy(cc);
                  break;
                }
              }

              if (destValue)
              {
                Q_ASSERT(srcValue != NULL);
                copySafe(destValue, srcValue);
                destPP->SetProxy(0, destValue);
              }
            }

            destIter->Delete();
            dest->UpdateVTKObjects();
            END_UNDO_SET();
          }
        }
    }
  }
}
