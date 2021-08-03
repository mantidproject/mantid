// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include <mutex>

namespace Mantid {
namespace DataHandling {

/**
Loads a workspace from a data file. The algorithm tries to determine the actual
type
of the file (raw, nxs, ...) and use the specialized loading algorithm to load
it.
 */
class DLLExport Load : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "Load"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Attempts to load a given file by finding an appropriate Load "
           "algorithm.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"LoadNexus", "LoadRaw", "LoadBBY"}; }

  /// Category
  const std::string category() const override { return "DataHandling"; }
  /// Aliases
  const std::string alias() const override { return "load"; }
  /// Override setPropertyValue
  void setPropertyValue(const std::string &name, const std::string &value) override;

protected:
  Parallel::ExecutionMode
  getParallelExecutionMode(const std::map<std::string, Parallel::StorageMode> &storageModes) const override;

private:
  /// This method returns shared pointer to a load algorithm which got
  /// the highest preference after file check.
  API::IAlgorithm_sptr getFileLoader(const std::string &filePath);
  void findFilenameProperty(const API::IAlgorithm_sptr &loader);
  /// Declare any additional input properties from the concrete loader
  void declareLoaderProperties(const API::IAlgorithm_sptr &loader);

  /// Initialize the static base properties
  void init() override;
  /// Execute
  void exec() override;

  /// Called when there is only one file to load.
  void loadSingleFile();
  /// Called when there are multiple files to load.
  void loadMultipleFiles();

  /// Overrides the cancel() method to call m_loader->cancel()
  void cancel() override;
  /// Create the concrete instance use for the actual loading.
  API::IAlgorithm_sptr createLoader(const double startProgress = -1.0, const double endProgress = -1.0,
                                    const bool logging = true) const;
  /// Set the loader option for use as a Child Algorithm.
  void setUpLoader(const API::IAlgorithm_sptr &loader, const double startProgress = -1.0,
                   const double endProgress = -1.0, const bool logging = true) const;
  /// Set the output properties
  void setOutputProperties(const API::IAlgorithm_sptr &loader);
  /// Retrieve a pointer to the output workspace from the Child Algorithm
  API::Workspace_sptr getOutputWorkspace(const std::string &propName, const API::IAlgorithm_sptr &loader) const;

  /// Load a file to a given workspace name.
  API::Workspace_sptr loadFileToWs(const std::string &fileName, const std::string &wsName);
  /// Plus two workspaces together, "in place".
  API::Workspace_sptr plusWs(API::Workspace_sptr ws1, const API::Workspace_sptr &ws2);
  /// Manually group workspaces.
  API::WorkspaceGroup_sptr groupWsList(const std::vector<API::Workspace_sptr> &wsList);

  /// The base properties
  std::unordered_set<std::string> m_baseProps;
  /// The actual loader
  API::IAlgorithm_sptr m_loader;
  /// The name of the property that will be passed the property from our
  /// Filename
  std::string m_filenamePropName;
  /// Mutex for temporary fix for #5963
  static std::recursive_mutex m_mutex;
};

} // namespace DataHandling
} // namespace Mantid
