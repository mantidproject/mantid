#ifndef VATES_API_SAVE_MD_WORKSPACE_TO_VTK_IMPL_H_
#define VATES_API_SAVE_MD_WORKSPACE_TO_VTK_IMPL_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/ThresholdRange.h"

#include <vtkXMLWriter.h>
#include <vtkSmartPointer.h>
#include <map>

namespace Mantid {
namespace VATES {

// Forward declaration
class MDLoadingPresenter;
class vtkDataSetFactory;

class DLLExport SaveMDWorkspaceToVTKImpl {
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
  vtkSmartPointer<vtkDataSet> getDataSetWithOrthogonalCorrection(vtkSmartPointer<vtkDataSet> dataSet,
                                                                 MDLoadingPresenter* presenter,
                                                                 Mantid::API::IMDWorkspace_sptr workspace,
                                                                 bool isHistoWorkspace) const;
  std::unique_ptr<vtkDataSetFactory> getDataSetFactoryChain(bool isHistWorkspace, ThresholdRange_scptr thresholdRange, VisualNormalization normalization, double time) const;
  std::unique_ptr<MDLoadingPresenter> getPresenter(bool isHistoWorkspace, Mantid::API::IMDWorkspace_sptr workspace, int recursionDepth) const;
};

}
}


#endif
