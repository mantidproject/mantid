/*=========================================================================

  Program:   Visualization Toolkit
  Module:    AlignedThreeSliceFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   AlignedThreeSliceFilter
 * @brief   Cut vtkDataSet along 3 planes
 *
 * AlignedThreeSliceFilter is a filter that slice the input data using 3 plane
 *cut.
 * Each axis cut could embed several slices by providing several values.
 * As output you will find 4 output ports.
 * The output ports are defined as follow:
 * - 0: Merge of all the cutter output
 * - 1: Output of the first internal vtkCutter filter
 * - 2: Output of the second internal vtkCutter filter
 * - 3: Output of the third internal vtkCutter filter
 */

#ifndef AlignedThreeSliceFilter_h
#define AlignedThreeSliceFilter_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkThreeSliceFilter.h"

class VTK_EXPORT AlignedThreeSliceFilter : public vtkThreeSliceFilter {
public:
  vtkTypeMacro(AlignedThreeSliceFilter, vtkThreeSliceFilter);
  /**
   * Construct with user-specified implicit function; initial value of 0.0; and
   * generating cut scalars turned off.
   */
  static AlignedThreeSliceFilter *New();

protected:
  AlignedThreeSliceFilter();
  ~AlignedThreeSliceFilter() override;

private:
  AlignedThreeSliceFilter(const AlignedThreeSliceFilter &) VTK_DELETE_FUNCTION;
  void operator=(const AlignedThreeSliceFilter &) VTK_DELETE_FUNCTION;
};

#endif
