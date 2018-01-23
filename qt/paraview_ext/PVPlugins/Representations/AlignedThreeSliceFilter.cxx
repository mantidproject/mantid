/*=========================================================================

  Program:   Visualization Toolkit
  Module:    AlignedThreeSliceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "AlignedThreeSliceFilter.h"

#include "AlignedCutter.h"
#include "vtkAppendPolyData.h"
#include "vtkPlane.h"

#include <cmath>

vtkStandardNewMacro(AlignedThreeSliceFilter);

//----------------------------------------------------------------------------
AlignedThreeSliceFilter::AlignedThreeSliceFilter() : vtkThreeSliceFilter() {
  this->CombinedFilteredInput->RemoveAllInputs();
  for (int i = 0; i < 3; ++i)
  {
    // Allocate internal vars
    this->Slices[i]->Delete();
    // AxisNumber cannot be set after being cast to vtkCutter.
    AlignedCutter *temp = AlignedCutter::New();
    temp->SetAxisNumber(i);
    temp->SetCutFunction(this->Planes[i]);
    this->Slices[i] = temp;
    // Bind pipeline
    this->CombinedFilteredInput->AddInputConnection(this->Slices[i]->GetOutputPort());
  }
  this->SetToDefaultSettings();
}

//----------------------------------------------------------------------------
AlignedThreeSliceFilter::~AlignedThreeSliceFilter() = default;
