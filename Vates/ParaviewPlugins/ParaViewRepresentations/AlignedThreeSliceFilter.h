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
 * AlignedThreeSliceFilter is a filter that slice the input data using 3 plane cut.
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
#include "vtkPolyDataAlgorithm.h"

class vtkAppendPolyData;
class vtkCellData;
class AlignedCutter;
class vtkDataSet;
class vtkPProbeFilter;
class vtkPlane;
class vtkPointData;
class vtkPointSource;
class vtkPolyData;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT AlignedThreeSliceFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(AlignedThreeSliceFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct with user-specified implicit function; initial value of 0.0; and
   * generating cut scalars turned off.
   */
  static AlignedThreeSliceFilter* New();

  /**
   * Override GetMTime because we rely on internal filters that have their own MTime
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  /**
   * Set a Slice Normal for a given cutter
   */
  void SetCutNormal(int cutIndex, double normal[3]);

  /**
   * Set a slice Origin for a given cutter
   */
  void SetCutOrigin(int cutIndex, double origin[3]);

  /**
   * Set a slice value for a given cutter
   */
  void SetCutValue(int cutIndex, int index, double value);

  /**
   * Set number of slices for a given cutter
   */
  void SetNumberOfSlice(int cutIndex, int size);

  /**
   * Default settings:
   * - reset the plan origin to be (0,0,0)
   * - number of slice for X, Y and Z to be 0
   * - Normal for SliceX=[1,0,0], SliceY=[0,1,0], SliceZ=[0,0,1]
   */
  void SetToDefaultSettings();

  /**
   * Set slice Origin for all cutter
   */
  void SetCutOrigins(double origin[3]);
  void SetCutOrigins(double x, double y, double z)
  {
    double xyz[] = { x, y, z };
    this->SetCutOrigins(xyz);
  }

  /**
   * Enable to probe the dataset at the given cut origin.
   */
  void EnableProbe(int enable);

  /**
   * Return true if any data is available and provide the value as argument
   */
  bool GetProbedPointData(const char* arrayName, double& value);

protected:
  AlignedThreeSliceFilter();
  ~AlignedThreeSliceFilter();

  virtual int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  AlignedCutter* Slices[3];
  vtkPlane* Planes[3];
  vtkAppendPolyData* CombinedFilteredInput;
  vtkPProbeFilter* Probe;
  vtkPointSource* PointToProbe;

  void Process(vtkDataSet* input, vtkPolyData* outputs[4], unsigned int compositeIndex);

private:
  AlignedThreeSliceFilter(const AlignedThreeSliceFilter&) VTK_DELETE_FUNCTION;
  void operator=(const AlignedThreeSliceFilter&) VTK_DELETE_FUNCTION;
};

#endif
