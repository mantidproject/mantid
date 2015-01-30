#include "MantidVatesSimpleGuiViewWidgets/SourcesManager.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidQtAPI/WorkspaceObserver.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidKernel/Logger.h"

#include "boost/shared_ptr.hpp"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqObjectBuilder.h>
#include <pqPipelineFilter.h>
#include <pqPipelineSource.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>
#include <vtkSMPropertyHelper.h>
#include <pqServerManagerModel.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMProxy.h>
#include <vtkSMPropertyIterator.h>
#include <vtkSMDoubleVectorProperty.h>
#include <QList>

#include <Poco/ActiveResult.h>

// Have to deal with ParaView warnings and Intel compiler the hard way.
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1170
#endif

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
        if (m_histoWorkspaceToEventWorkspace.count(workspaceName) > 0)
        {
          emit switchSourcesFromEventToHisto(workspaceName, m_histoWorkspaceToEventWorkspace[workspaceName]);
        }
      }

      /**
       * Check if the sources are valid.
       * @param source The pipeline source.
       * @param inputWorkspace Reference for the name of the input workspace.
       * @param outputWorkspace Reference for the name of the output workspace.
       */
      void SourcesManager::checkSource(pqPipelineSource* source, std::string& inputWorkspace, std::string& outputWorkspace)
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
        std::string workspaceName(vtkSMPropertyHelper(source->getProxy(),
                                                      "WorkspaceName", true).GetAsString());

        QString workspaceType(vtkSMPropertyHelper(source->getProxy(),
                                                  "WorkspaceTypeName", true).GetAsString());

        bool isHistoWorkspace = workspaceType.contains("MDHistoWorkspace");
        bool isEventWorkspace = workspaceType.contains("MDEventWorkspace");

        // Check if it is a Histo or Event workspace, if it is neither, then don't do anything
        if (isHistoWorkspace)
        {
          processMDHistoWorkspace(inputWorkspace, outputWorkspace, workspaceName);
        }
        else if (isEventWorkspace)
        {
          processMDEventWorkspace(inputWorkspace, outputWorkspace, workspaceName);
        }
      }

      /**
       * Creates the pipeline for the temporary source.
       * @param temporarySource The name of the temporary source.
       * @param originalSource The name of the original source.
       */
      void SourcesManager::repipeTemporarySource(std::string temporarySource, std::string originalSource)
      {
        // Swap source from original source to temporary source
        swapSources(originalSource, temporarySource);
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

        // Check if the original source has a filter if such then repipe otherwise we are done
        if ((src1->getAllConsumers()).size() <= 0)
        {
          return;
        }

        // Cast to the filter and reset the connection
        pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(src1->getConsumer(0));

        if (!filter)
        {
          throw std::runtime_error("VSI error: There is no filter in the pipeline.");
        }

        // We can rebuild the pipeline by either creating it from scratch or by changing the lowest level source
#if 1
        // Rebuild pipeline
        rebuildPipeline(src1, src2);
#else
        vtkSMPropertyHelper(filter->getProxy(), "Input").Set(src2->getProxy());

        // Update the pipeline from the end
        updateRebuiltPipeline(filter);

        // Set the visibility of the source. Paraview doesn't automatically set it to false, so we need to force it.
        setSourceVisibility(src2, false);
#endif
        // Render the active view to make the changes visible.
        pqActiveObjects::instance().activeView()->render();
      }

      /**
       * Removes the temporary source and reverts to the original source
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

        // Get to the underlying source
        std::string originalSource;
        std::string temporarySource; // not really used here
        checkSource(source, originalSource, temporarySource);

        // Make sure that the sources exist
        pqPipelineSource* tempsrc = getSourceForWorkspace(temporarySource);

        if (!tempsrc)
        {
          return;
        }

        // The input source can be either an MDEvent source or an MDHisto source.
        if (m_histoWorkspaceToEventWorkspace.count(originalSource) > 0)
        {
          originalWorkspaceName = m_histoWorkspaceToEventWorkspace[originalSource];
          temporaryWorkspaceName = originalSource;
        } else if (m_eventWorkspaceToHistoWorkspace.count(originalSource) > 0)
        {

          originalWorkspaceName = originalSource;
          temporaryWorkspaceName = m_eventWorkspaceToHistoWorkspace[originalSource];
        }
      }

      /**
       * Set the visibility of the underlying source
       * @param source The source which is to be manipulated.
       * @param visible The state of the source's visibility.
       */
      void SourcesManager::setSourceVisibility(pqPipelineSource* source, bool visible)
      {
        pqRepresentation* representation = qobject_cast<pqRepresentation* >(source->getRepresentation(pqActiveObjects::instance().activeView()));

        if(!representation)
        {
          throw std::runtime_error("VSI error: Casting to pqRepresentation failed.");
        }

        representation->setVisible(visible);
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
       * If sources which are derived of temporary MDHisto workspaces, the input workpspace is the 
       * original MDEvent workspace and the output is the temporary MDHisto workspace.
       * @param inputWorkspace Reference to the input workpspace.
       * @param outputWorkspace Reference to the output workspace.
       * @param workspaceName The name of the workspace of the current source.
       */
      void SourcesManager::processMDHistoWorkspace(std::string& inputWorkspace, std::string& outputWorkspace, std::string workspaceName)
      {
        if (m_histoWorkspaceToEventWorkspace.count(workspaceName) > 0)
        {
          inputWorkspace = m_histoWorkspaceToEventWorkspace[workspaceName];
          outputWorkspace = workspaceName;
        }
      }

      /**
       * If sources which are derived of temporary MDHisto workspaces, the input workpspace is the 
       * original MDEvent workspace and the output is the temporary MDHisto workspace.
       * @param inputWorkspace Reference to the input workpspace.
       * @param outputWorkspace Reference to the output workspace.
       * @param workspaceName The name of the workspace of the current source.
       */
      void SourcesManager::processMDEventWorkspace(std::string& inputWorkspace, std::string& outputWorkspace, std::string workspaceName)
      {
        inputWorkspace = workspaceName;
        outputWorkspace = workspaceName + m_tempPostfix;

        // Record the workspace
        m_eventWorkspaceToHistoWorkspace.insert(std::pair<std::string, std::string>(inputWorkspace, outputWorkspace));
        m_histoWorkspaceToEventWorkspace.insert(std::pair<std::string, std::string>(outputWorkspace, inputWorkspace));
      }

      /**
       * Stop keeping tabs on the specific workspace pair
       * @param temporaryWorspace The name of the temporary workspace.
       */
      void SourcesManager::untrackWorkspaces(std::string temporaryWorkspace)
      {
        std::string originalWorkspace = m_histoWorkspaceToEventWorkspace[temporaryWorkspace];

        m_histoWorkspaceToEventWorkspace.erase(temporaryWorkspace);
        m_eventWorkspaceToHistoWorkspace.erase(originalWorkspace);
      }

      /**
       * Removes the temporary workspace from memory.
       * @param temporaryWorkspace The name of the temporary workspace.
       */
      void SourcesManager::removeTemporaryWorkspace(std::string temporaryWorkspace)
      {
        Mantid::VATES::ADSWorkspaceProvider<Mantid::API::IMDHistoWorkspace> adsWorkspaceProvider;

        if (adsWorkspaceProvider.canProvideWorkspace(temporaryWorkspace))
        {
          adsWorkspaceProvider.disposeWorkspace(temporaryWorkspace);
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
        * Update the newly created pipeline from the last filter onwards
        * @param filter The filter after the source
        */
        void SourcesManager::updateRebuiltPipeline(pqPipelineFilter* filter)
        {
          // Crawl down the pipeline to the last filter
          while (filter->getNumberOfConsumers() > 0)
          {
            filter = qobject_cast<pqPipelineFilter*>(filter->getConsumer(0));

            if (!filter)
            {
              throw std::runtime_error("VSI error: There is no filter in the pipeline.");
            }

            filter->updatePipeline();
            filter->updateHelperProxies();

            vtkSMProxy* proxy = filter->getProxy();
            proxy->UpdateVTKObjects();
            proxy->UpdatePropertyInformation();
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
          pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
          pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(source1->getConsumer(0));

          vtkSMProxy* proxy = NULL;
          pqPipelineSource* newPipelineElement = NULL;
          pqPipelineFilter* newFilter = NULL;

          while(filter)
          {
            proxy = filter->getProxy();

            if (QString(proxy->GetXMLName()).contains("ScaleWorkspace"))
            {
              // Build the source
              newPipelineElement = builder->createFilter("filters","MantidParaViewScaleWorkspace", source2);
            } else if (QString(proxy->GetXMLName()).contains("Cut"))
            {
              newPipelineElement = builder->createFilter("filters", "Cut", source2);
            }

            newFilter = qobject_cast<pqPipelineFilter*>(newPipelineElement);

            // Bad proxies in ParaView have caused issues for Cut filters. If this 
            // happens, we remove the filter.
            copyProperties(filter, newFilter);

            emit triggerAcceptForNewFilters();

            if (filter->getNumberOfConsumers() > 0)
            {
              filter = qobject_cast<pqPipelineFilter*>(filter->getConsumer(0));
            }
            else 
            {
              filter = NULL;
            }
          }
        }

        /**
         * Copy the properties of the old filter to the new filter.
         * @param filter1 The old filter.
         * @param filter2 The new filter.
         */
        void SourcesManager::copyProperties(pqPipelineFilter* filter1, pqPipelineFilter* filter2)
        {
          vtkSMPropertyIterator *it = filter1->getProxy()->NewPropertyIterator();

          while (!it->IsAtEnd())
          {
            std::string key(it->GetKey());
            vtkSMProperty* propertyFilter1 = it->GetProperty();
            if (key != "Input")
            {
              filter2->getProxy()->GetProperty(key.c_str())->Copy(propertyFilter1);
            }

            it->Next();
          }
        }
    }
  }
}
