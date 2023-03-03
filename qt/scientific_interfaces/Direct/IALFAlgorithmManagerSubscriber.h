// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace MantidQt::CustomInterfaces {

class MANTIDQT_DIRECT_DLL IALFAlgorithmManagerSubscriber {

public:
  virtual ~IALFAlgorithmManagerSubscriber() = default;

  virtual void notifyAlgorithmError(std::string const &message) = 0;

  // Algorithm notifiers used when loading and normalising the Sample
  virtual void notifyLoadComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) { (void)workspace; };
  virtual void notifyNormaliseByCurrentComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) {
    (void)workspace;
  };
  virtual void notifyRebinToWorkspaceComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) { (void)workspace; };
  virtual void notifyDivideComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) { (void)workspace; };
  virtual void notifyReplaceSpecialValuesComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) {
    (void)workspace;
  };
  virtual void notifyConvertUnitsComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) { (void)workspace; };

  // Algorithm notifiers used when producing an Out of plane angle workspace
  virtual void notifyCreateWorkspaceComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) { (void)workspace; };
  virtual void notifyScaleXComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) { (void)workspace; };
  virtual void notifyRebunchComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) { (void)workspace; };

  // Algorithm notifiers used when fitting the extracted Out of plane angle workspace
  virtual void notifyCropWorkspaceComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) { (void)workspace; };
  virtual void notifyFitComplete(Mantid::API::MatrixWorkspace_sptr const &workspace,
                                 Mantid::API::IFunction_sptr const &function, std::string const &fitStatus) {
    (void)workspace;
    (void)function;
    (void)fitStatus;
  };
};

} // namespace MantidQt::CustomInterfaces
