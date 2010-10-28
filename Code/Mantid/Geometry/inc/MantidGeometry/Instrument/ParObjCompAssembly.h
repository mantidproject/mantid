#ifndef PAR_OBJ_COMPONENT_ASSEMBLY_
#define PAR_OBJ_COMPONENT_ASSEMBLY_
#include <string> 
#include <vector>
#include "MantidKernel/System.h"
#include "MantidGeometry/Instrument/ParObjComponent.h"
#include "MantidGeometry/ICompAssembly.h"

namespace Mantid
{
namespace Geometry
{

class ObjCompAssembly;

/** @class ParCompAssembly 
    @brief A wrapper for CompAssembly with possiblty modified parameters.
    @version A
    @author Laurent C Chapon, ISIS RAL
    @date 01/11/2007

    
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
class DLLExport ParObjCompAssembly : public virtual ICompAssembly, public virtual ParObjComponent
{
public:
  ///String description of the type of component
  virtual std::string type() const { return "ParObjCompAssembly";}
  /// Constructor
  ParObjCompAssembly(const ObjCompAssembly* base, const ParameterMap& map); 
  //! Destructor
  ~ParObjCompAssembly(){}
  //! Make a clone of the present component
  virtual IComponent* clone() const;
  //! Return the number of elements in the assembly
  int nelements() const;
  //! Add a component to the assembly
  int add(IComponent*);
  //! Add a copy (clone) of a component 
  int addCopy(IComponent*);
  //! Add a copy (clone) of a component and rename it
  int addCopy(IComponent*, const std::string&);
  //! Get a pointer to the ith component within the assembly. Easier to use than [] when you have a pointer
  boost::shared_ptr<IComponent> getChild(const int i) const{return (*this)[i];}
  //! Get a pointer to the ith component in the assembly
  boost::shared_ptr<IComponent> operator[](int i) const;
  //! Print information about all children
  void printChildren(std::ostream&) const;
  void printTree(std::ostream&) const;

  //! Get the position of the IComponent. Tree structure is traversed through the parent chain
  virtual V3D getPos() const;
  //! Get the Rotation of the IComponent.
  virtual const Quat getRotation() const;

private:
  ParObjCompAssembly(const ParObjCompAssembly&); 
  /// Private copy assignment operator
  ParObjCompAssembly& operator=(const ICompAssembly&);
};

DLLExport std::ostream& operator<<(std::ostream&, const ParObjCompAssembly&);

} //Namespace Geometry
} //Namespace Mantid

#endif /*PAR_OBJ_COMPONENT_ASSEMBLY_*/
