#ifndef MANTID_VATES_VTKPROXYFACTORY_H_
#define MANTID_VATES_VTKPROXYFACTORY_H_

#include "boost/shared_ptr.hpp"
#include "MantidVisitPresenters/vtkDataSetFactory.h"

namespace Mantid
{
namespace VATES
{

/** Proxy Factory. This type acts as a proxy for a previously generated vtkDataSet. Makes core code invarient
 * to the requirement to occasionally cache datasets rather than generate them from scratch.

 @author Owen Arnold, Tessella plc
 @date 04/02/2011

 Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

class vtkProxyFactory : public vtkDataSetFactory
{

public:

  /// Constructor accepting product.
  vtkProxyFactory(vtkDataSet* product);

  vtkProxyFactory(const vtkProxyFactory& other);

  vtkProxyFactory& operator=(const vtkProxyFactory& other);

  /// Destructor
  virtual ~vtkProxyFactory();

  /// Factory Method. Returns internal product reference.
  virtual vtkDataSet* create() const;

private:

  //Neither create nor destroy.
  vtkDataSet* m_product;

};

}
}


#endif
