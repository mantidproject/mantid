#ifndef ICOMPONENT_ASSEMBLY_
#define ICOMPONENT_ASSEMBLY_
#include <string> 
#include <vector>
#include "MantidGeometry/IComponent.h"
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace Geometry
{
/** @class ICompAssembly 
    @brief Class for Assembly of geometric components. 
    @version A
    @author Laurent C Chapon, ISIS RAL
    @date 01/11/2007

    CompAssembly allows Components to be positioned
    in a hierarchical structure in the form of a tree.
    CompAssembly inherits from component.

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport ICompAssembly : public virtual IComponent
{
public:
  ///String description of the type of component
  virtual std::string type() const { return "ICompAssembly";}
  //ICompAssembly();

  virtual ~ICompAssembly(){}
  //! Make a clone of the present component
  virtual IComponent* clone() const = 0;
  //! Return the number of elements in the assembly
  virtual int nelements() const = 0;
  //! Add a component to the assembly
  virtual int add(IComponent*) = 0;
  //! Add a copy (clone) of a component 
  virtual int addCopy(IComponent*) = 0;
  //! Add a copy (clone) of a component and rename it
  virtual int addCopy(IComponent*, const std::string&) = 0;
  //! Get a pointer to the ith component in the assembly
  virtual boost::shared_ptr<IComponent> operator[](int i) const = 0;
  //! Print information about all children
  virtual void printChildren(std::ostream&) const = 0;
  /*! Print information about all the elements in the tree to a stream
   *  Loops through all components in the tree 
   *  and call printSelf(os). 
   */
  virtual void printTree(std::ostream&) const = 0;
  /// Returns a child component at the given X/Y pixel index.
  virtual boost::shared_ptr<IComponent> getChildAtXY(int X, int Y) const = 0;
  /// Set the pixel size of the detector (optional)
  virtual void setNumPixels(int num_xPixels, int num_yPixels) = 0;

private:
  /// Private copy assignment operator
  ICompAssembly& operator=(const ICompAssembly&);


};

//DLLExport std::ostream& operator<<(std::ostream&, const CompAssembly&);

} //Namespace Geometry
} //Namespace Mantid

#endif
