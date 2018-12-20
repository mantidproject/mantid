/*=========================================================================

  Program:   Visualization Toolkit
  Module:    AlignedCutter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   AlignedCutter
 * @brief   Cut vtkDataSet with user-specified implicit function
 *
 * AlignedCutter is a filter to cut through data using any subclass of
 * vtkImplicitFunction. That is, a polygonal surface is created
 * corresponding to the implicit function F(x,y,z) = value(s), where
 * you can specify one or more values used to cut with.
 *
 * In VTK, cutting means reducing a cell of dimension N to a cut surface
 * of dimension N-1. For example, a tetrahedron when cut by a plane (i.e.,
 * vtkPlane implicit function) will generate triangles. (In comparison,
 * clipping takes a N dimensional cell and creates N dimension primitives.)
 *
 * AlignedCutter is generally used to "slice-through" a dataset, generating
 * a surface that can be visualized. It is also possible to use AlignedCutter
 * to do a form of volume rendering. AlignedCutter does this by generating
 * multiple cut surfaces (usually planes) which are ordered (and rendered)
 * from back-to-front. The surfaces are set translucent to give a
 * volumetric rendering effect.
 *
 * Note that data can be cut using either 1) the scalar values associated
 * with the dataset or 2) an implicit function associated with this class.
 * By default, if an implicit function is set it is used to clip the data
 * set, otherwise the dataset scalars are used to perform the clipping.
 *
 * @sa
 * vtkImplicitFunction vtkClipPolyData
 */

#ifndef AlignedCutter_h
#define AlignedCutter_h

#include "vtkCutter.h"

class VTK_EXPORT AlignedCutter : public vtkCutter {
public:
  vtkTypeMacro(AlignedCutter, vtkCutter);
  vtkSetMacro(AxisNumber, int);
  vtkGetMacro(AxisNumber, int);
  /**
   * Construct with user-specified implicit function; initial value of 0.0; and
   * generating cut scalars turned off.
   */
  static AlignedCutter *New();

protected:
  explicit AlignedCutter(vtkImplicitFunction *cf = nullptr);
  ~AlignedCutter() override;
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;
  void AlignedStructuredGridCutter(vtkDataSet *, vtkPolyData *);
  int AxisNumber{0};

private:
  AlignedCutter(const AlignedCutter &) VTK_DELETE_FUNCTION;
  void operator=(const AlignedCutter &) VTK_DELETE_FUNCTION;
};
//@}

#endif
