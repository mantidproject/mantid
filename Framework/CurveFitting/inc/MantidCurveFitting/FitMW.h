#ifndef MANTID_CURVEFITTING_FITMW_H_
#define MANTID_CURVEFITTING_FITMW_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/IMWDomainCreator.h"
#include "MantidKernel/cow_ptr.h"

#include <boost/weak_ptr.hpp>
#include <list>

namespace Mantid {
namespace API {
class FunctionDomain;
class FunctionDomain1D;
class FunctionValues;
class MatrixWorkspace;
} // namespace API

namespace CurveFitting {

class SimpleChebfun;
/**
Creates FunctionDomain1D form a spectrum in a MatrixWorkspace.
Declares WorkspaceIndex, StartX, and EndX input properties.
Declares OutputWorkspace output property.

@author Roman Tolchenov, Tessella plc
@date 06/12/2011

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport FitMW : public IMWDomainCreator {
public:
  /// Constructor
  FitMW(Kernel::IPropertyManager *fit, const std::string &workspacePropertyName,
        DomainType domainType = Simple);
  /// Constructor
  FitMW(DomainType domainType = Simple);
  /// Declare properties that specify the dataset within the workspace to fit
  /// to.
  void declareDatasetProperties(const std::string &suffix = "",
                                bool addProp = true) override;
  /// Create a domain from the input workspace
  void createDomain(boost::shared_ptr<API::FunctionDomain> &domain,
                    boost::shared_ptr<API::FunctionValues> &values,
                    size_t i0 = 0) override;
  /// Create an output workspace.
  boost::shared_ptr<API::Workspace> createOutputWorkspace(
      const std::string &baseName, API::IFunction_sptr function,
      boost::shared_ptr<API::FunctionDomain> domain,
      boost::shared_ptr<API::FunctionValues> values,
      const std::string &outputWorkspacePropertyName) override;
  /// Set max size for Sequantial and Parallel domains
  /// @param maxSize :: Maximum size of each simple domain
  void setMaxSize(size_t maxSize) { m_maxSize = maxSize; }
  /// Set the normalisation flag
  /// @param on :: If true and the spectrum is a histogram the fitting data will
  /// be normalised
  /// by the bin width.
  void setNormalise(bool on) { m_normalise = on; }

protected:
  /// Set all parameters
  void setParameters() const override;

private:
  /// Store maxSize property name
  std::string m_maxSizePropertyName;
  /// Store Normalise property name
  std::string m_normalisePropertyName;
  /// Store the Exclude property name
  std::string m_excludePropertyName;

  /// Max size for seq domain
  mutable size_t m_maxSize;
  /// Option to normalise the data
  mutable bool m_normalise;
  /// Ranges that must be excluded from fit
  mutable std::vector<double> m_exclude;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_FITMW_H_*/
