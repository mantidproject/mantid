// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/Load.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidAPI/NexusFileLoader.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FacilityInfo.h"

#include <Poco/Path.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <functional>
#include <numeric>
#include <set>

namespace {
/**
 * Convenience function that returns true if the passed vector of vector of
 *strings
 * contains a single string, false if contains more than that (or zero).
 *
 * @param fileNames :: a vector of vectors of file name strings.
 *
 * @returns true if there is exactly one string, else false.
 */
bool isSingleFile(const std::vector<std::vector<std::string>> &fileNames) {
  return fileNames.size() == 1 && fileNames[0u].size() == 1;
}

/**
 * Helper function that takes a vector of filenames, and generates a suggested
 *workspace name.
 *
 * @param filenames :: a vector of filenames.
 *
 * @returns a string containing a suggested ws name based on the given file
 *names.
 */
std::string generateWsNameFromFileNames(const std::vector<std::string> &filenames) {
  std::string wsName;

  for (auto &filename : filenames) {
    if (!wsName.empty())
      wsName += "_";

    Poco::Path path(filename);
    wsName += path.getBaseName();
  }

  return wsName;
}

/**
 * Helper function that takes a vector of vectors of items and flattens it into
 * a single vector of items.
 */
std::vector<std::string> flattenVecOfVec(std::vector<std::vector<std::string>> vecOfVec) {
  std::vector<std::string> flattenedVec;

  std::vector<std::vector<std::string>>::const_iterator it = vecOfVec.begin();

  for (; it != vecOfVec.end(); ++it) {
    flattenedVec.insert(flattenedVec.end(), it->begin(), it->end());
  }

  return flattenedVec;
}
} // namespace

namespace Mantid::DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(Load)

// The mutex
std::recursive_mutex Load::m_mutex;

using namespace Kernel;
using namespace API;

//--------------------------------------------------------------------------
// Public methods
//--------------------------------------------------------------------------

/** Override setPropertyValue to catch if filename is being set, as this may
 * mean
 *  a change of concrete loader. If it's any other property, just forward the
 * call.
 *  @param name The name of the property
 *  @param value The value of the property as a string
 */
void Load::setPropertyValue(const std::string &name, const std::string &value) {
  // Call base class method in all cases.
  // For a filename property is deals with resolving the full path.
  Algorithm::setPropertyValue(name, value);

  std::string NAME(name);
  std::transform(name.begin(), name.end(), NAME.begin(), toupper);
  if (NAME == "FILENAME") {
    // Get back full path before passing to getFileLoader method, and also
    // find out whether this is a multi file load.
    std::vector<std::string> fileNames = flattenVecOfVec(getProperty("Filename"));
    // If it's a single file load, then it's fine to change loader.
    if (fileNames.size() == 1) {
      auto loader = getFileLoader(getPropertyValue(name));
      assert(loader); // (getFileLoader should throw if no loader is found.)
      declareLoaderProperties(loader);
      m_loader = std::move(loader);
    }
    // Else we've got multiple files, and must enforce the rule that only one
    // type of loader is allowed.
    // Allowing more than one would mean that "extra" properties defined by the
    // class user (for example
    // "LoadLogFiles") are potentially ambiguous.
    else if (fileNames.size() > 1) {
      auto loader = getFileLoader(fileNames[0]);

      // If the first file has a loader ...
      if (loader) {
        // ... store its name and version and check that all other files have
        // loaders with the same name and version.
        const std::string loaderName = loader->name();
        int firstFileLoaderVersion = loader->version();

        // std::string ext =
        // fileNames[0].substr(fileNames[0].find_last_of("."));

        auto ifl = std::dynamic_pointer_cast<IFileLoader<Kernel::FileDescriptor>>(loader);
        auto iflNexus = std::dynamic_pointer_cast<IFileLoader<Kernel::NexusDescriptor>>(loader);

        for (size_t i = 1; i < fileNames.size(); ++i) {
          // If it's loading into a single file, perform a cursory check on file
          // extensions only.
          if ((ifl && ifl->loadMutipleAsOne()) || (iflNexus && iflNexus->loadMutipleAsOne())) {
            // Currently disabled for ticket
            // http://trac.mantidproject.org/mantid/ticket/10397 : should be put
            // back in when completing 10231
            /*  if( fileNames[i].substr(fileNames[i].find_last_of(".")) != ext)
            {
              throw std::runtime_error("Cannot load multiple files when more
            than one Loader is needed.");
            }*/
          } else {
            loader = getFileLoader(fileNames[i]);

            if (loaderName != loader->name() || firstFileLoaderVersion != loader->version())
              throw std::runtime_error("Cannot load multiple files when more "
                                       "than one Loader is needed.");
          }
        }
      }

      assert(loader); // (getFileLoader should throw if no loader is found.)
      declareLoaderProperties(loader);
    }
  }
}

//--------------------------------------------------------------------------
// Private methods
//--------------------------------------------------------------------------

/**
 * Get a shared pointer to the load algorithm with highest preference for
 * loading
 * @param filePath :: path of the file
 * @returns A shared pointer to the unmanaged algorithm
 */
API::IAlgorithm_sptr Load::getFileLoader(const std::string &filePath) {
  API::IAlgorithm_sptr winningLoader;
  try {
    winningLoader = API::FileLoaderRegistry::Instance().chooseLoader(filePath);
  } catch (Exception::NotFoundError &) {
    // Clear what may have been here previously
    setPropertyValue("LoaderName", "");
    setProperty("LoaderVersion", -1);
    throw std::runtime_error("Cannot find an algorithm that is able to load \"" + filePath +
                             "\".\n"
                             "Check that the file is a supported type.");
  }
  winningLoader->initialize();
  setUpLoader(winningLoader, 0, 1);

  findFilenameProperty(winningLoader);

  setPropertyValue("LoaderName", winningLoader->name());
  setProperty("LoaderVersion", winningLoader->version());
  return winningLoader;
}

void Load::findFilenameProperty(const API::IAlgorithm_sptr &loader) {
  const auto nxsLoader = std::dynamic_pointer_cast<NexusFileLoader>(loader);
  if (nxsLoader) {
    // NexusFileLoader has a method for giving back the name directly
    m_filenamePropName = nxsLoader->getFilenamePropertyName();
  } else {
    // Use the first file property as the main Filename
    const auto &props = loader->getProperties();
    for (auto const &prop : props) {
      auto const *multiprop = dynamic_cast<API::MultipleFileProperty *>(prop);
      auto const *singleprop = dynamic_cast<API::FileProperty *>(prop);
      if (multiprop) {
        m_filenamePropName = multiprop->name();
        break;
      }
      if (singleprop) {
        m_filenamePropName = singleprop->name();
        break;
      }
    }
  }

  // throw an exception if somehting nothing was found
  if (m_filenamePropName.empty()) {
    // unset member variables
    setPropertyValue("LoaderName", "");
    setProperty("LoaderVersion", -1);

    std::stringstream msg;
    msg << "Cannot find FileProperty on \"" << loader->name() << "\" v" << loader->version() << " algorithm.";
    throw std::runtime_error(msg.str());
  }
}

/**
 * Declare any additional properties of the concrete loader here
 * @param loader A pointer to the concrete loader
 */
void Load::declareLoaderProperties(const API::IAlgorithm_sptr &loader) {
  // If we have switch loaders then the concrete loader will have different
  // properties
  // so take care of ensuring Load has the correct ones
  // THIS IS A COPY as the properties are mutated as we move through them
  const std::vector<Property *> existingProps = this->getProperties();
  for (auto existingProp : existingProps) {
    const std::string &propName = existingProp->name();
    // Wipe all properties except the Load native ones
    if (m_baseProps.find(propName) == m_baseProps.end()) {
      this->removeProperty(propName);
    }
  }

  const std::vector<Property *> &loaderProps = loader->getProperties();
  size_t numProps(loaderProps.size());
  for (size_t i = 0; i < numProps; ++i) {
    Property const *loadProp = loaderProps[i];
    if (loadProp->name() == m_filenamePropName)
      continue;
    try {
      auto propClone = std::unique_ptr<Property>(loadProp->clone());
      propClone->clearSettings(); // Get rid of special settings because it
                                  // does not work in custom GUI.
      declareProperty(std::move(propClone), loadProp->documentation());
    } catch (Exception::ExistsError &) {
      // Already exists as a static property
      continue;
    }
  }
}

/// Initialisation method.
void Load::init() {
  // Take extensions first from Facility object
  const FacilityInfo &defaultFacility = Mantid::Kernel::ConfigService::Instance().getFacility();
  std::vector<std::string> exts = defaultFacility.extensions();
  // Add in some other known extensions
  exts.emplace_back(".xml");
  exts.emplace_back(".dat");
  exts.emplace_back(".txt");
  exts.emplace_back(".csv");
  exts.emplace_back(".spe");
  exts.emplace_back(".grp");
  exts.emplace_back(".nxspe");
  exts.emplace_back(".h5");
  exts.emplace_back(".hd5");
  exts.emplace_back(".sqw");
  exts.emplace_back(".fits");
  exts.emplace_back(".bin");
  exts.emplace_back(".edb");

  declareProperty(std::make_unique<MultipleFileProperty>("Filename", exts),
                  "The name of the file(s) to read, including the full or relative "
                  "path. (N.B. case sensitive if running on Linux). Multiple runs "
                  "can be loaded and added together, e.g. INST10,11+12,13.ext");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace that will be created, filled with the "
                  "read-in data and stored in the Analysis Data Service. Some algorithms "
                  "can created additional OutputWorkspace properties on the fly, e.g. "
                  "multi-period data.");

  declareProperty("LoaderName", std::string(""),
                  "When an algorithm has been found that will load the given "
                  "file, its name is set here.",
                  Direction::Output);
  declareProperty("LoaderVersion", -1,
                  "When an algorithm has been found that "
                  "will load the given file, its version "
                  "is set here.",
                  Direction::Output);
  // Save for later what the base Load properties are
  const std::vector<Property *> &props = this->getProperties();
  for (size_t i = 0; i < this->propertyCount(); ++i) {
    m_baseProps.insert(props[i]->name());
  }
}

/**
 * Executes the algorithm.
 */
void Load::exec() {
  std::vector<std::vector<std::string>> fileNames = getProperty("Filename");

  // Test for loading as a single file
  if (!m_loader)
    m_loader = getFileLoader(fileNames[0][0]);
  auto ifl = std::dynamic_pointer_cast<IFileLoader<Kernel::FileDescriptor>>(m_loader);
  auto iflNexus = std::dynamic_pointer_cast<IFileLoader<Kernel::NexusDescriptor>>(m_loader);

  if (isSingleFile(fileNames) || (ifl && ifl->loadMutipleAsOne()) || (iflNexus && iflNexus->loadMutipleAsOne())) {
    // This is essentially just the same code that was called before multiple
    // files were supported.
    loadSingleFile();
  } else {
    // Code that supports multiple file loading.
    loadMultipleFiles();
  }

  // Set the remaining properties of the loader
  setOutputProperties(m_loader);

  /**
   * Set the loader and version from correct loader IF one has been found,
   * so that caller can reuse these variables for later calls.
   * Though these are set in call to getFileLoader, they can be errantly
   * changed by the init process .
   */
  if (m_loader) {
    setPropertyValue("LoaderName", m_loader->name());
    setProperty("LoaderVersion", m_loader->version());
  }
}

void Load::loadSingleFile() {
  if (!m_loader) {
    m_loader = getFileLoader(getPropertyValue("Filename"));
  }

  g_log.information() << "Using " << m_loader->name() << " version " << m_loader->version() << ".\n";
  /// get the list properties for the concrete loader load algorithm
  const std::vector<Kernel::Property *> &loader_props = m_loader->getProperties();

  // Loop through and set the properties on the Child Algorithm
  std::vector<Kernel::Property *>::const_iterator itr;
  for (itr = loader_props.begin(); itr != loader_props.end(); ++itr) {
    const std::string propName = (*itr)->name();
    if (this->existsProperty(propName)) {
      m_loader->setPropertyValue(propName, getPropertyValue(propName));
    } else if (propName == m_filenamePropName) {
      m_loader->setPropertyValue(propName, getPropertyValue("Filename"));
    }
  }

  // Execute the concrete loader
  m_loader->execute();

  // Set the output Workspace
  Workspace_sptr wks = getOutputWorkspace("OutputWorkspace", m_loader);
  setProperty("OutputWorkspace", wks);
}

void Load::loadMultipleFiles() {
  // allFilenames contains "rows" of filenames. If the row has more than 1 file
  // in it
  // then that row is to be summed across each file in the row
  const std::vector<std::vector<std::string>> allFilenames = getProperty("Filename");
  std::string outputWsName = getProperty("OutputWorkspace");

  std::vector<std::string> wsNames(allFilenames.size());
  std::transform(allFilenames.begin(), allFilenames.end(), wsNames.begin(), generateWsNameFromFileNames);

  auto wsName = wsNames.cbegin();
  assert(allFilenames.size() == wsNames.size());

  std::vector<API::Workspace_sptr> loadedWsList;
  loadedWsList.reserve(allFilenames.size());

  Workspace_sptr tempWs;

  // Cycle through the filenames and wsNames.
  for (auto filenames = allFilenames.cbegin(); filenames != allFilenames.cend(); ++filenames, ++wsName) {
    auto filename = filenames->cbegin();
    Workspace_sptr sumWS = loadFileToWs(*filename, *wsName);

    ++filename;
    for (; filename != filenames->cend(); ++filename) {
      tempWs = loadFileToWs(*filename, "__@loadsum_temp@");
      sumWS = plusWs(sumWS, tempWs);
    }

    API::WorkspaceGroup_sptr group = std::dynamic_pointer_cast<WorkspaceGroup>(sumWS);
    if (group) {
      std::vector<std::string> childWsNames = group->getNames();
      auto childWsName = childWsNames.begin();
      size_t count = 1;
      for (; childWsName != childWsNames.end(); ++childWsName, ++count) {
        Workspace_sptr childWs = group->getItem(*childWsName);
        const std::string childName = group->getName() + "_" + std::to_string(count);
        API::AnalysisDataService::Instance().addOrReplace(childName, childWs);
        // childWs->setName(group->getName() + "_" +
        // boost::lexical_cast<std::string>(count));
      }
    }
    // Add the sum to the list of loaded workspace names.
    loadedWsList.emplace_back(sumWS);
  }

  // If we only have one loaded ws, set it as the output.
  if (loadedWsList.size() == 1) {
    setProperty("OutputWorkspace", loadedWsList[0]);
    AnalysisDataService::Instance().rename(loadedWsList[0]->getName(), outputWsName);
  }
  // Else we have multiple loaded workspaces - group them and set the group as
  // output.
  else {
    API::WorkspaceGroup_sptr group = groupWsList(loadedWsList);
    setProperty("OutputWorkspace", group);

    std::vector<std::string> childWsNames = group->getNames();
    size_t count = 1;
    for (auto &childWsName : childWsNames) {
      if (childWsName == outputWsName) {
        Mantid::API::Workspace_sptr child = group->getItem(childWsName);
        // child->setName(child->getName() + "_" +
        // boost::lexical_cast<std::string>(count));
        const std::string childName = child->getName() + "_" + std::to_string(count);
        API::AnalysisDataService::Instance().addOrReplace(childName, child);
        count++;
      }
    }

    childWsNames = group->getNames();
    count = 1;
    for (auto &childWsName : childWsNames) {
      Workspace_sptr childWs = group->getItem(childWsName);
      std::string outWsPropName = "OutputWorkspace_" + std::to_string(count);
      ++count;
      declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(outWsPropName, childWsName, Direction::Output));
      setProperty(outWsPropName, childWs);
    }
  }

  // Clean up.
  if (tempWs) {
    Algorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged("DeleteWorkspace");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("Workspace", tempWs);
    alg->execute();
  }
}

/**
 * Create the concrete instance use for the actual loading.
 * @param startProgress :: The percentage progress value of the overall
 * algorithm where this child algorithm starts
 * @param endProgress :: The percentage progress value of the overall
 * algorithm where this child algorithm ends
 * @param logging :: Set to false to disable logging from the child algorithm
 */
API::IAlgorithm_sptr Load::createLoader(const double startProgress, const double endProgress,
                                        const bool logging) const {
  std::string loaderName = getPropertyValue("LoaderName");
  int loaderVersion = getProperty("LoaderVersion");
  auto loader = API::AlgorithmManager::Instance().createUnmanaged(loaderName, loaderVersion);
  loader->initialize();
  if (!loader) {
    throw std::runtime_error("Cannot create loader for \"" + getPropertyValue("Filename") + "\"");
  }
  setUpLoader(loader, startProgress, endProgress, logging);
  return loader;
}

/**
 * Set the loader option for use as a Child Algorithm.
 * @param loader :: Concrete loader
 * @param startProgress :: The start progress fraction
 * @param endProgress :: The end progress fraction
 * @param logging:: If true, enable logging
 */
void Load::setUpLoader(const API::IAlgorithm_sptr &loader, const double startProgress, const double endProgress,
                       const bool logging) const {
  // Set as a child so that we are in control of output storage
  loader->setChild(true);
  loader->setLogging(logging);
  // If output workspaces are nameless, give them a temporary name to satisfy
  // validator
  const std::vector<Property *> &props = loader->getProperties();
  for (auto prop : props) {
    auto wsProp = dynamic_cast<IWorkspaceProperty *>(prop);

    if (wsProp && !wsProp->isOptional() && prop->direction() == Direction::Output) {
      if (prop->value().empty())
        prop->setValue("LoadChildWorkspace");
    }
  }
  if (startProgress >= 0. && endProgress > startProgress && endProgress <= 1.) {
    loader->addObserver(this->progressObserver());
    setChildStartProgress(startProgress);
    setChildEndProgress(endProgress);
  }
}

/**
 * Set all the output properties from the loader used to Load algorithm itself
 * @param loader :: Shared pointer to the load algorithm
 */
void Load::setOutputProperties(const API::IAlgorithm_sptr &loader) {
  // Set output properties by looping the loaders properties and taking them until we have none left
  while (loader->propertyCount() > 0) {
    auto prop = loader->takeProperty(0);
    if (prop && prop->direction() == Direction::Output) {
      // We skip OutputWorkspace as this is set already from loadSingleFile and loadMultipleFiles
      if (prop->name() != "OutputWorkspace")
        declareOrReplaceProperty(std::move(prop));
    }
  }
}

/**
 * Return an output workspace property dealing with the lack of connection
 * between of
 * WorkspaceProperty types
 * @param propName :: The name of the property
 * @param loader :: The loader algorithm
 * @returns A pointer to the OutputWorkspace property of the Child Algorithm
 */
API::Workspace_sptr Load::getOutputWorkspace(const std::string &propName, const API::IAlgorithm_sptr &loader) const {
  // @todo Need to try and find a better way using the getValue methods
  try {
    return loader->getProperty(propName);
  } catch (std::runtime_error &) {
  }

  // Try a MatrixWorkspace
  try {
    MatrixWorkspace_sptr childWS = loader->getProperty(propName);
    return childWS;
  } catch (std::runtime_error &) {
  }

  // EventWorkspace
  try {
    IEventWorkspace_sptr childWS = loader->getProperty(propName);
    return childWS;
  } catch (std::runtime_error &) {
  }

  // IMDEventWorkspace
  try {
    IMDEventWorkspace_sptr childWS = loader->getProperty(propName);
    return childWS;
  } catch (std::runtime_error &) {
  }

  // General IMDWorkspace
  try {
    IMDWorkspace_sptr childWS = loader->getProperty(propName);
    return childWS;
  } catch (std::runtime_error &) {
  }

  // ITableWorkspace?
  try {
    ITableWorkspace_sptr childWS = loader->getProperty(propName);
    return childWS;
  } catch (std::runtime_error &) {
  }

  // WorkspaceGroup?
  try {
    WorkspaceGroup_sptr childWS = loader->getProperty(propName);
    return childWS;
  } catch (std::runtime_error &) {
  }

  // Just workspace?
  try {
    Workspace_sptr childWS = loader->getProperty(propName);
    return childWS;
  } catch (std::runtime_error &) {
  }

  g_log.debug() << "Workspace property " << propName
                << " did not return to MatrixWorkspace, EventWorkspace, "
                   "IMDEventWorkspace, IMDWorkspace\n";
  return Workspace_sptr();
}

/*
 * Overrides the default cancel() method. Calls cancel() on the actual loader.
 */
void Load::cancel() {
  if (m_loader) {
    m_loader->cancel();
  }
}

/**
 * Loads a file into a *hidden* workspace.
 *
 * @param fileName :: file name to load.
 * @param wsName   :: workspace name, which will be prefixed by a "__"
 *
 * @returns a pointer to the loaded workspace
 */
API::Workspace_sptr Load::loadFileToWs(const std::string &fileName, const std::string &wsName) {
  auto loadAlg = createChildAlgorithm("Load", 1);

  // Get the list properties for the concrete loader load algorithm
  const std::vector<Kernel::Property *> &props = getProperties();

  // Loop through and set the properties on the Child Algorithm
  for (auto prop : props) {
    const std::string &propName = prop->name();

    if (this->existsProperty(propName)) {
      if (propName == "Filename") {
        loadAlg->setPropertyValue("Filename", fileName);
      } else if (propName == "OutputWorkspace") {
        loadAlg->setPropertyValue("OutputWorkspace", wsName);
      } else {
        loadAlg->setPropertyValue(propName, getPropertyValue(propName));
      }
    }
  }

  loadAlg->executeAsChildAlg();

  Workspace_sptr ws = loadAlg->getProperty("OutputWorkspace");
  // ws->setName(wsName);
  AnalysisDataService::Instance().addOrReplace(wsName, ws);
  m_loader = loadAlg;
  return ws;
}

/**
 * Plus two workspaces together, "in place".
 *
 * @param ws1 :: The first workspace.
 * @param ws2 :: The second workspace.
 *
 * @returns a pointer to the result (the first workspace).
 */
API::Workspace_sptr Load::plusWs(Workspace_sptr ws1, const Workspace_sptr &ws2) {
  WorkspaceGroup_sptr group1 = std::dynamic_pointer_cast<WorkspaceGroup>(ws1);
  WorkspaceGroup_sptr group2 = std::dynamic_pointer_cast<WorkspaceGroup>(ws2);

  if (group1 && group2) {
    // If we're dealing with groups, then the child workspaces must be added
    // separately - setProperty
    // wont work otherwise.
    std::vector<std::string> group1ChildWsNames = group1->getNames();
    std::vector<std::string> group2ChildWsNames = group2->getNames();

    if (group1ChildWsNames.size() != group2ChildWsNames.size())
      throw std::runtime_error("Unable to add group workspaces with different "
                               "number of child workspaces.");

    auto group1ChildWsName = group1ChildWsNames.begin();
    auto group2ChildWsName = group2ChildWsNames.begin();

    for (; group1ChildWsName != group1ChildWsNames.end(); ++group1ChildWsName, ++group2ChildWsName) {
      Workspace_sptr group1ChildWs = group1->getItem(*group1ChildWsName);
      Workspace_sptr group2ChildWs = group2->getItem(*group2ChildWsName);

      auto plusAlg = createChildAlgorithm("Plus", 1);
      plusAlg->setProperty<Workspace_sptr>("LHSWorkspace", group1ChildWs);
      plusAlg->setProperty<Workspace_sptr>("RHSWorkspace", group2ChildWs);
      plusAlg->setProperty<Workspace_sptr>("OutputWorkspace", group1ChildWs);
      plusAlg->executeAsChildAlg();
    }
  } else if (!group1 && !group2) {
    auto plusAlg = createChildAlgorithm("Plus", 1);
    plusAlg->setProperty<Workspace_sptr>("LHSWorkspace", ws1);
    plusAlg->setProperty<Workspace_sptr>("RHSWorkspace", ws2);
    plusAlg->setProperty<Workspace_sptr>("OutputWorkspace", ws1);
    plusAlg->executeAsChildAlg();
  } else {
    throw std::runtime_error("Unable to add a group workspace to a non-group workspace");
  }

  return ws1;
}

/**
 * Groups together a vector of workspaces.  This is done "manually", since the
 * workspaces being passed will be outside of the ADS and so the GroupWorkspaces
 * alg is not an option here.
 *
 * @param wsList :: the list of workspaces to group
 */
API::WorkspaceGroup_sptr Load::groupWsList(const std::vector<API::Workspace_sptr> &wsList) {
  auto group = std::make_shared<WorkspaceGroup>();

  for (const auto &ws : wsList) {
    WorkspaceGroup_sptr isGroup = std::dynamic_pointer_cast<WorkspaceGroup>(ws);
    // If the ws to add is already a group, then add its children individually.
    if (isGroup) {
      std::vector<std::string> childrenNames = isGroup->getNames();
      size_t count = 1;
      for (auto childName = childrenNames.begin(); childName != childrenNames.end(); ++childName, ++count) {
        Workspace_sptr childWs = isGroup->getItem(*childName);
        isGroup->remove(*childName);
        // childWs->setName(isGroup->getName() + "_" +
        // boost::lexical_cast<std::string>(count));
        group->addWorkspace(childWs);
      }

      // Remove the old group from the ADS
      AnalysisDataService::Instance().remove(isGroup->getName());
    } else {
      group->addWorkspace(ws);
    }
  }

  return group;
}
} // namespace Mantid::DataHandling
