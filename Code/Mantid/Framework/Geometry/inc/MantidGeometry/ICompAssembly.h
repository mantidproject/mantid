#ifndef MANTID_GEOMETRY_ICOMPASSEMBLY_
#define MANTID_GEOMETRY_ICOMPASSEMBLY_

#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/DllConfig.h"
#include <deque>
#include <string>
#include <vector>

namespace Mantid
{
  namespace Geometry
  {
    // Forward declaration
    class Track;

    /** @class ICompAssembly 
    @brief Class for Assembly of geometric components. 
    @version A
    @author Laurent C Chapon, ISIS RAL
    @date 01/11/2007

    CompAssembly allows Components to be positioned
    in a hierarchical structure in the form of a tree.
    CompAssembly inherits from component.

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class MANTID_GEOMETRY_DLL ICompAssembly : public virtual IComponent
    {
    public:
      ///String description of the type of component
      virtual std::string type() const { return "ICompAssembly";}

      virtual ~ICompAssembly(){}
      /// Make a clone of the present component
      virtual IComponent* clone() const = 0;
      /// Return the number of elements in the assembly
      virtual int nelements() const = 0;
      /// Add a component to the assembly
      virtual int add(IComponent*) = 0;
      /// Add a copy (clone) of a component 
      virtual int addCopy(IComponent*) = 0;
      /// Add a copy (clone) of a component and rename it
      virtual int addCopy(IComponent*, const std::string&) = 0;
      /// Get a pointer to the ith component within the assembly. Easier to use than [] when you have a pointer
      virtual boost::shared_ptr<IComponent> getChild(const int i) const = 0;
      /// Returns a pointer to the first component of assembly encountered with the given name
      virtual boost::shared_ptr<const IComponent> getComponentByName(const std::string & cname, int nlevels = 0) const = 0; 
      /// Get all children
      virtual void getChildren(std::vector<IComponent_const_sptr> & outVector, bool recursive) const = 0;
      /// Overloaded index operator. Get a pointer to the ith component in the assembly
      virtual boost::shared_ptr<IComponent> operator[](int i) const = 0;
      /// Print information about all children
      virtual void printChildren(std::ostream&) const = 0;
      /** Print information about all the elements in the tree to a stream
       *  Loops through all components in the tree 
       *  and call printSelf(os). 
       */
      virtual void printTree(std::ostream&) const = 0;


      /// Test the intersection of the ray with the children of the component assembly
      virtual void testIntersectionWithChildren(Track & testRay, std::deque<IComponent_const_sptr> & searchQueue) const = 0;

    private:
      /// Private copy assignment operator
      ICompAssembly& operator=(const ICompAssembly&);

    };

    /// Shared pointer to a ICompAssembly
    typedef boost::shared_ptr<ICompAssembly> ICompAssembly_sptr;
    /// Shared pointer to a const ICompAssembly
    typedef boost::shared_ptr<const ICompAssembly> ICompAssembly_const_sptr;

  } //Namespace Geometry
} //Namespace Mantid


#endif
