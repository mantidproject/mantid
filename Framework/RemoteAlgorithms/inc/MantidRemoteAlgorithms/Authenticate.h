// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"

namespace Mantid {
namespace RemoteAlgorithms {
/*** Authenticate to the remote compute resource.

    Input Properties:
    <UL>
    <LI> ComputeResource  - The name of the compute resource that will execute
   the job </LI>
    <LI> UserName         - User name on the compute resource </LI>
    <LI> Password         - Password for the compute resource </LI>
    </UL>

    Output Properties: None.
    If the authentication is successfull, a cookie is received that is stored
   internally and
    re-used for all subsequent interactions with the compute resource.


    @author Ross Miller, ORNL
    @date 04/30/2013
    */

class DLLExport Authenticate : public Mantid::API::Algorithm, public API::DeprecatedAlgorithm {
public:
  /// Default constructor
  Authenticate();

  /// Algorithm's name
  const std::string name() const override { return "Authenticate"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Authenticate to the remote compute resource."; }

  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "Remote"; }

private:
  void init() override;
  /// Execution code
  void exec() override;
};

} // end namespace RemoteAlgorithms
} // end namespace Mantid