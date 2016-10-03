#ifndef VATES_API_SAVE_MD_WORKSPACE_TO_VTK_IMPL_H_
#define VATES_API_SAVE_MD_WORKSPACE_TO_VTK_IMPL_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/ThresholdRange.h"

#include <map>
#include <vtkSmartPointer.h>
#include <vtkXMLWriter.h>

namespace Mantid {
namespace VATES {

// Forward declaration
class MDLoadingPresenter;
class vtkDataSetFactory;

/** SaveMDWorkspaceToVTKImpl : Defines the underlying functionaity of
    SaveMDDdWorkspaceToVTK. MDHistoWorkspaces are stored in the vts and
    MDEvent Workspaces are stored in the vtu file format.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport SaveMDWorkspaceToVTKImpl {
public:
  SaveMDWorkspaceToVTKImpl();
  ~SaveMDWorkspaceToVTKImpl() {}
  void saveMDWorkspace(
      Mantid::API::IMDWorkspace_sptr workspace, const std::string &filename,
      VisualNormalization normalization, ThresholdRange_scptr thresholdRange,
      int recursionDepth, const std::string &compressorType) const;

  const static std::string structuredGridExtension;
  const static std::string unstructuredGridExtension;

  std::vector<std::string>
  getAllowedNormalizationsInStringRepresentation() const;
  std::vector<std::string> getAllowedThresholdsInStringRepresentation() const;
  VisualNormalization
  translateStringToVisualNormalization(const std::string normalization) const;
  ThresholdRange_scptr
  translateStringToThresholdRange(const std::string thresholdRange) const;

  bool is3DWorkspace(Mantid::API::IMDWorkspace_sptr workspace) const;

private:
  std::map<std::string, VisualNormalization> m_normalizations;
  std::vector<std::string> m_thresholds;

  void setupMembers();
  bool is4DWorkspace(Mantid::API::IMDWorkspace_sptr workspace) const;
  int writeDataSetToVTKFile(vtkXMLWriter *writer, vtkDataSet *dataSet,
                            const std::string &filename,
                            vtkXMLWriter::CompressorType compressor) const;
  double selectTimeSliceValue(Mantid::API::IMDWorkspace_sptr workspace) const;
  std::string getFullFilename(std::string filename,
                              bool isHistoWorkspace) const;
  vtkSmartPointer<vtkXMLWriter> getXMLWriter(bool isHistoWorkspace) const;
  vtkSmartPointer<vtkDataSet> getDataSetWithOrthogonalCorrection(
      vtkSmartPointer<vtkDataSet> dataSet, MDLoadingPresenter *presenter,
      Mantid::API::IMDWorkspace_sptr workspace, bool isHistoWorkspace) const;
  std::unique_ptr<vtkDataSetFactory>
  getDataSetFactoryChain(bool isHistWorkspace,
                         ThresholdRange_scptr thresholdRange,
                         VisualNormalization normalization, double time) const;
  std::unique_ptr<MDLoadingPresenter>
  getPresenter(bool isHistoWorkspace, Mantid::API::IMDWorkspace_sptr workspace,
               int recursionDepth) const;
};
}
}

#endif
