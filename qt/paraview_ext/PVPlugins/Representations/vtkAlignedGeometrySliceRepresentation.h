// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/*=========================================================================

  Program:   ParaView
  Module:    vtkAlignedGeometrySliceRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAlignedGeometrySliceRepresentation
 * @brief   extends vtkGeometryRepresentation to
 * add support for showing just specific slices from the dataset.
 *
 * vtkAlignedGeometrySliceRepresentation extends vtkGeometryRepresentation to
 * show slices from the dataset. This is used for vtkPVMultiSliceView and
 * vtkPVOrthographicSliceView.
 */

#ifndef vtkAlignedGeometrySliceRepresentation_h
#define vtkAlignedGeometrySliceRepresentation_h

#include "vtkGeometryRepresentation.h"

class VTK_EXPORT vtkAlignedGeometrySliceRepresentation
    : public vtkGeometryRepresentation {
public:
  static vtkAlignedGeometrySliceRepresentation *New();
  vtkTypeMacro(vtkAlignedGeometrySliceRepresentation,
               vtkGeometryRepresentation);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  int ProcessViewRequest(vtkInformationRequestKey *request_type,
                         vtkInformation *inInfo,
                         vtkInformation *outInfo) VTK_OVERRIDE;

  enum { X_SLICE_ONLY, Y_SLICE_ONLY, Z_SLICE_ONLY, ALL_SLICES };
  vtkSetClampMacro(Mode, int, X_SLICE_ONLY, ALL_SLICES);
  vtkGetMacro(Mode, int);

  //@{
  /**
   * Get/Set whether original data outline should be shown in the view.
   */
  vtkSetMacro(ShowOutline, bool);
  vtkGetMacro(ShowOutline, bool);
  //@}

protected:
  vtkAlignedGeometrySliceRepresentation();
  ~vtkAlignedGeometrySliceRepresentation() override;

  void SetupDefaults() VTK_OVERRIDE;
  int RequestData(vtkInformation *request, vtkInformationVector **inputVector,
                  vtkInformationVector *outputVector) VTK_OVERRIDE;

  bool AddToView(vtkView *view) VTK_OVERRIDE;
  bool RemoveFromView(vtkView *view) VTK_OVERRIDE;

private:
  vtkAlignedGeometrySliceRepresentation(
      const vtkAlignedGeometrySliceRepresentation &) VTK_DELETE_FUNCTION;
  void
  operator=(const vtkAlignedGeometrySliceRepresentation &) VTK_DELETE_FUNCTION;

  class vtkInternals;
  vtkInternals *Internals;
  int Mode;
  bool ShowOutline;
};

#endif
