#ifndef SQW_LOADING_PRESENTER_H_
#define SQW_LOADING_PRESENTER_H_

#include "MantidVatesAPI/MDEWLoadingPresenter.h"

namespace Mantid {
namespace VATES {
/**
@class SQWLoadingPresenter
MVP loading presenter for .*sqw file types.
@author Owen Arnold, Tessella plc
@date 16/08/2011

Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MDLoadingView;
class DLLExport SQWLoadingPresenter : public MDEWLoadingPresenter {
public:
  SQWLoadingPresenter(std::unique_ptr<MDLoadingView> view,
                      const std::string &fileName);
  vtkSmartPointer<vtkDataSet>
  execute(vtkDataSetFactory *factory, ProgressAction &rebinningProgressUpdate,
          ProgressAction &drawingProgressUpdate) override;
  void extractMetadata(const Mantid::API::IMDEventWorkspace &eventWs) override;
  void executeLoadMetadata() override;
  ~SQWLoadingPresenter() override;
  bool canReadFile() const override;
  std::string getWorkspaceTypeName() override;

private:
  const std::string m_filename;
  std::string m_wsTypeName;
};
}
}

#endif
