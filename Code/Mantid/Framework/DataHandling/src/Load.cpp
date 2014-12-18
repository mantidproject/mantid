//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/Load.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FacilityInfo.h"

#include <Poco/Path.h>

#include <cctype>
#include <algorithm>
#include <functional>
#include <numeric>
#include <set>
#include <cstdio>

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
  if (fileNames.size() == 1) {
    std::vector<std::vector<std::string>>::const_iterator first =
        fileNames.begin();
    if (first->size() == 1)
      return true;
  }
  return false;
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
std::string generateWsNameFromFileNames(std::vector<std::string> filenames) {
  std::string wsName("");

  for (size_t i = 0; i < filenames.size(); ++i) {
    if (!wsName.empty())
      wsName += "_";

    Poco::Path path(filenames[i]);
    wsName += path.getBaseName();
  }

  return wsName;
}

/**
 * Helper function that takes a vector of vectors of items and flattens it into
 * a single vector of items.
 */
std::vector<std::string>
flattenVecOfVec(std::vector<std::vector<std::string>> vecOfVec) {
  std::vector<std::string> flattenedVec;

  std::vector<std::vector<std::string>>::const_iterator it = vecOfVec.begin();

  for (; it != vecOfVec.end(); ++it) {
    flattenedVec.insert(flattenedVec.end(), it->begin(), it->end());
  }

  return flattenedVec;
}
}

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(Load);

// The mutex
Poco::Mutex Load::m_mutex;

using namespace Kernel;
using namespace API;

//--------------------------------------------------------------------------
// Public methods
//--------------------------------------------------------------------------

/// Default constructor
Load::Load() : Algorithm(), m_baseProps(), m_loader(), m_filenamePropName() {}

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
    std::vector<std::string> fileNames =
        flattenVecOfVec(getProperty("Filename"));
    // If it's a single file load, then it's fine to change loader.
    if (fileNames.size() == 1) {
      IAlgorithm_sptr loader = getFileLoader(getPropertyValue(name));
      assert(loader); // (getFileLoader should throw if no loader is found.)
      declareLoaderProperties(loader);
    }
    // Else we've got multiple files, and must enforce the rule that only one
    // type of loader is allowed.
    // Allowing more than one would mean that "extra" properties defined by the
    // class user (for example
    // "LoadLogFiles") are potentially ambiguous.
    else if (fileNames.size() > 1) {
      IAlgorithm_sptr loader = getFileLoader(fileNames[0]);

      // If the first file has a loader ...
      if (loader) {
        // ... store it's name and version and check that all other files have
        // loaders with the same name and version.
        std::string name = loader->name();
        int version = loader->version();

        // std::string ext =
        // fileNames[0].substr(fileNames[0].find_last_of("."));

        auto ifl =
            boost::dynamic_pointer_cast<IFileLoader<Kernel::FileDescriptor>>(
                loader);
        auto iflNexus =
            boost::dynamic_pointer_cast<IFileLoader<Kernel::NexusDescriptor>>(
                loader);

        for (size_t i = 1; i < fileNames.size(); ++i) {
          // If it's loading into a single file, perform a cursory check on file
          // extensions only.
          if ((ifl && ifl->loadMutipleAsOne()) ||
              (iflNexus && iflNexus->loadMutipleAsOne())) {
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

            if (name != loader->name() || version != loader->version())
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
* Get a shared pointer to the load algorithm with highest preference for loading
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
    throw std::runtime_error(
        "Cannot find an algorithm that is able to load \"" + filePath +
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
  // Use the first file property as the main Filename
  const auto &props = loader->getProperties();
  for (auto it = props.begin(); it != props.end(); ++it) {
    auto *fp = dynamic_cast<API::MultipleFileProperty *>(*it);
    auto *fp2 = dynamic_cast<API::FileProperty *>(*it);
    if (fp) {
      m_filenamePropName = fp->name();
      break;
    }
    if (fp2) {
      m_filenamePropName = fp2->name();
      break;
    }
  }
  if (m_filenamePropName.empty()) {
    setPropertyValue("LoaderName", "");
    setProperty("LoaderVersion", -1);
    throw std::runtime_error("Cannot find FileProperty on " + loader->name() +
                             " algorithm.");
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
  for (size_t i = 0; i < existingProps.size(); ++i) {
    const std::string name = existingProps[i]->name();
    // Wipe all properties except the Load native ones
    if (m_baseProps.find(name) == m_baseProps.end()) {
      this->removeProperty(name);
    }
  }

  const std::vector<Property *> &loaderProps = loader->getProperties();
  size_t numProps(loaderProps.size());
  for (size_t i = 0; i < numProps; ++i) {
    Property *loadProp = loaderProps[i];
    if (loadProp->name() == m_filenamePropName)
      continue;
    try {
      Property *propClone = loadProp->clone();
      propClone->deleteSettings(); // Get rid of special settings because it
                                   // does not work in custom GUI.
      declareProperty(propClone, loadProp->documentation());
    } catch (Exception::ExistsError &) {
      // Already exists as a static property
      continue;
    }
  }
}

/// Initialisation method.
void Load::init() {
  // Take extensions first from Facility object
  const FacilityInfo &defaultFacility =
      Mantid::Kernel::ConfigService::Instance().getFacility();
  std::vector<std::string> exts = defaultFacility.extensions();
  // Add in some other known extensions
  exts.push_back(".xml");
  exts.push_back(".dat");
  exts.push_back(".txt");
  exts.push_back(".csv");
  exts.push_back(".spe");
  exts.push_back(".grp");
  exts.push_back(".nxspe");
  exts.push_back(".h5");
  exts.push_back(".hd5");
  exts.push_back(".sqw");
  exts.push_back(".fits");

  declareProperty(
      new MultipleFileProperty("Filename", exts),
      "The name of the file(s) to read, including the full or relative "
      "path. (N.B. case sensitive if running on Linux). Multiple runs "
      "can be loaded and added together, e.g. INST10,11+12,13.ext");
  declareProperty(
      new WorkspaceProperty<Workspace>("OutputWorkspace", "",
                                       Direction::Output),
      "The name of the workspace that will be created, filled with the "
      "read-in data and stored in the Analysis Data Service. Some algorithms "
      "can created additional OutputWorkspace properties on the fly, e.g. "
      "multi-period data.");

  declareProperty("LoaderName", std::string(""),
                  "When an algorithm has been found that will load the given "
                  "file, its name is set here.",
                  Direction::Output);
  declareProperty("LoaderVersion", -1, "When an algorithm has been found that "
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
  IAlgorithm_sptr loader = getFileLoader(fileNames[0][0]);
  auto ifl =
      boost::dynamic_pointer_cast<IFileLoader<Kernel::FileDescriptor>>(loader);
  auto iflNexus =
      boost::dynamic_pointer_cast<IFileLoader<Kernel::NexusDescriptor>>(loader);

  if (isSingleFile(fileNames) || (ifl && ifl->loadMutipleAsOne()) ||
      (iflNexus && iflNexus->loadMutipleAsOne())) {
    // This is essentially just the same code that was called before multiple
    // files were supported.
    loadSingleFile();
  } else {
    // Code that supports multiple file loading.
    loadMultipleFiles();
  }
}

void Load::loadSingleFile() {
  std::string loaderName = getPropertyValue("LoaderName");
  if (loaderName.empty()) {
    m_loader = getFileLoader(getPropertyValue("Filename"));
    loaderName = m_loader->name();
  } else {
    m_loader = createLoader(0, 1);
    findFilenameProperty(m_loader);
  }
  g_log.information() << "Using " << loaderName << " version "
                      << m_loader->version() << ".\n";
  /// get the list properties for the concrete loader load algorithm
  const std::vector<Kernel::Property *> &loader_props =
      m_loader->getProperties();

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
  // Set the workspace. Deals with possible multiple periods
  setOutputWorkspace(m_loader);
}

void Load::loadMultipleFiles() {
  // allFilenames contains "rows" of filenames. If the row has more than 1 file
  // in it
  // then that row is to be summed across each file in the row
  const std::vector<std::vector<std::string>> allFilenames =
      getProperty("Filename");
  std::string outputWsName = getProperty("OutputWorkspace");

  std::vector<std::string> wsNames(allFilenames.size());
  std::transform(allFilenames.begin(), allFilenames.end(), wsNames.begin(),
                 generateWsNameFromFileNames);

  std::vector<std::vector<std::string>>::const_iterator filenames =
      allFilenames.begin();
  std::vector<std::string>::const_iterator wsName = wsNames.begin();
  assert(allFilenames.size() == wsNames.size());

  std::vector<API::Workspace_sptr> loadedWsList;
  loadedWsList.reserve(allFilenames.size());

  Workspace_sptr tempWs;

  // Cycle through the filenames and wsNames.
  for (; filenames != allFilenames.end(); ++filenames, ++wsName) {
    std::vector<std::string>::const_iterator filename = filenames->begin();
    Workspace_sptr sumWS = loadFileToWs(*filename, *wsName);

    ++filename;
    for (; filename != filenames->end(); ++filename) {
      tempWs = loadFileToWs(*filename, "__@loadsum_temp@");
      sumWS = plusWs(sumWS, tempWs);
    }

    API::WorkspaceGroup_sptr group =
        boost::dynamic_pointer_cast<WorkspaceGroup>(sumWS);
    if (group) {
      std::vector<std::string> childWsNames = group->getNames();
      auto childWsName = childWsNames.begin();
      size_t count = 1;
      for (; childWsName != childWsNames.end(); ++childWsName, ++count) {
        Workspace_sptr childWs = group->getItem(*childWsName);
        const std::string childName =
            group->getName() + "_" + boost::lexical_cast<std::string>(count);
        API::AnalysisDataService::Instance().addOrReplace(childName, childWs);
        // childWs->setName(group->getName() + "_" +
        // boost::lexical_cast<std::string>(count));
      }
    }
    // Add the sum to the list of loaded workspace names.
    loadedWsList.push_back(sumWS);
  }

  // If we only have one loaded ws, set it as the output.
  if (loadedWsList.size() == 1) {
    setProperty("OutputWorkspace", loadedWsList[0]);
    AnalysisDataService::Instance().rename(loadedWsList[0]->getName(),
                                           outputWsName);
  }
  // Else we have multiple loaded workspaces - group them and set the group as
  // output.
  else {
    API::WorkspaceGroup_sptr group = groupWsList(loadedWsList);
    setProperty("OutputWorkspace", group);

    std::vector<std::string> childWsNames = group->getNames();
    size_t count = 1;
    for (auto childWsName = childWsNames.begin();
         childWsName != childWsNames.end(); ++childWsName) {
      if (*childWsName == outputWsName) {
        Mantid::API::Workspace_sptr child = group->getItem(*childWsName);
        // child->setName(child->getName() + "_" +
        // boost::lexical_cast<std::string>(count));
        const std::string childName =
            child->getName() + "_" + boost::lexical_cast<std::string>(count);
        API::AnalysisDataService::Instance().addOrReplace(childName, child);
        count++;
      }
    }

    childWsNames = group->getNames();
    count = 1;
    for (auto childWsName = childWsNames.begin();
         childWsName != childWsNames.end(); ++childWsName) {
      Workspace_sptr childWs = group->getItem(*childWsName);
      std::string outWsPropName =
          "OutputWorkspace_" + boost::lexical_cast<std::string>(count);
      ++count;
      declareProperty(new WorkspaceProperty<Workspace>(
          outWsPropName, *childWsName, Direction::Output));
      setProperty(outWsPropName, childWs);
    }
  }

  // Clean up.
  if (tempWs) {
    Algorithm_sptr alg =
        AlgorithmManager::Instance().createUnmanaged("DeleteWorkspace");
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
API::IAlgorithm_sptr Load::createLoader(const double startProgress,
                                        const double endProgress,
                                        const bool logging) const {
  std::string name = getPropertyValue("LoaderName");
  int version = getProperty("LoaderVersion");
  API::IAlgorithm_sptr loader =
      API::AlgorithmManager::Instance().createUnmanaged(name, version);
  loader->initialize();
  if (!loader) {
    throw std::runtime_error("Cannot create loader for \"" +
                             getPropertyValue("Filename") + "\"");
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
void Load::setUpLoader(API::IAlgorithm_sptr &loader, const double startProgress,
                       const double endProgress, const bool logging) const {
  // Set as a child so that we are in control of output storage
  loader->setChild(true);
  loader->setLogging(logging);
  // If output workspaces are nameless, give them a temporary name to satisfy
  // validator
  const std::vector<Property *> &props = loader->getProperties();
  for (unsigned int i = 0; i < props.size(); ++i) {
    auto wsProp = dynamic_cast<IWorkspaceProperty *>(props[i]);

    if (wsProp && !wsProp->isOptional() &&
        props[i]->direction() == Direction::Output) {
      if (props[i]->value().empty())
        props[i]->setValue("LoadChildWorkspace");
    }
  }
  if (startProgress >= 0. && endProgress > startProgress && endProgress <= 1.) {
    loader->addObserver(this->progressObserver());
    setChildStartProgress(startProgress);
    setChildEndProgress(endProgress);
  }
}

/**
* Set the output workspace(s) if the load's return workspace has type
* API::Workspace
* @param loader :: Shared pointer to load algorithm
*/
void Load::setOutputWorkspace(const API::IAlgorithm_sptr &loader) {
  // Go through each OutputWorkspace property and check whether we need to make
  // a counterpart here
  const std::vector<Property *> &loaderProps = loader->getProperties();
  const size_t count = loader->propertyCount();
  for (size_t i = 0; i < count; ++i) {
    Property *prop = loaderProps[i];
    if (dynamic_cast<IWorkspaceProperty *>(prop) &&
        prop->direction() == Direction::Output) {
      const std::string &name = prop->name();
      if (!this->existsProperty(name)) {
        declareProperty(new WorkspaceProperty<Workspace>(
            name, loader->getPropertyValue(name), Direction::Output));
      }
      Workspace_sptr wkspace = getOutputWorkspace(name, loader);
      setProperty(name, wkspace);
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
API::Workspace_sptr
Load::getOutputWorkspace(const std::string &propName,
                         const API::IAlgorithm_sptr &loader) const {
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

  // Just workspace?
  try {
    Workspace_sptr childWS = loader->getProperty(propName);
    return childWS;
  } catch (std::runtime_error &) {
  }

  g_log.debug() << "Workspace property " << propName
                << " did not return to MatrixWorkspace, EventWorkspace, "
                   "IMDEventWorkspace, IMDWorkspace" << std::endl;
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
API::Workspace_sptr Load::loadFileToWs(const std::string &fileName,
                                       const std::string &wsName) {
  Mantid::API::IAlgorithm_sptr loadAlg = createChildAlgorithm("Load", 1);

  // Get the list properties for the concrete loader load algorithm
  const std::vector<Kernel::Property *> &props = getProperties();

  // Loop through and set the properties on the Child Algorithm
  std::vector<Kernel::Property *>::const_iterator prop = props.begin();
  for (; prop != props.end(); ++prop) {
    const std::string &propName = (*prop)->name();

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
API::Workspace_sptr Load::plusWs(Workspace_sptr ws1, Workspace_sptr ws2) {
  WorkspaceGroup_sptr group1 = boost::dynamic_pointer_cast<WorkspaceGroup>(ws1);
  WorkspaceGroup_sptr group2 = boost::dynamic_pointer_cast<WorkspaceGroup>(ws2);

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

    for (; group1ChildWsName != group1ChildWsNames.end();
         ++group1ChildWsName, ++group2ChildWsName) {
      Workspace_sptr group1ChildWs = group1->getItem(*group1ChildWsName);
      Workspace_sptr group2ChildWs = group2->getItem(*group2ChildWsName);

      Mantid::API::IAlgorithm_sptr plusAlg = createChildAlgorithm("Plus", 1);
      plusAlg->setProperty<Workspace_sptr>("LHSWorkspace", group1ChildWs);
      plusAlg->setProperty<Workspace_sptr>("RHSWorkspace", group2ChildWs);
      plusAlg->setProperty<Workspace_sptr>("OutputWorkspace", group1ChildWs);
      plusAlg->executeAsChildAlg();
    }
  } else if (!group1 && !group2) {
    Mantid::API::IAlgorithm_sptr plusAlg = createChildAlgorithm("Plus", 1);
    plusAlg->setProperty<Workspace_sptr>("LHSWorkspace", ws1);
    plusAlg->setProperty<Workspace_sptr>("RHSWorkspace", ws2);
    plusAlg->setProperty<Workspace_sptr>("OutputWorkspace", ws1);
    plusAlg->executeAsChildAlg();
  } else {
    throw std::runtime_error(
        "Unable to add a group workspace to a non-group workspace");
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
API::WorkspaceGroup_sptr
Load::groupWsList(const std::vector<API::Workspace_sptr> &wsList) {
  WorkspaceGroup_sptr group = WorkspaceGroup_sptr(new WorkspaceGroup);

  for (auto ws = wsList.begin(); ws != wsList.end(); ++ws) {
    WorkspaceGroup_sptr isGroup =
        boost::dynamic_pointer_cast<WorkspaceGroup>(*ws);
    // If the ws to add is already a group, then add its children individually.
    if (isGroup) {
      std::vector<std::string> childrenNames = isGroup->getNames();
      size_t count = 1;
      for (auto childName = childrenNames.begin();
           childName != childrenNames.end(); ++childName, ++count) {
        Workspace_sptr childWs = isGroup->getItem(*childName);
        isGroup->remove(*childName);
        // childWs->setName(isGroup->getName() + "_" +
        // boost::lexical_cast<std::string>(count));
        group->addWorkspace(childWs);
      }

      // Remove the old group from the ADS
      AnalysisDataService::Instance().remove(isGroup->getName());
    } else {
      group->addWorkspace(*ws);
    }
  }

  return group;
}

} // namespace DataHandling
} // namespace Mantid
