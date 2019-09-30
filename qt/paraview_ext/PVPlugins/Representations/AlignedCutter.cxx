/*=========================================================================

  Program:   Visualization Toolkit
  Module:    AlignedCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "AlignedCutter.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkImplicitFunction.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"

#include <array>

vtkStandardNewMacro(AlignedCutter);

AlignedCutter::AlignedCutter(vtkImplicitFunction *cf) : vtkCutter(cf) {}

//----------------------------------------------------------------------------
AlignedCutter::~AlignedCutter() = default;

namespace {
std::array<double, 3> getOffset(vtkDataArray *input, int64_t lastPos,
                                double celldim) {
  double first[3], last[3];
  input->GetTuple(0, first);
  input->GetTuple(lastPos, last);
  double prefactor = 0.5 / celldim;
  std::array<double, 3> offset;
  for (size_t i = 0; i < 3; ++i) {
    offset[i] = prefactor * (last[i] - first[i]);
  }
  return offset;
}
} // namespace

//----------------------------------------------------------------------------
void AlignedCutter::AlignedStructuredGridCutter(vtkDataSet *dataSetInput,
                                                vtkPolyData *thisOutput) {
  vtkStructuredGrid *input = vtkStructuredGrid::SafeDownCast(dataSetInput);
  vtkNew<vtkPolyData> output;
  output->Allocate();
  vtkIdType numPts = input->GetNumberOfPoints();

  if (numPts < 1) {
    return;
  }

  vtkNew<vtkDoubleArray> cutScalars;
  cutScalars->SetName("cutScalars");

  vtkDataArray *dataArrayInput = input->GetPoints()->GetData();

  int dims[3], celldims[3];
  input->GetDimensions(dims);
  vtkIdType d01 = vtkIdType{dims[0]} * dims[1];
  input->GetCellDims(celldims);
  vtkIdType cd01 = vtkIdType{celldims[0]} * celldims[1];
  auto inCD = input->GetCellData();
  auto outCD = output->GetCellData();
  outCD->CopyAllocate(inCD);

  vtkNew<vtkIdList> ids;
  vtkNew<vtkPoints> outPts;
  vtkPoints *inPts = input->GetPoints();
  ids->SetNumberOfIds(4);

  vtkIdType NumberOfContours = this->ContourValues->GetNumberOfContours();
  if (AxisNumber == 0) {
    outPts->Allocate(4 * celldims[1] * celldims[2] * NumberOfContours);
  } else if (AxisNumber == 1) {
    outPts->Allocate(4 * celldims[0] * celldims[2] * NumberOfContours);
  } else if (AxisNumber == 2) {
    outPts->Allocate(4 * celldims[0] * celldims[1] * NumberOfContours);
  }
  vtkIdType outCellId = 0;

  for (int i = 0; i != NumberOfContours; ++i) {
    double value = this->ContourValues->GetValue(i);
    if (AxisNumber == 0) {
      std::array<double, 3> offset =
          getOffset(dataArrayInput, celldims[0], celldims[0]);
      cutScalars->SetNumberOfTuples(dims[0]);
      for (vtkIdType j = 0; j < dims[0]; ++j) {
        double x[3];
        dataArrayInput->GetTuple(j, x);
        for (size_t k = 0; k < 3; ++k) {
          x[k] += offset[k];
        }
        double FuncVal = this->CutFunction->EvaluateFunction(x);
        cutScalars->SetTypedComponent(i, 0, std::abs(FuncVal - value));
      }
    } else if (AxisNumber == 1) {
      std::array<double, 3> offset =
          getOffset(dataArrayInput, d01 - dims[0], celldims[1]);
      cutScalars->SetNumberOfTuples(dims[1]);
      for (vtkIdType j = 0, k = 0; j < d01; j = j + dims[0], ++k) {
        double x[3];
        dataArrayInput->GetTuple(i, x);
        for (size_t l = 0; l < 3; ++l) {
          x[l] += offset[l];
        }
        double FuncVal = this->CutFunction->EvaluateFunction(x);
        cutScalars->SetTypedComponent(k, 0, std::abs(FuncVal - value));
      }
    } else if (AxisNumber == 2) {
      std::array<double, 3> offset =
          getOffset(dataArrayInput, numPts - d01, celldims[2]);
      cutScalars->SetNumberOfTuples(dims[2]);
      for (vtkIdType j = 0, k = 0; j < numPts; j = j + d01, ++k) {
        double x[3];
        dataArrayInput->GetTuple(j, x);
        for (size_t l = 0; l < 3; ++l) {
          x[l] += offset[l];
        }
        double FuncVal = this->CutFunction->EvaluateFunction(x);
        cutScalars->SetTypedComponent(k, 0, std::abs(FuncVal - value));
      }
    }
    double *ptr = cutScalars->GetPointer(0);
    vtkIdType min = std::distance(
        ptr, std::min_element(ptr, ptr + cutScalars->GetNumberOfTuples()));

    // check for out-of-bounds values
    if (min == 0 || min == celldims[AxisNumber])
      break;

    min = std::min(min, static_cast<vtkIdType>(celldims[AxisNumber] - 1));

    if (AxisNumber == 0) {
      for (int j = 0; j < celldims[1]; ++j) {
        for (int k = 0; k < celldims[2]; ++k) {
          vtkIdType index = min + j * celldims[0] + k * cd01;
          if (input->IsCellVisible(index)) {
            double x[3];
            inPts->GetPoint(min + j * dims[0] + k * d01, x);
            ids->SetId(0, outPts->InsertNextPoint(x));
            inPts->GetPoint(min + j * dims[0] + (k + 1) * d01, x);
            ids->SetId(1, outPts->InsertNextPoint(x));
            inPts->GetPoint(min + (j + 1) * dims[0] + (k + 1) * d01, x);
            ids->SetId(2, outPts->InsertNextPoint(x));
            inPts->GetPoint(min + (j + 1) * dims[0] + k * d01, x);
            ids->SetId(3, outPts->InsertNextPoint(x));
            output->InsertNextCell(VTK_QUAD, ids.Get());
            outCD->CopyData(inCD, index, outCellId++);
          }
        }
      }
    } else if (AxisNumber == 1) {
      for (int j = 0; j < celldims[0]; ++j) {
        for (int k = 0; k < celldims[2]; ++k) {
          vtkIdType index = j + min * celldims[0] + k * cd01;
          if (input->IsCellVisible(index)) {
            double x[3];
            inPts->GetPoint(j + min * dims[0] + k * d01, x);
            ids->SetId(0, outPts->InsertNextPoint(x));
            inPts->GetPoint(j + 1 + min * dims[0] + k * d01, x);
            ids->SetId(1, outPts->InsertNextPoint(x));
            inPts->GetPoint(j + 1 + min * dims[0] + (k + 1) * d01, x);
            ids->SetId(2, outPts->InsertNextPoint(x));
            inPts->GetPoint(j + min * dims[0] + (k + 1) * d01, x);
            ids->SetId(3, outPts->InsertNextPoint(x));
            output->InsertNextCell(VTK_QUAD, ids.Get());
            outCD->CopyData(inCD, index, outCellId++);
          }
        }
      }
    } else if (AxisNumber == 2) {
      for (int j = 0; j < celldims[0]; ++j) {
        for (int k = 0; k < celldims[1]; ++k) {
          vtkIdType index = j + k * celldims[0] + min * cd01;
          if (input->IsCellVisible(index)) {
            double x[3];
            inPts->GetPoint(j + k * dims[0] + min * d01, x);
            ids->SetId(0, outPts->InsertNextPoint(x));
            inPts->GetPoint(j + (k + 1) * dims[0] + min * d01, x);
            ids->SetId(1, outPts->InsertNextPoint(x));
            inPts->GetPoint(j + 1 + (k + 1) * dims[0] + min * d01, x);
            ids->SetId(2, outPts->InsertNextPoint(x));
            inPts->GetPoint(j + 1 + k * dims[0] + min * d01, x);
            ids->SetId(3, outPts->InsertNextPoint(x));
            output->InsertNextCell(VTK_QUAD, ids.Get());
            outCD->CopyData(inCD, index, outCellId++);
          }
        }
      }
    }
  }
  output->SetPoints(outPts.Get());
  thisOutput->ShallowCopy(output.Get());
}

namespace {
//----------------------------------------------------------------------------
// Find the first visible cell in a vtkStructuredGrid.
//
vtkIdType GetFirstVisibleCell(vtkDataSet *DataSetInput) {
  vtkStructuredGrid *input = vtkStructuredGrid::SafeDownCast(DataSetInput);
  if (input) {
    if (input->HasAnyBlankCells()) {
      vtkIdType size = input->GetNumberOfElements(vtkDataSet::CELL);
      for (vtkIdType i = 0; i < size; ++i) {
        if (input->IsCellVisible(i) != 0) {
          return i;
        }
      }
    }
  }
  return 0;
}
} // namespace

//----------------------------------------------------------------------------
// Cut through data generating surface.
//
int AlignedCutter::RequestData(vtkInformation *request,
                               vtkInformationVector **inputVector,
                               vtkInformationVector *outputVector) {
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input =
      vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output =
      vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<< "Executing cutter");
  if (!this->CutFunction) {
    vtkErrorMacro("No cut function specified");
    return 0;
  }

  if (!input) {
    // this could be a table in a multiblock structure, i.e. no cut!
    return 0;
  }

  if (input->GetNumberOfPoints() < 1 || this->GetNumberOfContours() < 1) {
    return 1;
  }

#ifdef TIMEME
  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  timer->StartTimer();
#endif

  if ((input->GetDataObjectType() == VTK_STRUCTURED_POINTS ||
       input->GetDataObjectType() == VTK_IMAGE_DATA) &&
      input->GetCell(0) && input->GetCell(0)->GetCellDimension() >= 3) {
    this->StructuredPointsCutter(input, output, request, inputVector,
                                 outputVector);
  } else if (input->GetDataObjectType() == VTK_STRUCTURED_GRID &&
             input->GetCell(0) &&
             input->GetCell(GetFirstVisibleCell(input))->GetCellDimension() >=
                 3) {
    this->AlignedStructuredGridCutter(input, output);
  } else if (input->GetDataObjectType() == VTK_RECTILINEAR_GRID &&
             static_cast<vtkRectilinearGrid *>(input)->GetDataDimension() ==
                 3) {
    this->RectilinearGridCutter(input, output);
  } else if (input->GetDataObjectType() == VTK_UNSTRUCTURED_GRID_BASE ||
             input->GetDataObjectType() == VTK_UNSTRUCTURED_GRID) {
    vtkDebugMacro(<< "Executing Unstructured Grid Cutter");
    this->UnstructuredGridCutter(input, output);
  } else {
    vtkDebugMacro(<< "Executing DataSet Cutter");
    this->DataSetCutter(input, output);
  }

#ifdef TIMEME
  timer->StopTimer();
  cout << "Sliced " << output->GetNumberOfCells() << " cells in "
       << timer->GetElapsedTime() << " secs " << endl;
#endif
  return 1;
}
