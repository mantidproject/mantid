#ifndef MANTID_INDIRECTFITDATACREATIONHELPER_H_
#define MANTID_INDIRECTFITDATACREATIONHELPER_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <boost/variant.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>

namespace Mantid {
namespace IndirectFitDataCreationHelper {

/// Functions used in the creation of workspaces
Mantid::API::MatrixWorkspace_sptr createWorkspace(int const &numberOfSpectra);
Mantid::API::MatrixWorkspace_sptr createInstrumentWorkspace(int const &xLength,
                                                            int const &yLength);
Mantid::API::MatrixWorkspace_sptr
setWorkspaceEFixed(Mantid::API::MatrixWorkspace_sptr workspace,
                   int const &xLength);
Mantid::API::MatrixWorkspace_sptr
setWorkspaceBinEdges(Mantid::API::MatrixWorkspace_sptr workspace,
                     int const &yLength,
                     Mantid::HistogramData::BinEdges const &binEdges);
Mantid::API::MatrixWorkspace_sptr
setWorkspaceBinEdges(Mantid::API::MatrixWorkspace_sptr workspace,
                     int const &xLength, int const &yLength);
Mantid::API::MatrixWorkspace_sptr
setWorkspaceProperties(Mantid::API::MatrixWorkspace_sptr workspace,
                       int const &xLength, int const &yLength);
Mantid::API::MatrixWorkspace_sptr
createWorkspaceWithInstrument(int const &xLength, int const &yLength);

/// Simple class to set up the ADS with the configuration required
struct SetUpADSWithWorkspace {

  template <typename T>
  SetUpADSWithWorkspace(std::string const &inputWSName, T const &workspace) {
    Mantid::API::AnalysisDataService::Instance().addOrReplace(inputWSName,
                                                              workspace);
  }

  template <typename T>
  void addOrReplace(std::string const &workspaceName, T const &workspace) {
    Mantid::API::AnalysisDataService::Instance().addOrReplace(workspaceName,
                                                              workspace);
  }

  bool doesExist(std::string const &workspaceName) {
    return Mantid::API::AnalysisDataService::Instance().doesExist(
        workspaceName);
  }

  Mantid::API::MatrixWorkspace_sptr
  retrieveWorkspace(std::string const &workspaceName) {
    return boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve(workspaceName));
  }

  ~SetUpADSWithWorkspace() {
    Mantid::API::AnalysisDataService::Instance().clear();
  }
};

/// This is used to compare Spectra which is implemented as a boost::variant
struct AreSpectraEqual : public boost::static_visitor<bool> {

  template <typename T, typename U>
  bool operator()(const T &, const U &) const {
    return false; // cannot compare different types
  }

  template <typename T> bool operator()(const T &lhs, const T &rhs) const {
    return lhs == rhs;
  }
};

} // namespace IndirectFitDataCreationHelper
} // namespace Mantid

#endif
