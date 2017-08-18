/*=========================================================================

  Program:   ParaView
  Module:    vtkMySpecialRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMySpecialRepresentation
 * @brief   extends vtkGeometryRepresentation to
 * add support for showing just specific slices from the dataset.
 *
 * vtkMySpecialRepresentation extends vtkGeometryRepresentation to show
 * slices from the dataset. This is used for vtkPVMultiSliceView and
 * vtkPVOrthographicSliceView.
*/

#ifndef vtkMySpecialRepresentation_h
#define vtkMySpecialRepresentation_h

#include "vtkGeometryRepresentation.h"

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkMySpecialRepresentation
  : public vtkGeometryRepresentation
{
public:
  static vtkMySpecialRepresentation* New();
  vtkTypeMacro(vtkMySpecialRepresentation, vtkGeometryRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) VTK_OVERRIDE;

  enum
  {
    X_SLICE_ONLY,
    Y_SLICE_ONLY,
    Z_SLICE_ONLY,
    ALL_SLICES
  };
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
  vtkMySpecialRepresentation();
  ~vtkMySpecialRepresentation();

  virtual void SetupDefaults() VTK_OVERRIDE;
  virtual int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;

  virtual bool AddToView(vtkView* view) VTK_OVERRIDE;
  virtual bool RemoveFromView(vtkView* view) VTK_OVERRIDE;

private:
  vtkMySpecialRepresentation(const vtkMySpecialRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMySpecialRepresentation&) VTK_DELETE_FUNCTION;

  class vtkInternals;
  vtkInternals* Internals;
  int Mode;
  bool ShowOutline;
};

#endif
