// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VATES_API_SAVE_MD_WORKSPACE_TO_VTK_H_
#define VATES_API_SAVE_MD_WORKSPACE_TO_VTK_H_
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include <map>

namespace Mantid {
namespace VATES {

/** SaveMDWorkspaceToVTK : Defines an algorithm to save MDWorkspaces
to a VTK compatible format in order to load them into ParaView.
MDHistoWorkspaces are stored in the vts and MDEvent Workspaces
are stored in the vtu file format. Note that currently only 3D workspaces
are supported.
*/
class SaveMDWorkspaceToVTKImpl;

class DLLExport SaveMDWorkspaceToVTK : public Mantid::API::Algorithm {
public:
  SaveMDWorkspaceToVTK();
  ~SaveMDWorkspaceToVTK() override;
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
  std::unique_ptr<SaveMDWorkspaceToVTKImpl> saver;
};
} // namespace VATES
} // namespace Mantid
#endif
