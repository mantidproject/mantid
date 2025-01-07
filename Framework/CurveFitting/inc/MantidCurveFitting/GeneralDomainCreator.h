// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IDomainCreator.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {

namespace API {
class IFunctionGeneral;
class Column;
class ITableWorkspace;
} // namespace API

namespace CurveFitting {

/** GeneralDomainCreator:

    GeneralDomainCreator creates a FunctionDomainGeneral which is
    used with IFunctionGeneral.

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

class MANTID_CURVEFITTING_DLL GeneralDomainCreator : public API::IDomainCreator {
public:
  GeneralDomainCreator(const API::IFunctionGeneral &fun, Kernel::IPropertyManager &manager,
                       const std::string &workspacePropertyName);

  void createDomain(std::shared_ptr<API::FunctionDomain> &domain, std::shared_ptr<API::FunctionValues> &values,
                    size_t i0 = 0) override;

  API::Workspace_sptr
  createOutputWorkspace(const std::string &baseName, API::IFunction_sptr function,
                        std::shared_ptr<API::FunctionDomain> domain, std::shared_ptr<API::FunctionValues> values,
                        const std::string &outputWorkspacePropertyName = "OutputWorkspace") override;

  size_t getDomainSize() const override;

  void declareDatasetProperties(const std::string &suffix = "", bool addProp = true) override;

private:
  /// Retrive the input workspace from the property manager.
  std::shared_ptr<API::ITableWorkspace> getInputWorkspace() const;
  // Names of additional properties
  /// Property names for columns in a TableWorkspace to be passed to
  /// the domain.
  std::vector<std::string> m_domainColumnNames;
  /// Property names for columns in a TableWorkspace to be used as the data
  /// to fit to.
  std::vector<std::string> m_dataColumnNames;
  /// Property names for columns in a TableWorkspace to be used as the
  /// fitting weights.
  std::vector<std::string> m_weightsColumnNames;
  /// Default number of values
  size_t m_defaultValuesSize;
};

} // namespace CurveFitting
} // namespace Mantid
