#ifndef MANTID_WORKFLOWALGORITHMS_DGSDIAGNOSE_H_
#define MANTID_WORKFLOWALGORITHMS_DGSDIAGNOSE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace WorkflowAlgorithms {

/** DgsDiagnose : This algorithm constructs all of the necessary workspaces
 * for performing detector diagnostics and then runs the diagnostic
 * algorithm.

Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

@date 2012-02-08

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
class DLLExport DgsDiagnose : public API::Algorithm {
public:
  DgsDiagnose();
  virtual ~DgsDiagnose();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Setup and run DetectorDiagnostic.";
  }

  virtual int version() const;
  virtual const std::string category() const;

private:
  void init();
  void exec();
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /* MANTID_WORKFLOWALGORITHMS_DGSDIAGNOSE_H_ */
