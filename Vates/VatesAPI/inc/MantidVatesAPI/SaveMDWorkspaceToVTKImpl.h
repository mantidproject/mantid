#ifndef VATES_API_SAVE_MD_WORKSPACE_TO_VTK_IMPL_H_
#define VATES_API_SAVE_MD_WORKSPACE_TO_VTK_IMPL_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/ThresholdRange.h"


#include <vtkDataSet.h>
#include <vtkXMLWriter.h>
#include <vtkSmartPointer.h>
#include <map>

namespace Mantid {
namespace VATES {

class MDLoadingPresenter;

class SaveMDWorkspaceToVTKImpl {
public:
  SaveMDWorkspaceToVTKImpl();
  ~SaveMDWorkspaceToVTKImpl() {}
  void saveMDWorkspace(Mantid::API::IMDWorkspace_sptr workspace,
                            std::string filename,
                            VisualNormalization normalization,
                            ThresholdRange_scptr thresholdRange,
                            int recursionDepth) const;

  const static std::string structuredGridExtension;
  const static std::string unstructuredGridExtension;

  std::vector<std::string> getAllowedNormalizationsInStringRepresentation() const;
  std::vector<std::string> getAllowedThresholdsInStringRepresentation() const;
  VisualNormalization translateStringToVisualNormalization(const std::string normalization) const;
  ThresholdRange_scptr translateStringToThresholdRange(const std::string thresholdRange) const;

  bool is4DWorkspace(Mantid::API::IMDWorkspace_sptr workspace) const;

private:
  std::map<std::string, VisualNormalization> m_normalizations;
  std::vector<std::string> m_thresholds;

  void setupMembers();
  int writeDataSetToVTKFile(vtkXMLWriter* writer, vtkDataSet* dataSet, std::string filename) const;
  double selectTimeSliceValue(Mantid::API::IMDWorkspace_sptr workspace) const;
  std::string getFullFilename(std::string filename, bool isHistoWorkspace) const;
  vtkSmartPointer<vtkXMLWriter> getXMLWriter(bool isHistoWorkspace) const;
  vtkSmartPointer<vtkDataSet> getDataSetWithOrthogonalCorrection(vtkSmartPointer<vtkDataSet> dataSet, MDLoadingPresenter* presenter, bool isHistoWorkspace) const;
};

}
}


#endif
