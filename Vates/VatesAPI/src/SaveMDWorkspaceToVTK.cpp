#include "MantidVatesAPI/SaveMDWorkspaceToVTK.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ListValidator.h"
#include "boost/pointer_cast.hpp"
#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/SaveMDWorkspaceToVTKImpl.h"

namespace Mantid {
namespace VATES{

  DECLARE_ALGORITHM(SaveMDWorkspaceToVTK)

  const std::string SaveMDWorkspaceToVTK::name() const {
  return "SaveMDWorkspaceToVTK";
  }

  int SaveMDWorkspaceToVTK::version() const {
    return 1;
  }

  const std::string SaveMDWorkspaceToVTK::category() const {
    return "MDAlgorithms";
  }

  const std::string SaveMDWorkspaceToVTK::summary() const {
    std::string summary = "Saves MD workspaces to VTK file types which can be loaded by ParaView./n MDHisto workspaces are saved as .vts files and MDEvent workspaces as .vtu files.";
    return summary;
  }

  void SaveMDWorkspaceToVTK::init() {
    declareProperty(
      new Mantid::API::WorkspaceProperty<Mantid::API::IMDWorkspace>("InputWorkspace", "", Mantid::Kernel::Direction::Input),
      "MDWorkspace to save/export");

    std::vector<std::string> extensions;
    declareProperty(new Mantid::API::FileProperty("Filename", "", Mantid::API::FileProperty::Save,
      extensions, Mantid::Kernel::Direction::Input),
      "Save location.");
    auto normalizations = pimpl->getAllowedNormalizationsInStringRepresentation();
#if 1
    declareProperty(
      "Visual Normalization", "AutoSelect",
      boost::make_shared<Mantid::Kernel::StringListValidator>(normalizations),
      "TThe visual normalization option. The automatic default will choose a normalization based on your data type and instrument.");
#endif
    // TODO: Add threshold property
    // TODO: Add range selection
  }

  void SaveMDWorkspaceToVTK::exec() {
    Mantid::API::IMDWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
    std::string filename = this->getProperty("Filename");

    auto normalization = Mantid::VATES::VisualNormalization::AutoSelect;


    if (auto histoWS = boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(inputWS)) {
      pimpl->saveMDHistoWorkspace(histoWS, filename, normalization);
    }
    else {
      auto eventWS = boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(inputWS);
      pimpl->saveMDEventWorkspace(eventWS, filename, normalization);
    }
  }

  std::map < std::string, std::string> SaveMDWorkspaceToVTK::validateInputs() {
    std::map<std::string, std::string> errorMessage;

    // Check for input workspace type
    Mantid::API::IMDWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
    if (!boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(inputWS) ||
        !boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(inputWS)) {
      errorMessage.emplace("InputWorkspace", "You can only save MDHisto or MDEvent workspaces.");
    }

    // Check for file location
    return errorMessage;
  }
}
}