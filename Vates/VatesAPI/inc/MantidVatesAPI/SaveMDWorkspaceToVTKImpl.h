#ifndef VATES_API_SAVE_MD_WORKSPACE_TO_VTK_IMPL_H_
#define VATES_API_SAVE_MD_WORKSPACE_TO_VTK_IMPL_H_

#include "MantidAPI/IMDHistoWorkspace_fwd.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/ThresholdRange.h"

#include <vtkXMLWriter.h>
#include <vtkDataSet.h>
#include <map>

namespace Mantid {
namespace VATES {

class SaveMDWorkspaceToVTKImpl {
public:
  SaveMDWorkspaceToVTKImpl();
  ~SaveMDWorkspaceToVTKImpl() {};
  void saveMDHistoWorkspace(Mantid::API::IMDHistoWorkspace_sptr histoWS,
                            std::string filename,
                            VisualNormalization normalization,
                            ThresholdRange_scptr thresholdRange) const;
  void saveMDEventWorkspace(Mantid::API::IMDEventWorkspace_sptr eventWS,
                            std::string filename,
                            VisualNormalization normalization,
                            ThresholdRange_scptr thresholdRange) const;

  std::vector<std::string> getAllowedNormalizationsInStringRepresentation() const;
  std::vector<std::string> getAllowedThresholdsInStringRepresentation() const;
  VisualNormalization translateStringToVisualNormalization(const std::string normalization) const;
  ThresholdRange_scptr translateStringToThresholdRange(const std::string thresholdRange) const;

  bool is4DWorkspace(Mantid::API::IMDWorkspace_sptr workspace) const;

private:
  const static std::string structuredGridExtension;
  const static std::string unstructuredGridExtension;

  std::map<std::string, VisualNormalization> m_normalizations;
  std::vector<std::string> m_thresholds;

  void setupMembers();
  void writeDataSetToVTKFile(vtkXMLWriter* writer, vtkDataSet* dataSet, std::string filename) const;
  double selectTimeSliceValue(Mantid::API::IMDWorkspace_sptr workspace) const;
};

}
}


#endif
