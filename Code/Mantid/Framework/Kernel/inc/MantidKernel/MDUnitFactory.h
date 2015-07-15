#ifndef MANTID_KERNEL_MDUNITFACTORY_H_
#define MANTID_KERNEL_MDUNITFACTORY_H_

#include "MantidKernel/System.h"
#include <boost/optional.hpp>
#include <memory>

namespace Mantid {
namespace Kernel {

// Forward declartion
class MDUnit;


template <typename ChainableType>
class Chainable{
protected:
    /// Successor factory
    boost::optional<std::unique_ptr<ChainableType> > m_successor;
public:
    /// Set the successor
    Chainable& setSuccessor(std::unique_ptr<ChainableType> successor){
        m_successor = std::unique_ptr<ChainableType>(successor.release());
        return *(*m_successor);
    }
    bool hasSuccessor() const {return m_successor.is_initialized();}
    virtual ~Chainable() = 0;
};

template <typename ChainableType>
Chainable<ChainableType>::~Chainable(){}


/** MDUnitFactory : Abstract type. Factory method with chain of reponsibility succession for creating MDUnits.

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
class DLLExport MDUnitFactory : public Chainable<MDUnitFactory> {

public:

  /// Destructor
  virtual ~MDUnitFactory(){}

  /// Create the product
  virtual std::unique_ptr<MDUnit> create(const std::string& unitString) const;

private:

  /// Create the product
  virtual MDUnit* createRaw(const std::string& unitString) const = 0;

  /// Indicate an ability to intepret the string
  virtual bool canInterpret(const std::string& unitString) const = 0;
};


} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_MDUNITFACTORY_H_ */
