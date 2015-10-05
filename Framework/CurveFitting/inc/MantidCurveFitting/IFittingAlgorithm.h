#ifndef MANTID_CURVEFITTING_IFITTINGALGORITHM_H_
#define MANTID_CURVEFITTING_IFITTINGALGORITHM_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IDomainCreator.h"

namespace Mantid {

namespace API {
class IFunction;
}

namespace CurveFitting {

/**

  A base class for fitting algorithms. It declares two properties:
  "Function" and "InputWorkspace". When these properties are set it
  creates an appropriate domain creator.

  The concrete classes must implement methods:
    - name()
    - version()
    - summary()
    - exec()
    - initConcrete() to declare more properties

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport IFittingAlgorithm : public API::Algorithm {
public:
  IFittingAlgorithm();
  virtual ~IFittingAlgorithm();

  virtual const std::string category() const;

private:
  void init();
  void exec();
  /// Child classes declare their properties here.
  virtual void initConcrete() = 0;
  /// Child classes implement the algorithm logic here.
  virtual void execConcrete() = 0;

  virtual void afterPropertySet(const std::string &propName);
  void addWorkspace(const std::string &workspaceNameProperty,
                    bool addProperties = true);
  /// Read domain type property and cache the value
  void setDomainType();

protected:
  void setFunction();
  void addWorkspaces();

  /// Keep the domain type
  API::IDomainCreator::DomainType m_domainType;
  /// Pointer to the fitting function
  boost::shared_ptr<API::IFunction> m_function;
  /// Pointer to a domain creator
  boost::shared_ptr<API::IDomainCreator> m_domainCreator;
  std::vector<std::string> m_workspacePropertyNames;

  friend class API::IDomainCreator;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_IFITTINGALGORITHM_H_ */