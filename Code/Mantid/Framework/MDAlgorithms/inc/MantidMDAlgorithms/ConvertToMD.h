#ifndef MANTID_MDALGORITHMS_CONVERT_TO_MDALGORITHMS_H_
#define MANTID_MDALGORITHMS_CONVERT_TO_MDALGORITHMS_H_

#include "MantidMDAlgorithms/BoxControllerSettingsAlgorithm.h"
#include "MantidMDAlgorithms/ConvertToMDParent.h"
#include "MantidMDAlgorithms/MDWSDescription.h"

#include "MantidKernel/DeltaEMode.h"

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

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

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

    File change history is stored at:
<https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

/// Convert to MD Events class itself:
class DLLExport ConvertToMD : public ConvertToMDParent {
public:
  ConvertToMD();
  ~ConvertToMD();

  /// Algorithm's name for identification
  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Create a MDEventWorkspace with selected dimensions, e.g. the "
           "reciprocal space of momentums (Qx, Qy, Qz) or momentums modules "
           "mod(Q), energy transfer dE if available and any other user "
           "specified log values which can be treated as dimensions.";
  }

  /// Algorithm's version for identification
  virtual int version() const;

private:
  std::map<std::string, std::string> validateInputs();
  void exec();
  void init();
  /// progress reporter
  boost::scoped_ptr<API::Progress> m_Progress;

  //------------------------------------------------------------------------------------------------------------------------------------------
protected: // for testing, otherwise private:
  /// pointer to the input workspace;
  Mantid::API::MatrixWorkspace_sptr m_InWS2D;
  // TODO: This will eventually go. ///The pointer to class which keeps output
  // MD workspace and is responsible for adding data to N-dimensional workspace;
  boost::shared_ptr<MDAlgorithms::MDEventWSWrapper> m_OutWSWrapper;

  // Workflow helpers:
  /**Check if target workspace new or existing one and we need to create new
   * workspace*/
  bool doWeNeedNewTargetWorkspace(API::IMDEventWorkspace_sptr spws);
  /**Create new MD workspace using existing parameters for algorithm */
  API::IMDEventWorkspace_sptr
  createNewMDWorkspace(const MDAlgorithms::MDWSDescription &NewMDWSDescription);

  bool buildTargetWSDescription(API::IMDEventWorkspace_sptr spws,
                                const std::string &Q_mod_req,
                                const std::string &dEModeRequested,
                                const std::vector<std::string> &other_dim_names,
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
};

} // namespace Mantid
} // namespace MDAlgorithms

#endif /* MANTID_MDALGORITHMS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_ */
