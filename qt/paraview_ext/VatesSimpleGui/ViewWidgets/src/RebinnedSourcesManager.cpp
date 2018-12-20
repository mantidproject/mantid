#include "MantidVatesSimpleGuiViewWidgets/RebinnedSourcesManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/TSVSerialiser.h"
#include "MantidQtWidgets/Common/WorkspaceObserver.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"

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
#include <pqUndoStack.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMInputProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMPropertyIterator.h>
#include <vtkSMProxy.h>
#include <vtkSMProxyListDomain.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMSourceProxy.h>

#if defined(__INTEL_COMPILER)
#pragma warning enable 1170
#endif
#include "boost/shared_ptr.hpp"
#include <Poco/ActiveResult.h>
#include <QList>
#include <utility>

namespace Mantid {
namespace Vates {
namespace SimpleGui {

namespace {
Mantid::Kernel::Logger g_log("RebinnedSourcesManager");
}

RebinnedSourcesManager::RebinnedSourcesManager(QWidget *parent)
    : QWidget(parent), m_tempPostfix("_rebinned_vsi"), m_tempPrefix(""),
      m_inputSource(nullptr), m_rebinnedSource(nullptr) {
  observeAdd();
  observeAfterReplace();
  observePreDelete();
}

RebinnedSourcesManager::~RebinnedSourcesManager() {}

/**
 * Checks if a rebinned MDHisto workspace was added and invokes a replacement
 * procedure
 * @param workspaceName Name of the workspace.
 * @param workspace A pointer to the added workspace.
 */
void RebinnedSourcesManager::addHandle(const std::string &workspaceName,
                                       Mantid::API::Workspace_sptr workspace) {
  // Check if the workspace which has experienced a change is being tracked in
  // our buffer
  if (m_newWorkspacePairBuffer.empty()) {
    return;
  }

  std::string outputWorkspace = m_newWorkspacePairBuffer.begin()->second.first;

  if (outputWorkspace == workspaceName) {
    std::string sourceType;
    Mantid::API::IMDEventWorkspace_sptr eventWorkspace =
        boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(workspace);
    Mantid::API::IMDHistoWorkspace_sptr histoWorkspace =
        boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(workspace);

    if (eventWorkspace) {
      sourceType = "MDEW Source";
    } else if (histoWorkspace) {
      sourceType = "MDHW Source";
    } else {
      return;
    }

    emit switchSources(workspaceName, sourceType);
  }
}

/**
 * Catch the deletion of either the rebinned or the original workspace.
 * @param wsName The name of the workspace.
 */
void RebinnedSourcesManager::preDeleteHandle(
    const std::string &wsName,
    const boost::shared_ptr<Mantid::API::Workspace>) {
  // Check if the workspace which is to be deleted is a tracked rebinned
  // workspace
  std::vector<pqPipelineSource *> sources =
      findAllRebinnedSourcesForWorkspace(wsName);

  for (std::vector<pqPipelineSource *>::const_iterator it = sources.begin();
       it != sources.end(); ++it) {
    // Untrack the source
    untrackWorkspaces(createKeyPairForSource(*it));

    // Delete pipeline
    removePipeline(*it);
  }
}

/**
 * Catch a change of a workspace
 * @param workspaceName Name of the workspace.
 * @param workspace A pointer to the added workspace.
 */
void RebinnedSourcesManager::afterReplaceHandle(
    const std::string &workspaceName,
    const boost::shared_ptr<Mantid::API::Workspace> workspace) {
  addHandle(workspaceName, workspace);
}

/**
 * Check if the sources are valid.
 * @param src The pipeline source.
 * @param inputWorkspace Reference for the name of the input workspace.
 * @param outputWorkspace Reference for the name of the output workspace.
 * @param algorithmType The type of the algorithm which will be used to create
 * the rebinned source.
 */
void RebinnedSourcesManager::checkSource(pqPipelineSource *src,
                                         std::string &inputWorkspace,
                                         std::string &outputWorkspace,
                                         std::string algorithmType) {
  pqPipelineSource *source = goToPipelineBeginning(src);

  std::string workspaceName;
  std::string workspaceType;

  getWorkspaceInfo(source, workspaceName, workspaceType);

  bool isHistoWorkspace =
      workspaceType.find("MDHistoWorkspace") != std::string::npos;
  bool isEventWorkspace =
      workspaceType.find("MDEventWorkspace") != std::string::npos;

  // Check if it is a Histo or Event workspace, if it is neither, then don't do
  // anything
  if (isHistoWorkspace || isEventWorkspace) {
    processWorkspaceNames(inputWorkspace, outputWorkspace, source,
                          workspaceName, std::move(algorithmType));
  }
}

/**
 * Get workspace name and type
 * @param source The pipeline source.
 * @param workspaceName Reference to workspace name.
 * @param workspaceType Reference to workspace type.
 */
void RebinnedSourcesManager::getWorkspaceInfo(pqPipelineSource *source,
                                              std::string &workspaceName,
                                              std::string &workspaceType) {
  // Make sure that the input source exists. Note that this can happen when
  // there is no active view
  if (!source) {
    return;
  }

  // Update the source/filter
  vtkSMProxy *proxy = source->getProxy();
  proxy->UpdateVTKObjects();
  proxy->UpdatePropertyInformation();
  source->updatePipeline();

  // Crawl up to the source level
  pqPipelineFilter *filter = qobject_cast<pqPipelineFilter *>(source);

  while (filter) {
    source = filter->getInput(0);
    filter = qobject_cast<pqPipelineFilter *>(source);
  }

  // Ensure that the source is either an MDEvent source or an MDHisto source
  std::string sourceName(source->getProxy()->GetXMLName());
  if (sourceName.find("MDEW Source") == std::string::npos &&
      sourceName.find("MDHW Source") == std::string::npos) {
    return;
  }

  // Check if the source has an underlying event workspace or histo workspace
  workspaceName = vtkSMPropertyHelper(source->getProxy(), "WorkspaceName", true)
                      .GetAsString();

  workspaceType =
      vtkSMPropertyHelper(source->getProxy(), "WorkspaceTypeName", true)
          .GetAsString();
}

/**
 * Creates the pipeline for the rebinned source.
 */
void RebinnedSourcesManager::repipeRebinnedSource() {
  swapSources(m_inputSource, m_rebinnedSource);

  // If we had been dealing with rebinning a rebinned workspace and changing the
  // algorithm,
  // e.g. when changing from BinMD to SliceMD, then we need to untrack the old,
  // rebinned
  // workspace
  if (!m_newRebinnedWorkspacePairBuffer.empty()) {
    untrackWorkspaces(createKeyPairForSource(m_inputSource));
  }

  m_newRebinnedWorkspacePairBuffer.clear();
  m_newWorkspacePairBuffer.clear();

  deleteSpecificSource(m_inputSource);
}

/**
 * Creates the pipeline for the original source.
 * @param rebinnedSource The name of the rebinned source.
 * @param originalSource The name of the original source.
 */
void RebinnedSourcesManager::repipeOriginalSource(
    pqPipelineSource *rebinnedSource, pqPipelineSource *originalSource) {
  // Advance the rebinnedSource to the start of the pipeline
  pqPipelineSource *rebSource = goToPipelineBeginning(rebinnedSource);

  // Swap source from rebinned source to original source.
  swapSources(rebSource, originalSource);

  // Untrack the sources
  untrackWorkspaces(createKeyPairForSource(rebSource));

  deleteSpecificSource(rebSource);
}

/**
 * Swap the sources at the bottom level of the pipeline.
 * @param src1 First source.
 * @param src2 Second source.
 */
void RebinnedSourcesManager::swapSources(pqPipelineSource *src1,
                                         pqPipelineSource *src2) {
  if (!src1 || !src2) {
    throw std::runtime_error("VSI error: Either the original or rebinned "
                             "source don't seem to exist.");
  }

  // Check that both sources contain non-empty data sets

  // Check if the original source has a filter if such then repipe otherwise we
  // are done
  if (src1->getAllConsumers().empty()) {
    // Need to press apply to finalize the internal setup of the source.
    // emit triggerAcceptForNewFilters();
    return;
  }

  // Rebuild pipeline
  rebuildPipeline(src1, src2);

  // Render the active view to make the changes visible.
  pqActiveObjects::instance().activeView()->render();
}

/**
 * Get the stored workspace names assoicated with a source.
 * @param src The name of the source.
 * @param originalWorkspaceName The name of the original workspace.
 * @param rebinnedWorkspaceName The name of the rebinned workspace.
 */
void RebinnedSourcesManager::getStoredWorkspaceNames(
    pqPipelineSource *src, std::string &originalWorkspaceName,
    std::string &rebinnedWorkspaceName) {
  if (!src) {
    return;
  }

  pqPipelineSource *source = goToPipelineBeginning(src);

  // Get the key pair which contains the name of the rebinned workspace and the
  // source name
  std::pair<std::string, std::string> key = createKeyPairForSource(source);
  rebinnedWorkspaceName = key.first;

  if (m_rebinnedWorkspaceAndSourceToOriginalWorkspace.count(key) > 0) {
    originalWorkspaceName =
        m_rebinnedWorkspaceAndSourceToOriginalWorkspace[key];
  } else {
    originalWorkspaceName = "";
  }
}

/**
 * Get all sources which are linked to the workspace which is to be deleted. The
 * workspace
 * can be a rebinned workspace or an unrebinned workspace.
 * @param workspaceName The workspace name associated with the source.
 * @returns a list with all tracked (true sources) which are associated with the
 * workspace
 */
std::vector<pqPipelineSource *>
RebinnedSourcesManager::findAllRebinnedSourcesForWorkspace(
    const std::string &workspaceName) {
  std::vector<std::string> linkedSources;
  // We need to iterate over the map
  for (std::map<std::pair<std::string, std::string>,
                std::string>::const_iterator it =
           m_rebinnedWorkspaceAndSourceToOriginalWorkspace.begin();
       it != m_rebinnedWorkspaceAndSourceToOriginalWorkspace.begin(); ++it) {
    // Check the first part of the key and the value ofthe map
    std::string rebinnedWorkspaceName = it->first.first;
    std::string originalWorkspaceName = it->second;

    if (workspaceName == rebinnedWorkspaceName ||
        workspaceName == originalWorkspaceName) {
      linkedSources.push_back(it->first.second);
    }
  }

  // Now that we have the sources it is time to get them from the ParaView
  // server
  std::vector<pqPipelineSource *> sourcesToBeDeleted;

  pqServer *server = pqActiveObjects::instance().activeServer();
  pqServerManagerModel *smModel =
      pqApplicationCore::instance()->getServerManagerModel();
  const QList<pqPipelineSource *> sources =
      smModel->findItems<pqPipelineSource *>(server);

  foreach (pqPipelineSource *source, sources) {
    pqPipelineFilter *filter = qobject_cast<pqPipelineFilter *>(source);
    if (!filter) {
      std::string sourceName(source->getProxy()->GetGlobalIDAsString());

      if (doesSourceNeedToBeDeleted(sourceName, linkedSources)) {
        sourcesToBeDeleted.push_back(source);
      }
    }
  }

  return sourcesToBeDeleted;
}

/**
 * Check if a source is being tracked and hence needs to be deleted
 * @param sourceName the name of the source which we want to check
 * @param trackedSources a list of tracked sources which need to be deleted
 */
bool RebinnedSourcesManager::doesSourceNeedToBeDeleted(
    const std::string &sourceName,
    const std::vector<std::string> &trackedSources) {
  if (sourceName.empty())
    return false;

  return std::find(trackedSources.cbegin(), trackedSources.cend(),
                   sourceName) != trackedSources.cend();
}

/**
 * Process the workspaces names for the original source and the input source
 * @param inputWorkspace Reference to the input workpspace.
 * @param outputWorkspace Reference to the output workspace.
 * @param source A pointer to the original source
 * @param workspaceName The name of the workspace of the current source.
 * @param algorithmType The algorithm which creates the rebinned source.
 */
void RebinnedSourcesManager::processWorkspaceNames(
    std::string &inputWorkspace, std::string &outputWorkspace,
    pqPipelineSource *source, std::string workspaceName,
    const std::string &algorithmType) {
  // Reset the temporary tracking elements, which are needed only for the
  // duration of the rebinning itself
  m_newWorkspacePairBuffer.clear();
  m_newRebinnedWorkspacePairBuffer.clear();
  m_inputSource = nullptr;
  m_rebinnedSource = nullptr;

  // If the workspace is the original workspace or it is a freshly loaded, i.e.
  // it is not being tracked
  if (workspaceName.find(m_tempPostfix) == std::string::npos ||
      !isRebinnedSourceBeingTracked(source)) {
    inputWorkspace = workspaceName;
    outputWorkspace =
        m_tempPrefix + workspaceName + algorithmType + m_tempPostfix;
  } // If the workspace is rebinned and was created with the same algorithm as
  // the currently selected one.
  else if (workspaceName.find(algorithmType) != std::string::npos &&
           workspaceName.find(m_tempPostfix) != std::string::npos) {
    // Check if the source and the workspace name are being tracked already
    if (isRebinnedSourceBeingTracked(source)) {
      inputWorkspace = m_rebinnedWorkspaceAndSourceToOriginalWorkspace
          [createKeyPairForSource(source)];
      outputWorkspace = workspaceName;
    }
  } else // If the workspace is rebinned but was not created with the same
         // algorithm as the currently selected one.
  {
    if (isRebinnedSourceBeingTracked(source)) {
      inputWorkspace = m_rebinnedWorkspaceAndSourceToOriginalWorkspace
          [createKeyPairForSource(source)];
      outputWorkspace =
          m_tempPrefix + inputWorkspace + algorithmType + m_tempPostfix;
      // Keep track of the old rebinned workspace and source
      m_newRebinnedWorkspacePairBuffer.emplace(
          workspaceName, std::make_pair(outputWorkspace, source));
    }
  }
  // Record the workspaces
  m_newWorkspacePairBuffer.emplace(inputWorkspace,
                                   std::make_pair(outputWorkspace, source));
  m_inputSource = source;
}

/**
 * Stop keeping tabs on the specific workspace pair
 * @param key a key to the tracking map
 */
void RebinnedSourcesManager::untrackWorkspaces(
    const std::pair<std::string, std::string> &key) {
  if (m_rebinnedWorkspaceAndSourceToOriginalWorkspace.count(key) > 0) {
    m_rebinnedWorkspaceAndSourceToOriginalWorkspace.erase(key);
  }
}

/**
 * Rebuild the pipeline for the new source
 * @param source1 The old source.
 * @param source2 The new source.
 */
void RebinnedSourcesManager::rebuildPipeline(pqPipelineSource *source1,
                                             pqPipelineSource *source2) {
  // Step through all the filters in old pipeline and reproduce them
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  pqPipelineFilter *filter1 =
      qobject_cast<pqPipelineFilter *>(source1->getConsumer(0));

  pqPipelineSource *endOfSource2Pipeline = source2;

  while (filter1) {
    vtkSMProxy *proxy1 = filter1->getProxy();
    pqPipelineSource *newPipelineElement = nullptr;
    pqPipelineFilter *newFilter = nullptr;
    // Move source2 to its end.
    while (endOfSource2Pipeline->getNumberOfConsumers() > 0) {
      endOfSource2Pipeline = endOfSource2Pipeline->getConsumer(0);
    }

    if (QString(proxy1->GetXMLName()).contains("ScaleWorkspace")) {
      // Build the source
      newPipelineElement = builder->createFilter(
          "filters", "MantidParaViewScaleWorkspace", endOfSource2Pipeline);
    } else if (QString(proxy1->GetXMLName()).contains("Cut")) {
      newPipelineElement =
          builder->createFilter("filters", "Cut", endOfSource2Pipeline);
    } else if (QString(proxy1->GetXMLName()).contains("Threshold")) {
      newPipelineElement =
          builder->createFilter("filters", "Threshold", endOfSource2Pipeline);
    } else {
      QString message = QString("The filter ") + QString(proxy1->GetXMLName()) +
                        QString(" is not known. You need to add it to the list "
                                "of filters in the RebinnedSourcesManager");
      throw std::runtime_error(message.toStdString());
    }

    newFilter = qobject_cast<pqPipelineFilter *>(newPipelineElement);

    // Copy the properties from the old filter to the new filter.
    copyProperties(filter1, newFilter);

    if (filter1->getNumberOfConsumers() > 0) {
      filter1 = qobject_cast<pqPipelineFilter *>(filter1->getConsumer(0));
    } else {
      filter1 = nullptr;
    }
  }
  emit triggerAcceptForNewFilters();
}

/**
 * Copy the properties of the old filter to the new filter.
 * @param filter1 The old filter.
 * @param filter2 The new filter.
 */
void RebinnedSourcesManager::copyProperties(pqPipelineFilter *filter1,
                                            pqPipelineFilter *filter2) {
  vtkSMProxy *proxy1 = filter1->getProxy();
  vtkSMProxy *proxy2 = filter2->getProxy();

  copySafe(proxy2, proxy1);
}

/**
 * This method is taken from a newer version of pqCopyReaction, which contains
 * a bug fix
 * for copying CutFilter properties. This is the correct way to copy proxy
 * properties.
 * @param dest Destination proxy.
 * @param source Source proxy.
 */
void RebinnedSourcesManager::copySafe(vtkSMProxy *dest, vtkSMProxy *source) {
  if (dest && source) {
    BEGIN_UNDO_SET("Copy Properties");
    dest->Copy(source, "vtkSMProxyProperty");

    // handle proxy properties.
    auto destIter = vtkSmartPointer<vtkSMPropertyIterator>::Take(
        dest->NewPropertyIterator());
    for (destIter->Begin(); !destIter->IsAtEnd(); destIter->Next()) {
      if (vtkSMInputProperty::SafeDownCast(destIter->GetProperty())) {
        // skip input properties.
        continue;
      }

      vtkSMProxyProperty *destPP =
          vtkSMProxyProperty::SafeDownCast(destIter->GetProperty());
      vtkSMProxyProperty *srcPP = vtkSMProxyProperty::SafeDownCast(
          source->GetProperty(destIter->GetKey()));

      if (!destPP || !srcPP || srcPP->GetNumberOfProxies() > 1) {
        // skip non-proxy properties since those were already copied.
        continue;
      }

      vtkSMProxyListDomain *destPLD = vtkSMProxyListDomain::SafeDownCast(
          destPP->FindDomain("vtkSMProxyListDomain"));
      vtkSMProxyListDomain *srcPLD = vtkSMProxyListDomain::SafeDownCast(
          srcPP->FindDomain("vtkSMProxyListDomain"));

      if (!destPLD || !srcPLD) {
        // we copy proxy properties that have proxy list domains.
        continue;
      }

      if (srcPP->GetNumberOfProxies() == 0) {
        destPP->SetNumberOfProxies(0);
        continue;
      }

      vtkSMProxy *srcValue = srcPP->GetProxy(0);
      vtkSMProxy *destValue = nullptr;

      // find srcValue type in destPLD and that's the proxy to use as destValue.
      for (unsigned int cc = 0;
           srcValue != nullptr && cc < destPLD->GetNumberOfProxyTypes(); cc++) {
        if (srcValue->GetXMLName() && destPLD->GetProxyName(cc) &&
            strcmp(srcValue->GetXMLName(), destPLD->GetProxyName(cc)) == 0 &&
            srcValue->GetXMLGroup() && destPLD->GetProxyGroup(cc) &&
            strcmp(srcValue->GetXMLGroup(), destPLD->GetProxyGroup(cc)) == 0) {
          destValue = destPLD->GetProxy(cc);
          break;
        }
      }

      if (destValue) {
        Q_ASSERT(srcValue != nullptr);
        copySafe(destValue, srcValue);
        destPP->SetProxy(0, destValue);
      }
    }

    dest->UpdateVTKObjects();
    END_UNDO_SET();
  }
}

/**
 * Register the rebinned source. Specifically, connect to the destroyed signal
 * of the rebinned source.
 * @param source The rebinned source.
 */
void RebinnedSourcesManager::registerRebinnedSource(pqPipelineSource *source) {
  if (!source) {
    return;
  }

  // Make sure that the underlying source is associated with the buffered
  // workspace pair and register it
  try {
    // Add entry to map from (rebinnedWsName, rebinnedSourceName) ->
    // (originalWsName)
    if (m_newWorkspacePairBuffer.size() != 1) {
      throw std::runtime_error(
          "Original source for rebinned source could not be found.");
    }

    const std::string &originalWorkspaceName =
        m_newWorkspacePairBuffer.begin()->first;
    const std::string &rebinnedWorkspaceName =
        m_newWorkspacePairBuffer.begin()->second.first;

    std::pair<std::string, std::string> key(rebinnedWorkspaceName,
                                            getSourceName(source));
    m_rebinnedWorkspaceAndSourceToOriginalWorkspace.emplace(
        key, originalWorkspaceName);

    // Record the rebinned source
    m_rebinnedSource = source;
  } catch (std::runtime_error &ex) {
    g_log.warning() << ex.what();
  }

  QObject::connect(source, SIGNAL(destroyed()), this,
                   SLOT(onRebinnedSourceDestroyed()));
}

/**
 * React to the deletion of a rebinned source. We need to make sure that the
 * source is untracked
 */
void RebinnedSourcesManager::onRebinnedSourceDestroyed() {
  pqServer *server = pqActiveObjects::instance().activeServer();
  pqServerManagerModel *smModel =
      pqApplicationCore::instance()->getServerManagerModel();
  QList<pqPipelineSource *> sources =
      smModel->findItems<pqPipelineSource *>(server);

  std::vector<std::pair<std::string, std::string>> toBeUntracked;

  // Compare all registered sources to all loaded sources,
  for (const auto &wsPair : m_rebinnedWorkspaceAndSourceToOriginalWorkspace) {
    const std::string &registeredSourceName = wsPair.first.second;

    auto source =
        std::find_if(sources.begin(), sources.end(),
                     [this, &registeredSourceName](pqPipelineSource *src) {
                       return registeredSourceName == getSourceName(src);
                     });

    // If there was no matching source then mark it to be untracked
    if (source == sources.end()) {
      toBeUntracked.push_back(wsPair.first);
    }
  }

  // Finally untrack all sources which need it
  for (const auto &key : toBeUntracked) {
    untrackWorkspaces(key);
  }
}

/**
 * Remove the pipeline
 */
void RebinnedSourcesManager::removePipeline(pqPipelineSource *source) {
  // Go to the end of the pipeline
  while (source->getNumberOfConsumers() > 0) {
    source = source->getConsumer(0);
  }

  // Destroy the pipeline from the end
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  pqPipelineFilter *filter = qobject_cast<pqPipelineFilter *>(source);

  while (filter) {
    source = filter->getInput(0);
    builder->destroy(filter);
    filter = qobject_cast<pqPipelineFilter *>(source);
  }

  builder->destroy(source); // The listener takes now care of the workspace.
}

/**
 * Gets the source name
 * @param source A pointer to a source
 * @returns the name of the source
 */
std::string RebinnedSourcesManager::getSourceName(pqPipelineSource *source) {
  return std::string(source->getProxy()->GetGlobalIDAsString());
}

/**
 * Check if a rebinned source is being tracked.
 * @param src A pointer to a source
 * @returns if it is being tracked or not
 */
bool RebinnedSourcesManager::isRebinnedSourceBeingTracked(
    pqPipelineSource *src) {
  pqPipelineSource *source = goToPipelineBeginning(src);

  std::pair<std::string, std::string> key = createKeyPairForSource(source);

  if (m_rebinnedWorkspaceAndSourceToOriginalWorkspace.count(key) > 0) {
    return true;
  } else {
    return false;
  }
}

std::string RebinnedSourcesManager::saveToProject() {
  if (!m_inputSource || !m_rebinnedSource)
    return "";

  MantidQt::API::TSVSerialiser tsv;
  auto &activeObjects = pqActiveObjects::instance();
  auto proxyManager = activeObjects.activeServer()->proxyManager();
  auto source = activeObjects.activeSource();

  std::string rebinName, origName;
  getStoredWorkspaceNames(source, origName, rebinName);

  tsv.writeLine("RebinnedWorkspaceName") << rebinName;
  tsv.writeLine("RebinnedProxyName")
      << proxyManager->GetProxyName("sources", m_rebinnedSource->getProxy());
  tsv.writeLine("OriginalWorkspaceName") << origName;

  return tsv.outputLines();
}

void RebinnedSourcesManager::loadFromProject(const std::string &lines) {
  MantidQt::API::TSVSerialiser tsv(lines);

  std::string rebinWorkspaceName, originalWorkspaceName, rebinProxyName;
  tsv.selectLine("RebinnedWorkspaceName");
  tsv >> rebinWorkspaceName;
  tsv.selectLine("OriginalWorkspaceName");
  tsv >> originalWorkspaceName;
  tsv.selectLine("RebinnedProxyName");
  tsv >> rebinProxyName;

  auto proxyManager =
      pqActiveObjects::instance().activeServer()->proxyManager();
  auto model = pqApplicationCore::instance()->getServerManagerModel();
  auto rebinSourceProxy =
      proxyManager->GetProxy("sources", rebinProxyName.c_str());
  auto rebinSource = model->findItem<pqPipelineSource *>(rebinSourceProxy);

  m_rebinnedSource = rebinSource;
  m_newWorkspacePairBuffer.emplace(
      originalWorkspaceName, std::make_pair(rebinWorkspaceName, rebinSource));
  registerRebinnedSource(rebinSource);
}

/**
 * Construct a workspaceName, sourceName key pair for a given source
 * @param source A pointer to a source
 * @returns a key which can be used with the tracking map
 */
std::pair<std::string, std::string>
RebinnedSourcesManager::createKeyPairForSource(pqPipelineSource *source) {
  if (!source) {
    return std::pair<std::string, std::string>("", "");
  }

  std::string workspaceName =
      vtkSMPropertyHelper(source->getProxy(), "WorkspaceName", true)
          .GetAsString();
  return std::pair<std::string, std::string>(workspaceName,
                                             getSourceName(source));
}

/**
 * Delete a specific source and all of its filters. This assumes a linear filter
 * system
 * @param source A pointer to the source
 */
void RebinnedSourcesManager::deleteSpecificSource(pqPipelineSource *source) {
  if (source) {
    // Go to the end of the source and work your way back
    pqPipelineSource *tempSource = source;

    while (!tempSource->getAllConsumers().empty()) {
      tempSource = tempSource->getConsumer(0);
    }

    // Now delete all filters and the source
    pqObjectBuilder *builder =
        pqApplicationCore::instance()->getObjectBuilder();

    // Crawl up to the source level
    pqPipelineFilter *filter = qobject_cast<pqPipelineFilter *>(tempSource);

    while (filter) {
      tempSource = filter->getInput(0);
      builder->destroy(filter);
      filter = qobject_cast<pqPipelineFilter *>(tempSource);
    }

    builder->destroy(tempSource);
  }
}

/**
 * Go to the beginning of a pipeline (assuming that it is linear
 * @param source A pointer to a source
 * @returns a pointer to the beginning of the pipeline
 */
pqPipelineSource *
RebinnedSourcesManager::goToPipelineBeginning(pqPipelineSource *source) {
  // Crawl up to the beginnign of the pipeline
  pqPipelineSource *tempSource = source;
  pqPipelineFilter *filter = qobject_cast<pqPipelineFilter *>(tempSource);
  while (filter) {
    tempSource = filter->getInput(0);
    filter = qobject_cast<pqPipelineFilter *>(tempSource);
  }

  return tempSource;
}
} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid
