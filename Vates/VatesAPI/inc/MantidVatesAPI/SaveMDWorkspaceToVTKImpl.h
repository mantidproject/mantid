#ifndef VATES_API_SAVE_MD_WORKSPACE_TO_VTK_IMPL_H_
#define VATES_API_SAVE_MD_WORKSPACE_TO_VTK_IMPL_H_

#include "MantidAPI/IMDHistoWorkspace_fwd.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidVatesAPI/Normalization.h"

#include <vtkXMLWriter.h>
#include <map>

namespace Mantid {
namespace VATES {

class SaveMDWorkspaceToVTKImpl {
public:
  SaveMDWorkspaceToVTKImpl();
  ~SaveMDWorkspaceToVTKImpl() {};
  void saveMDHistoWorkspace(Mantid::API::IMDHistoWorkspace_sptr histoWS,
                            std::string filename,
                            VisualNormalization normalization) const;
  void saveMDEventWorkspace(Mantid::API::IMDEventWorkspace_sptr eventWS,
                            std::string filename,
                            VisualNormalization normalization) const;
  std::vector<std::string> getAllowedNormalizationsInStringRepresentation() const;
  VisualNormalization translateStringToVisualNormalization(const std::string normalization) const;

private:
  const static std::string structuredGridExtension;
  const static std::string unstructuredGridExtension;

  std::map<std::string, VisualNormalization> m_normalizations;
  void setupNormalization();
  void writeDataSetToVTKFile(vtkXMLWriter* writer, vtkDataSet* dataSet, std::string filename) const;
};

}
}


#endif