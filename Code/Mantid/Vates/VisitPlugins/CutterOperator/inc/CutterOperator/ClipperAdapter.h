
#ifndef CLIPPERADAPTER_H_
#define CLIPPERADAPTER_H_

 /** Concrete implementation of abstract clipper. Adapter wraps visit clipper.
  * All calls to this type forwarded to adaptee.

 @author Owen Arnold, Tessella plc
 @date 08/02/2011

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


#include "MantidVisitPresenters/Clipper.h"
#include <vtkVisItClipper.h>

class ClipperAdapter : public Mantid::VATES::Clipper
{
private:
  vtkVisItClipper* m_clipper;
public:

  ClipperAdapter(vtkVisItClipper* pClipper);

  void SetInput(::vtkDataSet* in_ds);

  void SetClipFunction(vtkImplicitFunction* func);

  void SetInsideOut(bool insideout);

  void SetRemoveWholeCells(bool removeWholeCells);

  void SetOutput(vtkUnstructuredGrid* out_ds);

  void Update();

  void Delete();

  ~ClipperAdapter();

};

#endif
