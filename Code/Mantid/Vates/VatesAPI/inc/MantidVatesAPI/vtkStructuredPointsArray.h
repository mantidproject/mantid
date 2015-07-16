/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkStructuredPointsArray - Map native Exodus II results arrays
// into the vtkDataArray interface.
//
// .SECTION Description
// Map native Exodus II results arrays into the vtkDataArray interface. Use
// the vtkCPExodusIIInSituReader to read an Exodus II file's data into this
// structure.

#ifndef vtkStructuredPointsArray_h
#define vtkStructuredPointsArray_h

#include "vtkMappedDataArray.h"

#include "vtkTypeTemplate.h"  // For templated vtkObject API
#include "vtkObjectFactory.h" // for vtkStandardNewMacro

#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidAPI/NullCoordTransform.h"
#include <vtkMatrix3x3.h>

#include <cstdlib>

class vtkImageData;

template <class Scalar>
class vtkStructuredPointsArray
    : public vtkTypeTemplate<vtkStructuredPointsArray<Scalar>,
                             vtkMappedDataArray<Scalar>> {
public:
  vtkMappedDataArrayNewInstanceMacro(
      vtkStructuredPointsArray<Scalar>) static vtkStructuredPointsArray *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  void InitializeArray(Mantid::DataObjects::MDHistoWorkspace *points);
  void InitializeArray(Mantid::DataObjects::MDHistoWorkspace *points,
                       const double *skewMatrix);

  // Reimplemented virtuals -- see superclasses for descriptions:
  void Initialize();
  void GetTuples(vtkIdList *ptIds, vtkAbstractArray *output);
  void GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray *output);
  void Squeeze();
  vtkArrayIterator *NewIterator();
  vtkIdType LookupValue(vtkVariant value);
  void LookupValue(vtkVariant value, vtkIdList *ids);
  vtkVariant GetVariantValue(vtkIdType idx);
  void ClearLookup();
  double *GetTuple(vtkIdType i);
  void GetTuple(vtkIdType i, double *tuple);
  vtkIdType LookupTypedValue(Scalar value);
  void LookupTypedValue(Scalar value, vtkIdList *ids);
  Scalar GetValue(vtkIdType idx);
  Scalar &GetValueReference(vtkIdType idx);
  void GetTupleValue(vtkIdType idx, Scalar *t);

  // Description:
  // This container is read only -- this method does nothing but print a
  // warning.
  int Allocate(vtkIdType sz, vtkIdType ext);
  int Resize(vtkIdType numTuples);
  void SetNumberOfTuples(vtkIdType number);
  void SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray *source);
  void SetTuple(vtkIdType i, const float *source);
  void SetTuple(vtkIdType i, const double *source);
  void InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray *source);
  void InsertTuple(vtkIdType i, const float *source);
  void InsertTuple(vtkIdType i, const double *source);
  void InsertTuples(vtkIdList *dstIds, vtkIdList *srcIds,
                    vtkAbstractArray *source);
  void InsertTuples(vtkIdType dstStart, vtkIdType n, vtkIdType srcStart,
                    vtkAbstractArray *source);
  vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray *source);
  vtkIdType InsertNextTuple(const float *source);
  vtkIdType InsertNextTuple(const double *source);
  void DeepCopy(vtkAbstractArray *aa);
  void DeepCopy(vtkDataArray *da);
  void InterpolateTuple(vtkIdType i, vtkIdList *ptIndices,
                        vtkAbstractArray *source, double *weights);
  void InterpolateTuple(vtkIdType i, vtkIdType id1, vtkAbstractArray *source1,
                        vtkIdType id2, vtkAbstractArray *source2, double t);
  void SetVariantValue(vtkIdType idx, vtkVariant value);
  void RemoveTuple(vtkIdType id);
  void RemoveFirstTuple();
  void RemoveLastTuple();
  void SetTupleValue(vtkIdType i, const Scalar *t);
  void InsertTupleValue(vtkIdType i, const Scalar *t);
  vtkIdType InsertNextTupleValue(const Scalar *t);
  void SetValue(vtkIdType idx, Scalar value);
  vtkIdType InsertNextValue(Scalar v);
  void InsertValue(vtkIdType idx, Scalar v);

protected:
  vtkStructuredPointsArray();
  ~vtkStructuredPointsArray();

private:
  vtkStructuredPointsArray(
      const vtkStructuredPointsArray &);            // Not implemented.
  void operator=(const vtkStructuredPointsArray &); // Not implemented.

  vtkIdType Lookup(const Scalar &val, vtkIdType startIndex);
  Scalar m_skewMatrix[9] = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
  vtkIdType m_dims[3];
  Scalar m_TempScalarArray[3], m_origin[3], m_spacing[3];
  Mantid::DataObjects::MDHistoWorkspace *m_workspace;
};

#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkVariant.h"
#include "vtkVariantCast.h"
#include "vtkImageData.h"

//------------------------------------------------------------------------------
// Can't use vtkStandardNewMacro on a templated class.
template <class Scalar>
vtkStructuredPointsArray<Scalar> *vtkStructuredPointsArray<Scalar>::New() {
  VTK_STANDARD_NEW_BODY(vtkStructuredPointsArray<Scalar>)
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::PrintSelf(ostream &os,
                                                 vtkIndent indent) {
  this->vtkStructuredPointsArray<Scalar>::Superclass::PrintSelf(os, indent);

  os << indent << "TempScalarArray: " << this->m_TempScalarArray << "\n";
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::InitializeArray(
    Mantid::DataObjects::MDHistoWorkspace *points) {

  m_workspace = points;

  Scalar extent[6];
  extent[0] = m_workspace->getXDimension()->getMinimum();
  extent[1] = m_workspace->getXDimension()->getMaximum();
  extent[2] = m_workspace->getYDimension()->getMinimum();
  extent[3] = m_workspace->getYDimension()->getMaximum();
  extent[4] = m_workspace->getZDimension()->getMinimum();
  extent[5] = m_workspace->getZDimension()->getMaximum();

  m_origin[0] = extent[0];
  m_origin[1] = extent[2];
  m_origin[2] = extent[4];

  m_dims[0] = m_workspace->getXDimension()->getNBins() + 1;
  m_dims[1] = m_workspace->getYDimension()->getNBins() + 1;
  m_dims[2] = m_workspace->getZDimension()->getNBins() + 1;

  m_spacing[0] = (extent[1] - extent[0]) / Scalar(m_dims[0] - 1);
  m_spacing[1] = (extent[3] - extent[2]) / Scalar(m_dims[1] - 1);
  m_spacing[2] = (extent[5] - extent[4]) / Scalar(m_dims[2] - 1);

  this->MaxId = (m_dims[0] * m_dims[1] * m_dims[2]) * 3 - 1;
  this->Size = this->MaxId + 1;
  this->NumberOfComponents = 3;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::InitializeArray(
    Mantid::DataObjects::MDHistoWorkspace *points, const double *skewMatrix) {
  for (auto i = 0; i < 9; ++i) {
    m_skewMatrix[i] = skewMatrix[i];
  }
  this->InitializeArray(points);
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkStructuredPointsArray<Scalar>::Initialize() {
  this->MaxId = -1;
  this->Size = 0;
  this->NumberOfComponents = 3;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::GetTuples(vtkIdList *ptIds,
                                                 vtkAbstractArray *output) {
  vtkDataArray *da = vtkDataArray::FastDownCast(output);
  if (!da) {
    vtkWarningMacro(<< "Input is not a vtkDataArray");
    return;
  }

  if (da->GetNumberOfComponents() != this->GetNumberOfComponents()) {
    vtkWarningMacro(<< "Incorrect number of components in input array.");
    return;
  }

  const vtkIdType numPoints = ptIds->GetNumberOfIds();
  for (vtkIdType i = 0; i < numPoints; ++i) {
    da->SetTuple(i, this->GetTuple(ptIds->GetId(i)));
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::GetTuples(vtkIdType p1, vtkIdType p2,
                                                 vtkAbstractArray *output) {
  vtkDataArray *da = vtkDataArray::FastDownCast(output);
  if (!da) {
    vtkErrorMacro(<< "Input is not a vtkDataArray");
    return;
  }

  if (da->GetNumberOfComponents() != this->GetNumberOfComponents()) {
    vtkErrorMacro(<< "Incorrect number of components in input array.");
    return;
  }

  for (vtkIdType daTupleId = 0; p1 <= p2; ++p1) {
    da->SetTuple(daTupleId++, this->GetTuple(p1));
  }
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkStructuredPointsArray<Scalar>::Squeeze() {
  // noop
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkArrayIterator *vtkStructuredPointsArray<Scalar>::NewIterator() {
  vtkErrorMacro(<< "Not implemented.");
  return NULL;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkStructuredPointsArray<Scalar>::LookupValue(vtkVariant value) {
  bool valid = true;
  Scalar val = vtkVariantCast<Scalar>(value, &valid);
  if (valid) {
    return this->Lookup(val, 0);
  }
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::LookupValue(vtkVariant value,
                                                   vtkIdList *ids) {

   bool valid = true;
   Scalar val = vtkVariantCast<Scalar>(value, &valid);
   ids->Reset();
   if (valid)
   {
   vtkIdType index = 0;
   while ((index = this->Lookup(val, index)) >= 0)
   {
   ids->InsertNextId(index);
   ++index;
   }
   }
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkVariant vtkStructuredPointsArray<Scalar>::GetVariantValue(vtkIdType idx) {
  return vtkVariant(this->GetValue(idx));
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkStructuredPointsArray<Scalar>::ClearLookup() {
  // no-op, no fast lookup implemented.
}

//------------------------------------------------------------------------------
template <class Scalar>
double *vtkStructuredPointsArray<Scalar>::GetTuple(vtkIdType i) {
  this->GetTuple(i, &m_TempScalarArray[0]);
  return &m_TempScalarArray[0];
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::GetTuple(vtkIdType i, double *tuple) {
  this->GetTupleValue(i, tuple);
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkStructuredPointsArray<Scalar>::LookupTypedValue(Scalar value) {
  return this->Lookup(value, 0);
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::LookupTypedValue(Scalar value,
                                                        vtkIdList *ids) {

   ids->Reset();
   vtkIdType index = 0;
   while ((index = this->Lookup(value, index)) >= 0)
   {
   ids->InsertNextId(index);
   ++index;
   }
}

//------------------------------------------------------------------------------
template <class Scalar>
Scalar vtkStructuredPointsArray<Scalar>::GetValue(vtkIdType idx) {
  return this->GetValueReference(idx);
}

//------------------------------------------------------------------------------
template <class Scalar>
Scalar &vtkStructuredPointsArray<Scalar>::GetValueReference(vtkIdType idx) {
  const auto tmp = std::div(idx, static_cast<vtkIdType>(3));
  this->GetTupleValue(tmp.quot, this->m_TempScalarArray);
  return m_TempScalarArray[tmp.rem];
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::GetTupleValue(vtkIdType tupleId,
                                                     Scalar *tuple) {
  // int loc[3];
  // loc[0] = tupleId % m_dims[0];
  // loc[1] = (tupleId / m_dims[0]) % m_dims[1];
  // loc[2] = tupleId / (m_dims[0]*m_dims[1]);
  const auto tmp1 = std::div(tupleId, m_dims[0]);
  const auto tmp2 = std::div(tmp1.quot, m_dims[1]);
  const vtkIdType loc[3] = {tmp1.rem, tmp2.rem, tmp2.quot};

  Scalar v[3];
  for (int i = 0; i < 3; i++) {
    v[i] = m_origin[i] + loc[i] * m_spacing[i];
  }

  // vtkMatrix3x3::MultiplyPoint(m_skewMatrix, v, tuple);
  tuple[0] =
      v[0] * m_skewMatrix[0] + v[1] * m_skewMatrix[1] + v[2] * m_skewMatrix[2];
  tuple[1] =
      v[0] * m_skewMatrix[3] + v[1] * m_skewMatrix[4] + v[2] * m_skewMatrix[5];
  tuple[2] =
      v[0] * m_skewMatrix[6] + v[1] * m_skewMatrix[7] + v[2] * m_skewMatrix[8];
}

//------------------------------------------------------------------------------
template <class Scalar>
int vtkStructuredPointsArray<Scalar>::Allocate(vtkIdType, vtkIdType) {
  vtkErrorMacro("Read only container.") return 0;
}

//------------------------------------------------------------------------------
template <class Scalar>
int vtkStructuredPointsArray<Scalar>::Resize(vtkIdType) {
  vtkErrorMacro("Read only container.") return 0;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::SetNumberOfTuples(vtkIdType) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::SetTuple(vtkIdType, vtkIdType,
                                                vtkAbstractArray *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::SetTuple(vtkIdType, const float *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::SetTuple(vtkIdType, const double *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::InsertTuple(vtkIdType, vtkIdType,
                                                   vtkAbstractArray *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::InsertTuple(vtkIdType, const float *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::InsertTuple(vtkIdType, const double *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::InsertTuples(vtkIdList *, vtkIdList *,
                                                    vtkAbstractArray *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::InsertTuples(vtkIdType, vtkIdType,
                                                    vtkIdType,
                                                    vtkAbstractArray *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType
vtkStructuredPointsArray<Scalar>::InsertNextTuple(vtkIdType,
                                                  vtkAbstractArray *) {
  vtkErrorMacro("Read only container.") return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkStructuredPointsArray<Scalar>::InsertNextTuple(const float *) {

  vtkErrorMacro("Read only container.") return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkStructuredPointsArray<Scalar>::InsertNextTuple(const double *) {
  vtkErrorMacro("Read only container.") return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::DeepCopy(vtkAbstractArray *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::DeepCopy(vtkDataArray *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::InterpolateTuple(vtkIdType, vtkIdList *,
                                                        vtkAbstractArray *,
                                                        double *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::InterpolateTuple(vtkIdType, vtkIdType,
                                                        vtkAbstractArray *,
                                                        vtkIdType,
                                                        vtkAbstractArray *,
                                                        double) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::SetVariantValue(vtkIdType, vtkVariant) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::RemoveTuple(vtkIdType) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::RemoveFirstTuple() {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::RemoveLastTuple() {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::SetTupleValue(vtkIdType,
                                                     const Scalar *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::InsertTupleValue(vtkIdType,
                                                        const Scalar *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType
vtkStructuredPointsArray<Scalar>::InsertNextTupleValue(const Scalar *) {
  vtkErrorMacro("Read only container.") return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::SetValue(vtkIdType, Scalar) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkStructuredPointsArray<Scalar>::InsertNextValue(Scalar) {
  vtkErrorMacro("Read only container.") return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkStructuredPointsArray<Scalar>::InsertValue(vtkIdType, Scalar) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkStructuredPointsArray<Scalar>::vtkStructuredPointsArray() {}

//------------------------------------------------------------------------------
template <class Scalar>
vtkStructuredPointsArray<Scalar>::~vtkStructuredPointsArray() {}

template <class Scalar>
vtkIdType vtkStructuredPointsArray<Scalar>::Lookup(const Scalar &val,
                                                   vtkIdType index) {
  while (index <= this->MaxId) {
    if (this->GetValueReference(++index) == val) {
      return index;
    }
  }
  return -1;
}

#endif // vtkStructuredPointsArray_h

// VTK-HeaderTest-Exclude: vtkStructuredPointsArray.h
