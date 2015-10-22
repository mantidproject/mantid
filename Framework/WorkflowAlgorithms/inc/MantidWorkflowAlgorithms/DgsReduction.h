#ifndef MANTID_WORKFLOWALGORITHMS_DGSREDUCTION_H_
#define MANTID_WORKFLOWALGORITHMS_DGSREDUCTION_H_

#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/System.h"
#include "MantidAPI/DataProcessorAlgorithm.h"

namespace Mantid {
namespace WorkflowAlgorithms {

/** DgsReduction : This is the top-level workflow algorithm for controlling
 * direct geometry spectrometer reduction.

@date 2012-06-06

Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport DgsReduction : public API::DataProcessorAlgorithm {
public:
  DgsReduction();
  virtual ~DgsReduction();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Top-level workflow algorithm for DGS reduction.";
  }

  virtual int version() const;
  virtual const std::string category() const;

private:
  void init();
  void exec();
  API::Workspace_sptr loadInputData(const std::string prop,
                                    const bool mustLoad = true);
  API::MatrixWorkspace_sptr loadGroupingFile(const std::string prop);
  API::MatrixWorkspace_sptr loadHardMask();
  double getParameter(std::string algParam, API::MatrixWorkspace_sptr ws,
                      std::string altParam);

  boost::shared_ptr<Kernel::PropertyManager> reductionManager;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /* MANTID_WORKFLOWALGORITHMS_DGSREDUCTION_H_ */
