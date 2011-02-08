#ifndef ABSTRACT_CLIPPER_H
#define ABSTRACT_CLIPPER_H

//Forward declarations
class vtkDataSet;
class vtkUnstructuredGrid;
class vtkImplicitFunction;

namespace Mantid
{
namespace VATES
{

  /** Abstract clipper type. Allows full separation of vendor specific vtk technology from back end.

 @author Owen Arnold, Tessella plc
 @date 07/02/2011

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

class Clipper
{
public:

  virtual void SetInput(vtkDataSet* in_ds) =0;

  virtual void SetClipFunction(vtkImplicitFunction* func) =0;

  virtual void SetInsideOut(bool insideout) =0;

  virtual void SetRemoveWholeCells(bool removeWholeCells) =0;

  virtual void SetOutput(vtkUnstructuredGrid* out_ds) =0;

  virtual void Update() = 0;

  virtual ~Clipper() =0;

  virtual void Delete() = 0;
};

}
}


#endif
