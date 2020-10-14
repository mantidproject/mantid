// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/BoxControllerSettingsAlgorithm.h"
#include "MantidMDAlgorithms/ConvertToMDParent.h"
#include "MantidMDAlgorithms/MDWSDescription.h"

#include "MantidKernel/DeltaEMode.h"

#include <boost/scoped_ptr.hpp>

namespace Mantid {
namespace MDAlgorithms {

/** ConvertToMD :
   *  Transform a workspace into MD workspace with components defined by user.
   *
   * Gateway for number of ChildTransformations, provided by ConvertToMD
   factory.
   * Intended to cover wide range of cases;
   *
   * The description of the algorithm is available at:
   <http://www.mantidproject.org/ConvertToMD>
   * The detailed description of the algorithm is provided at:
   <http://www.mantidproject.org/Writing_custom_ConvertTo_MD_transformation>

   * @date 11-10-2011
*/

/// Convert to MD Events class itself:
class DLLExport ConvertToMD : public ConvertToMDParent {
public:
  /// Algorithm's name for identification
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Create a MDEventWorkspace with selected dimensions, e.g. the "
           "reciprocal space of momentums (Qx, Qy, Qz) or momentums modules "
           "mod(Q), energy transfer dE if available and any other user "
           "specified log values which can be treated as dimensions.";
  }

  /// Algorithm's version for identification
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"ConvertToDiffractionMDWorkspace", "ConvertToMDMinMaxGlobal",
            "ConvertToMDMinMaxLocal", "CreateMDWorkspace",
            "SetSpecialCoordinates"};
  }

private:
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;
  void init() override;
  /// progress reporter
  boost::scoped_ptr<API::Progress> m_Progress;

  void setupFileBackend(const std::string &filebackPath,
                        const API::IMDEventWorkspace_sptr &outputWS);

  //------------------------------------------------------------------------------------------------------------------------------------------
protected: // for testing, otherwise private:
  /// pointer to the input workspace;
  Mantid::API::MatrixWorkspace_sptr m_InWS2D;
  // TODO: This will eventually go. ///The pointer to class which keeps output
  // MD workspace and is responsible for adding data to N-dimensional workspace;
  std::shared_ptr<MDAlgorithms::MDEventWSWrapper> m_OutWSWrapper;

  // Workflow helpers:
  /**Check if target workspace new or existing one and we need to create new
   * workspace*/
  bool doWeNeedNewTargetWorkspace(const API::IMDEventWorkspace_sptr &spws);
  /**Create new MD workspace using existing parameters for algorithm */
  API::IMDEventWorkspace_sptr
  createNewMDWorkspace(const MDAlgorithms::MDWSDescription &targWSDescr,
                       const bool filebackend, const std::string &filename);

  bool buildTargetWSDescription(const API::IMDEventWorkspace_sptr &spws,
                                const std::string &QModReq,
                                const std::string &dEModReq,
                                const std::vector<std::string> &otherDimNames,
                                std::vector<double> &dimMin,
                                std::vector<double> &dimMax,
                                const std::string &QFrame,
                                const std::string &convertTo_,
                                MDAlgorithms::MDWSDescription &targWSDescr);

  /// par of store metadata routine which generate metadata necessary for
  /// initializing ConvertToMD plugin
  void addExperimentInfo(API::IMDEventWorkspace_sptr &mdEventWS,
                         MDAlgorithms::MDWSDescription &targWSDescr) const;

  /// Store metadata and set some metadata, needed for plugin to run on the
  /// target workspace description
  void copyMetaData(API::IMDEventWorkspace_sptr &mdEventWS) const;

  void findMinMax(const Mantid::API::MatrixWorkspace_sptr &inWS,
                  const std::string &QMode, const std::string &dEMode,
                  const std::string &QFrame, const std::string &ConvertTo,
                  const std::vector<std::string> &otherDim,
                  std::vector<double> &minVal, std::vector<double> &maxVal);

  /// Sets up the top level splitting, i.e. of level 0, for the box controller
  void setupTopLevelSplitting(const Mantid::API::BoxController_sptr &bc);
};

} // namespace MDAlgorithms
} // namespace Mantid
