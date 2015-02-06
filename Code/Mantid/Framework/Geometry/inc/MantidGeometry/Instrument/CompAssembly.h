#ifndef COMPONENT_ASSEMBLY_
#define COMPONENT_ASSEMBLY_
#include <string>
#include <vector>
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/ICompAssembly.h"

#ifdef _WIN32
#pragma warning(disable : 4250)
#endif

namespace Mantid {
namespace Geometry {

/** @class CompAssembly
    @brief Class for Assembly of geometric components.
    @version A
    @author Laurent C Chapon, ISIS RAL
    @date 01/11/2007

    CompAssembly allows Components to be positioned
    in a hierarchical structure in the form of a tree.
    CompAssembly inherits from component.

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
class MANTID_GEOMETRY_DLL CompAssembly : public ICompAssembly,
                                         public Component {
protected:
  typedef std::vector<IComponent *>::iterator comp_it; ///< Iterator type
  typedef std::vector<IComponent *>::const_iterator
      const_comp_it; ///< Const iterator type

public:
  /// String description of the type of component
  virtual std::string type() const { return "CompAssembly"; }
  //! Empty constructor
  CompAssembly();
  /// Constructor for parametrized version
  CompAssembly(const IComponent *base, const ParameterMap *map);
  //! Constructor with a name and parent reference
  CompAssembly(const std::string &, IComponent *reference = 0);
  //! Copy constructor
  CompAssembly(const CompAssembly &);
  virtual ~CompAssembly();
  //! Make a clone of the present component
  virtual IComponent *clone() const;
  //! Return the number of elements in the assembly
  int nelements() const;
  //! Add a component to the assembly
  int add(IComponent *);
  //! Add a copy (clone) of a component

  void addChildren(IComponent *comp);

  int addCopy(IComponent *);
  //! Add a copy (clone) of a component and rename it
  int addCopy(IComponent *, const std::string &);
  /// Remove a component from the assembly
  int remove(IComponent *);
  //! Get a pointer to the ith component within the assembly. Easier to use than
  //[] when you have a pointer
  boost::shared_ptr<IComponent> getChild(const int i) const;
  //! Returns a vector of all children contained.
  void getChildren(std::vector<IComponent_const_sptr> &outVector,
                   bool recursive) const;
  //! Get a pointer to the ith component in the assembly
  boost::shared_ptr<IComponent> operator[](int i) const;
  /// Returns a pointer to the first component of assembly encountered with the
  /// given name
  virtual boost::shared_ptr<const IComponent>
  getComponentByName(const std::string &cname, int nlevels = 0) const;

  Kernel::V3D getPos() const;

  const Kernel::Quat getRotation() const;

  /// Get the bounding box for this component and store it in the given argument
  virtual void getBoundingBox(BoundingBox &boundingBox) const;

  //! Print information about all children
  void printChildren(std::ostream &) const;
  void printTree(std::ostream &) const;

  /** Test the intersection of the ray with the children of the component
   * assembly, for InstrumentRayTracer  */
  virtual void testIntersectionWithChildren(
      Track &testRay, std::deque<IComponent_const_sptr> &searchQueue) const;

private:
  /// Private copy assignment operator
  CompAssembly &operator=(const ICompAssembly &);

protected:
  /// the group of child components
  std::vector<IComponent *> m_children;

  /// A cached bounding box
  mutable BoundingBox *m_cachedBoundingBox;
};

MANTID_GEOMETRY_DLL std::ostream &operator<<(std::ostream &,
                                             const CompAssembly &);

} // Namespace Geometry
} // Namespace Mantid

#endif
