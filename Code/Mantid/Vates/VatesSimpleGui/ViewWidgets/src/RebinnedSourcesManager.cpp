#include "MantidVatesSimpleGuiViewWidgets/RebinnedSourcesManager.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidQtAPI/WorkspaceObserver.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Workspace.h"
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


#if defined(__INTEL_COMPILER)
  #pragma warning enable 1170
#endif
#include <QList>
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
      Mantid::Kernel::Logger g_log("RebinnedSourcesManager");
    }

      RebinnedSourcesManager::RebinnedSourcesManager(QWidget* parent) : QWidget(parent), m_tempPostfix("_tempvsi"), m_tempPrefix("")
      {
        observeAdd();
        observeAfterReplace();
        observePreDelete();
      }

      RebinnedSourcesManager::~RebinnedSourcesManager()
      {
      }

      /**
       * Checks if a rebinned MDHisto workspace was added and invokes a replacement procedure
       * @param workspaceName Name of the workspace.
       * @param workspace A pointer to the added workspace.
       */
      void RebinnedSourcesManager::addHandle(const std::string &workspaceName, Mantid::API::Workspace_sptr workspace)
      {
        if (m_rebinnedWorkspaceToOriginalWorkspace.count(workspaceName) > 0 || m_rebinnedWorkspaceToRebinnedWorkspace.count(workspaceName) > 0)
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
       * Catch the deletion of either the rebinned or the original workspace.
       * @param wsName The name of the workspace.
       */
      void RebinnedSourcesManager::preDeleteHandle(const std::string &wsName, const boost::shared_ptr<Mantid::API::Workspace>)
      {
        // If the original workspace has been deleted, then delete the rebinned
        // source (and workspace via the listener)
        if (m_originalWorkspaceToRebinnedWorkspace.count(wsName))
        {
          // Get the rebinned source and destroy the entire pipeline
          pqPipelineSource* source = getSourceForWorkspace(m_originalWorkspaceToRebinnedWorkspace[wsName]);

          // Go to the end of the pipeline
          while(source->getNumberOfConsumers() > 0)
          {
            source = source->getConsumer(0);
          }

          //Destroy the pipeline from the end
          pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
          pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(source);

          while (filter)
          {
            source = filter->getInput(0);
            builder->destroy(filter);
            filter = qobject_cast<pqPipelineFilter*>(source);
          }

          builder->destroy(source); // The listener takes now care of the workspace.
          untrackWorkspaces(m_originalWorkspaceToRebinnedWorkspace[wsName]);
        }
      }

      /**
       * Catch a change of a workspace
       * @param workspaceName Name of the workspace.
       * @param workspace A pointer to the added workspace.
       */
      void RebinnedSourcesManager::afterReplaceHandle(const std::string &workspaceName, const boost::shared_ptr<Mantid::API::Workspace> workspace)
      {
        addHandle(workspaceName, workspace);
      }

      /**
       * Check if the sources are valid.
       * @param source The pipeline source.
       * @param inputWorkspace Reference for the name of the input workspace.
       * @param outputWorkspace Reference for the name of the output workspace.
       * @param algorithmType The type of the algorithm which will be used to create the rebinned source.
       */
      void RebinnedSourcesManager::checkSource(pqPipelineSource* source, std::string& inputWorkspace, std::string& outputWorkspace, std::string algorithmType)
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
       * @param source The pipeline source.
       * @param workspaceName Reference to workspace name.
       * @param workspaceType Reference to workspace type.
       */
      void RebinnedSourcesManager::getWorkspaceInfo(pqPipelineSource* source, std::string& workspaceName, std::string& workspaceType)
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
       * Creates the pipeline for the rebinned source.
       * @param rebinnedSource The name of the rebinned source.
       * @param sourceToBeDeleted The name of the sources which needs to be removed from the pipeline browser.
       */
      void RebinnedSourcesManager::repipeRebinnedSource(std::string rebinnedSource, std::string& sourceToBeDeleted)
      {
        // We need to check if the source from which we receive our filters is the original source or 
        // a rebinned source.
        if (m_rebinnedWorkspaceToRebinnedWorkspace.count(rebinnedSource) == 0)
        {
          std::string originalSource = m_rebinnedWorkspaceToOriginalWorkspace[rebinnedSource];

          // Swap with the original source
          swapSources(originalSource, rebinnedSource);

          sourceToBeDeleted = originalSource;
        }
        else
        {
          std::string oldRebinnedSource = m_rebinnedWorkspaceToRebinnedWorkspace[rebinnedSource];
          std::string originalSource = m_rebinnedWorkspaceToOriginalWorkspace[oldRebinnedSource];

          // Swap with the other rebinned source
          swapSources(oldRebinnedSource, rebinnedSource);
          
          sourceToBeDeleted = oldRebinnedSource;

          m_originalWorkspaceToRebinnedWorkspace.insert(std::pair<std::string, std::string>(originalSource, rebinnedSource));
          m_rebinnedWorkspaceToOriginalWorkspace.insert(std::pair<std::string, std::string>(rebinnedSource, originalSource));

          // Unregister the connection between the two rebinned sources.
          m_rebinnedWorkspaceToRebinnedWorkspace.erase(rebinnedSource);
        }
      }

      /**
       * Creates the pipeline for the original source.
       * @param rebinnedSource The name of the rebinned source.
       * @param originalSource The name of the original source.
       */
      void RebinnedSourcesManager::repipeOriginalSource(std::string rebinnedSource, std::string originalSource)
      {
        // Swap source from rebinned source to original source.
        swapSources(rebinnedSource, originalSource);

        m_originalWorkspaceToRebinnedWorkspace.erase(originalSource);
        m_rebinnedWorkspaceToOriginalWorkspace.erase(rebinnedSource);
      }

      /**
       * Swap the sources at the bottom level of the pipeline.
       * @param source1 First source.
       * @param source2 Second source.
       */
      void RebinnedSourcesManager::swapSources(std::string source1, std::string source2)
      {
        pqPipelineSource* src1= getSourceForWorkspace(source1);
        pqPipelineSource* src2 = getSourceForWorkspace(source2);

        if (!src1 || !src2)
        {
          throw std::runtime_error("VSI error: Either the original or rebinned source don't seem to exist.");
        }

        // Check that both sources contain non-empty data sets

        // Check if the original source has a filter if such then repipe otherwise we are done
        if ((src1->getAllConsumers()).size() <= 0)
        {
          // Need to press apply to finalize the internal setup of the source.
          //emit triggerAcceptForNewFilters();
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
       * @param rebinnedWorkspaceName The name of the rebinned workspace.
       */
      void RebinnedSourcesManager::getStoredWorkspaceNames(pqPipelineSource* source, std::string& originalWorkspaceName, std::string& rebinnedWorkspaceName)
      {
        if (!source)
        {
          return;
        }

        // Get the underlying workspace name and type
        std::string workspaceName;
        std::string workspaceType;
        getWorkspaceInfo(source, workspaceName, workspaceType);

        // The input can either be a rebinned source or a 
        if (m_rebinnedWorkspaceToOriginalWorkspace.count(workspaceName) > 0)
        {
          originalWorkspaceName = m_rebinnedWorkspaceToOriginalWorkspace[workspaceName];
          rebinnedWorkspaceName = workspaceName;
        } else if (m_originalWorkspaceToRebinnedWorkspace.count(workspaceName) > 0)
        {
          originalWorkspaceName = workspaceName;
          rebinnedWorkspaceName = m_originalWorkspaceToRebinnedWorkspace[workspaceName];
        }
      }

      /**
       * Get the desired source
       * @param workspaceName The workspace name associated with the source.
       */
      pqPipelineSource* RebinnedSourcesManager::getSourceForWorkspace(std::string workspaceName)
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
       * @param algorithmType The algorithm which creates the rebinned source.
       */
      void RebinnedSourcesManager::processWorkspaceNames(std::string& inputWorkspace, std::string& outputWorkspace, std::string workspaceName, std::string algorithmType)
      {
        // If the workspace is the original workspace
        if (workspaceName.find(m_tempPostfix) == std::string::npos)
        {
          inputWorkspace = workspaceName;
          outputWorkspace =  m_tempPrefix + workspaceName + algorithmType + m_tempPostfix;

          // Record the workspace
          m_originalWorkspaceToRebinnedWorkspace.insert(std::pair<std::string, std::string>(inputWorkspace, outputWorkspace));
          m_rebinnedWorkspaceToOriginalWorkspace.insert(std::pair<std::string, std::string>(outputWorkspace, inputWorkspace));
        } // If the workspace is rebinned and was created with the same algorithm as the currently selected one.
        else if (workspaceName.find(algorithmType) != std::string::npos) 
        {
          if (m_rebinnedWorkspaceToOriginalWorkspace.count(workspaceName) > 0)
          {
            inputWorkspace = m_rebinnedWorkspaceToOriginalWorkspace[workspaceName];
            outputWorkspace = workspaceName;
          }
        }
        else // If the workspace is rebinned but was not created with the same algorithm as the currently selected one.
        {
          if (m_rebinnedWorkspaceToOriginalWorkspace.count(workspaceName) > 0)
          {
            inputWorkspace = m_rebinnedWorkspaceToOriginalWorkspace[workspaceName];
            outputWorkspace = m_tempPrefix + inputWorkspace + algorithmType + m_tempPostfix;

            // Map the new rebinned workspace name to the old rebinned workspace name
            m_rebinnedWorkspaceToRebinnedWorkspace.insert(std::pair<std::string, std::string>(outputWorkspace, workspaceName));
          }
        }
      }

      /**
       * Stop keeping tabs on the specific workspace pair
       * @param rebinnedWorkspace The name of the rebinned workspace.
       */
      void RebinnedSourcesManager::untrackWorkspaces(std::string rebinnedWorkspace)
      {
        std::string originalWorkspace = m_rebinnedWorkspaceToOriginalWorkspace[rebinnedWorkspace];

        // Remove the mapping ofthe rebinned workspace to the original workspace.
        if (m_rebinnedWorkspaceToOriginalWorkspace.count(rebinnedWorkspace) > 0)
        {
          m_rebinnedWorkspaceToOriginalWorkspace.erase(rebinnedWorkspace);
        }

        // Remove the mapping of the original workspace to the rebinned workspace, if the mapping is still intact.
        if (m_originalWorkspaceToRebinnedWorkspace.count(originalWorkspace) > 0 && m_originalWorkspaceToRebinnedWorkspace[originalWorkspace] == rebinnedWorkspace)
        {
          m_originalWorkspaceToRebinnedWorkspace.erase(originalWorkspace);
        }
      }

      /**
       * Register the rebinned source. Specifically, connect to the destroyed signal of the rebinned source.
       * @param source The rebinned source.
       */
      void RebinnedSourcesManager::registerRebinnedSource(pqPipelineSource* source)
      {
        if (!source)
        {
          return;
        }

        QObject::connect(source, SIGNAL(destroyed()),
                         this, SLOT(onRebinnedSourceDestroyed()));
      }

      /**
       * React to the deletion of a rebinned source.
       */
      void RebinnedSourcesManager::onRebinnedSourceDestroyed()
      {
        removeUnusedRebinnedWorkspaces();
      }

      /**
       * Remove unused rebinned workspaces, by comparing the workspaces against the sources.
       */
      void RebinnedSourcesManager::removeUnusedRebinnedWorkspaces()
      {
        // Iterate through all workspaces and check for ones ending with the tempIdentifier
        std::set<std::string> workspaceNames = Mantid::API::AnalysisDataService::Instance().getObjectNamesInclHidden();
  
        for (std::set<std::string>::iterator it = workspaceNames.begin(); it != workspaceNames.end(); ++it)
        {
          // Only look at the rebinned files
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
       void RebinnedSourcesManager::compareToSources(std::string workspaceName)
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

              // If the rebinned workspace has a source equivalent, then exit
              if (name==workspaceName)
              {
                return;
              }
            }
          }

          // There is no source which corresponds to the workspace, hence delete and unregister the workspace.
          removeRebinnedWorkspace(workspaceName);
          untrackWorkspaces(workspaceName);
       }

      /**
        * Removes the rebinned workspace from memory.
        * @param rebinnedWorkspace The name of the rebinned workspace.
        */
      void RebinnedSourcesManager::removeRebinnedWorkspace(std::string rebinnedWorkspace)
      {
        Mantid::VATES::ADSWorkspaceProvider<Mantid::API::IMDHistoWorkspace> adsHistoWorkspaceProvider;
        Mantid::VATES::ADSWorkspaceProvider<Mantid::API::IMDEventWorkspace> adsEventWorkspaceProvider;

        if (adsHistoWorkspaceProvider.canProvideWorkspace(rebinnedWorkspace))
        {
          adsHistoWorkspaceProvider.disposeWorkspace(rebinnedWorkspace);
        }
        else if (adsEventWorkspaceProvider.canProvideWorkspace(rebinnedWorkspace))
        {
          adsEventWorkspaceProvider.disposeWorkspace(rebinnedWorkspace);
        }
      }

      /**
        * Rebuild the pipeline for the new source
        * @param source1 The old source.
        * @param source2 The new source.
        */
      void RebinnedSourcesManager::rebuildPipeline(pqPipelineSource* source1, pqPipelineSource* source2)
      {
        // Step through all the filters in old pipeline and reproduce them
        pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
        pqPipelineFilter* filter1 = qobject_cast<pqPipelineFilter*>(source1->getConsumer(0));

        pqPipelineSource* endOfSource2Pipeline = source2;

        while(filter1)
        {
          vtkSMProxy* proxy1 = NULL;
          proxy1 = filter1->getProxy();
          pqPipelineSource* newPipelineElement = NULL;
          pqPipelineFilter* newFilter = NULL;
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
      void RebinnedSourcesManager::copyProperties(pqPipelineFilter* filter1, pqPipelineFilter* filter2)
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
      void RebinnedSourcesManager::copySafe(vtkSMProxy* dest, vtkSMProxy* source)
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

      /**
       * Check if we have a rebinned source
       * @param name The source name.
       */
      bool RebinnedSourcesManager::isRebinnedSource(std::string name)
      {
        if (m_rebinnedWorkspaceToOriginalWorkspace.count(name) > 0)
        {
          return true;
        }
        else
        {
          return false;
        }
      }

      /**
       * Check if rebinned workspace has already been loaded
       * @param workspaceName The name of the rebinned workspace
       */
      bool RebinnedSourcesManager::doesWorkspaceBelongToRebinnedSource(std::string workspaceName)
      {
        pqPipelineSource *source = getSourceForWorkspace(workspaceName);

        if (!source)
        {
          return false;
        }

        if (isRebinnedSource(workspaceName))
        {
          return true;
        }
        else
        {
          return false;
        }
      }
    }
  }
}
