#ifndef MANTID_MDALGORITHMS_COMPAREMDWORKSPACES_H_
#define MANTID_MDALGORITHMS_COMPAREMDWORKSPACES_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidMDEvents/MDEventWorkspace.h"

namespace Mantid {
namespace MDAlgorithms {

/** Compare two MDWorkspaces for equality.

  @date 2012-01-19

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
class DLLExport CompareMDWorkspaces : public API::Algorithm {
public:
  CompareMDWorkspaces();
  virtual ~CompareMDWorkspaces();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Compare two MDWorkspaces for equality.";
  }

  virtual int version() const;
  virtual const std::string category() const;

private:
  void init();
  void exec();
  void doComparison();
  void compareMDGeometry(Mantid::API::IMDWorkspace_sptr ws1,
                         Mantid::API::IMDWorkspace_sptr ws2);
  void compareMDHistoWorkspaces(Mantid::MDEvents::MDHistoWorkspace_sptr ws1,
                                Mantid::MDEvents::MDHistoWorkspace_sptr ws2);

  template <typename MDE, size_t nd>
  void compareMDWorkspaces(
      typename Mantid::MDEvents::MDEventWorkspace<MDE, nd>::sptr ws);

  template <typename T> void compare(T a, T b, const std::string &message);

  template <typename T>
  inline void compareTol(T a, T b, const std::string &message);

  Mantid::API::IMDWorkspace_sptr inWS2;

  /// Result string
  std::string m_result;

  /// Tolerance
  double m_tolerance;

  /// Is CheckEvents true
  bool m_CheckEvents;

  bool m_CompareBoxID;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_COMPAREMDWORKSPACES_H_ */
