#ifndef PARCOMPONENT_ASSEMBLY_
#define PARCOMPONENT_ASSEMBLY_
#include <string> 
#include <vector>
#include "MantidKernel/System.h"
#include "MantidGeometry/Instrument/ParametrizedComponent.h"
#include "MantidGeometry/ICompAssembly.h"

namespace Mantid
{
namespace Geometry
{

class CompAssembly;

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
class DLLExport ParCompAssembly : public virtual ICompAssembly, public ParametrizedComponent
{
public:
  ///String description of the type of component
  virtual std::string type() const { return "ParCompAssembly";}
  /// Constructor
  ParCompAssembly(const CompAssembly* base, const ParameterMap& map); 
  //! Destructor
  ~ParCompAssembly(){}
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
  //! Get a pointer to the ith component in the assembly
  boost::shared_ptr<IComponent> operator[](int i) const;
  /// Returns a child component at the given X/Y pixel index.
  virtual boost::shared_ptr<IComponent> getChildAtXY(int X, int Y) const;
  /// Set the pixel size of the detector (optional)
  virtual void setNumPixels(int num_xPixels, int num_yPixels);

  /// Get the bounding box for this assembly and store it in the given argument
  virtual void getBoundingBox(BoundingBox& boundingBox) const;

  //! Print information about all children
  void printChildren(std::ostream&) const;
  void printTree(std::ostream&) const;

  //! Get the position of the IComponent. Tree structure is traversed through the parent chain
  virtual V3D getPos() const;
  //! Get the Rotation of the IComponent.
  virtual const Quat getRotation() const;

private:
  /// Private copy assignment operator
  ParCompAssembly& operator=(const ICompAssembly&);

  /// The number of pixels in the X (horizontal) direction; optional; for area detectors
  int xPixels;
  /// The number of pixels in the Y (vertical) direction; optional; for area detectors
  int yPixels;


};

DLLExport std::ostream& operator<<(std::ostream&, const ParCompAssembly&);

} //Namespace Geometry
} //Namespace Mantid

#endif
