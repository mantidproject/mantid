/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMDHWPointsArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkMDHWPointsArray - Map native Exodus II results arrays
// into the vtkDataArray interface.
//
// .SECTION Description
// Map native Exodus II results arrays into the vtkDataArray interface. Use
// the vtkCPExodusIIInSituReader to read an Exodus II file's data into this
// structure.

#ifndef vtkMDHWPointsArray_h
#define vtkMDHWPointsArray_h

#include "vtkMappedDataArray.h"

#include "vtkTypeTemplate.h"  // For templated vtkObject API
#include "vtkObjectFactory.h" // for vtkStandardNewMacro
#include "vtkIdList.h"
#include "vtkVariant.h"
#include "vtkVariantCast.h"
//#include <vtkMatrix3x3.h>

#include "MantidDataObjects/MDHistoWorkspace.h"

#include <cstdlib>

namespace Mantid {
namespace VATES {

template <class Scalar>
class vtkMDHWPointsArray : public vtkTypeTemplate<vtkMDHWPointsArray<Scalar>,
                                                  vtkMappedDataArray<Scalar>> {
public:
  vtkMappedDataArrayNewInstanceMacro(
      vtkMDHWPointsArray<Scalar>) static vtkMDHWPointsArray *New();
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
  vtkMDHWPointsArray();
  ~vtkMDHWPointsArray();

private:
  vtkMDHWPointsArray(const vtkMDHWPointsArray &); // Not implemented.
  void operator=(const vtkMDHWPointsArray &);     // Not implemented.

  template <class otherScalar>
  void GetAnyScalarTupleValue(vtkIdType idx, otherScalar *t);
  vtkIdType Lookup(const Scalar &val, vtkIdType startIndex);
  Scalar m_skewMatrix[9] = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
  vtkIdType m_dims[3];
  Scalar m_TempScalarArray[3], m_origin[3], m_spacing[3];
  double m_tempDoubleArray[3];
  Mantid::DataObjects::MDHistoWorkspace *m_workspace;
};

//------------------------------------------------------------------------------
// Can't use vtkStandardNewMacro on a templated class.
template <class Scalar>
vtkMDHWPointsArray<Scalar> *vtkMDHWPointsArray<Scalar>::New() {
  VTK_STANDARD_NEW_BODY(vtkMDHWPointsArray<Scalar>)
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::PrintSelf(ostream &os, vtkIndent indent) {
  this->vtkMDHWPointsArray<Scalar>::Superclass::PrintSelf(os, indent);

  os << indent << "TempScalarArray: " << this->m_TempScalarArray << "\n";
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::InitializeArray(
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
void vtkMDHWPointsArray<Scalar>::InitializeArray(
    Mantid::DataObjects::MDHistoWorkspace *points, const double *skewMatrix) {
  for (auto i = 0; i < 9; ++i) {
    m_skewMatrix[i] = skewMatrix[i];
  }
  this->InitializeArray(points);
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkMDHWPointsArray<Scalar>::Initialize() {
  this->MaxId = -1;
  this->Size = 0;
  this->NumberOfComponents = 3;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::GetTuples(vtkIdList *ptIds,
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
void vtkMDHWPointsArray<Scalar>::GetTuples(vtkIdType p1, vtkIdType p2,
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
    da->SetTuple(daTupleId, this->GetTuple(p1));
    ++daTupleId;
  }
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkMDHWPointsArray<Scalar>::Squeeze() {
  // noop
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkArrayIterator *vtkMDHWPointsArray<Scalar>::NewIterator() {
  vtkErrorMacro(<< "Not implemented.");
  return NULL;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkMDHWPointsArray<Scalar>::LookupValue(vtkVariant value) {
  bool valid = true;
  Scalar val = vtkVariantCast<Scalar>(value, &valid);
  if (valid) {
    return this->Lookup(val, 0);
  }
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::LookupValue(vtkVariant value, vtkIdList *ids) {

  bool valid = true;
  Scalar val = vtkVariantCast<Scalar>(value, &valid);
  ids->Reset();
  if (valid) {
    vtkIdType index = 0;
    while ((index = this->Lookup(val, index)) >= 0) {
      ids->InsertNextId(index);
      ++index;
    }
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkVariant vtkMDHWPointsArray<Scalar>::GetVariantValue(vtkIdType idx) {
  return vtkVariant(this->GetValue(idx));
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkMDHWPointsArray<Scalar>::ClearLookup() {
  // no-op, no fast lookup implemented.
}

//------------------------------------------------------------------------------
template <class Scalar>
double *vtkMDHWPointsArray<Scalar>::GetTuple(vtkIdType i) {
  this->GetAnyScalarTupleValue(i, &m_tempDoubleArray[0]);
  return &m_tempDoubleArray[0];
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::GetTuple(vtkIdType i, double *tuple) {
  this->GetAnyScalarTupleValue(i, tuple);
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkMDHWPointsArray<Scalar>::LookupTypedValue(Scalar value) {
  return this->Lookup(value, 0);
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::LookupTypedValue(Scalar value,
                                                  vtkIdList *ids) {

  ids->Reset();
  vtkIdType index = 0;
  while ((index = this->Lookup(value, index)) >= 0) {
    ids->InsertNextId(index);
    ++index;
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
Scalar vtkMDHWPointsArray<Scalar>::GetValue(vtkIdType idx) {
  return this->GetValueReference(idx);
}

//------------------------------------------------------------------------------
template <class Scalar>
Scalar &vtkMDHWPointsArray<Scalar>::GetValueReference(vtkIdType idx) {
  const auto tmp = std::div(idx, static_cast<vtkIdType>(3));
  this->GetTupleValue(tmp.quot, this->m_TempScalarArray);
  return m_TempScalarArray[tmp.rem];
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::GetTupleValue(vtkIdType tupleId,
                                               Scalar *tuple) {
  this->GetAnyScalarTupleValue(tupleId, tuple);
}

template <class Scalar>
template <class otherScalar>
void vtkMDHWPointsArray<Scalar>::GetAnyScalarTupleValue(vtkIdType tupleId,
                                                        otherScalar *tuple) {

  const auto tmp1 = std::div(tupleId, m_dims[0]);
  const auto tmp2 = std::div(tmp1.quot, m_dims[1]);
  const Scalar loc[3] = {static_cast<Scalar>(tmp1.rem),
                         static_cast<Scalar>(tmp2.rem),
                         static_cast<Scalar>(tmp2.quot)};

  Scalar v0 = m_origin[0] + loc[0] * m_spacing[0];
  Scalar v1 = m_origin[1] + loc[1] * m_spacing[1];
  Scalar v2 = m_origin[2] + loc[2] * m_spacing[2];

  // vtkMatrix3x3::MultiplyPoint(m_skewMatrix, v, tuple);
  tuple[0] = v0 * m_skewMatrix[0] + v1 * m_skewMatrix[1] + v2 * m_skewMatrix[2];
  tuple[1] = v0 * m_skewMatrix[3] + v1 * m_skewMatrix[4] + v2 * m_skewMatrix[5];
  tuple[2] = v0 * m_skewMatrix[6] + v1 * m_skewMatrix[7] + v2 * m_skewMatrix[8];
}

template <class Scalar>
vtkIdType vtkMDHWPointsArray<Scalar>::Lookup(const Scalar &val,
                                             vtkIdType index) {
  while (index <= this->MaxId) {
    if (this->GetValueReference(index) == val) {
      return index;
    }
    index++;
  }
  return -1;
}
//------------------------------------------------------------------------------
template <class Scalar>
int vtkMDHWPointsArray<Scalar>::Allocate(vtkIdType, vtkIdType) {
  vtkErrorMacro("Read only container.") return 0;
}

//------------------------------------------------------------------------------
template <class Scalar> int vtkMDHWPointsArray<Scalar>::Resize(vtkIdType) {
  vtkErrorMacro("Read only container.") return 0;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::SetNumberOfTuples(vtkIdType) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::SetTuple(vtkIdType, vtkIdType,
                                          vtkAbstractArray *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::SetTuple(vtkIdType, const float *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::SetTuple(vtkIdType, const double *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::InsertTuple(vtkIdType, vtkIdType,
                                             vtkAbstractArray *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::InsertTuple(vtkIdType, const float *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::InsertTuple(vtkIdType, const double *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::InsertTuples(vtkIdList *, vtkIdList *,
                                              vtkAbstractArray *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::InsertTuples(vtkIdType, vtkIdType, vtkIdType,
                                              vtkAbstractArray *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkMDHWPointsArray<Scalar>::InsertNextTuple(vtkIdType,
                                                      vtkAbstractArray *) {
  vtkErrorMacro("Read only container.") return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkMDHWPointsArray<Scalar>::InsertNextTuple(const float *) {

  vtkErrorMacro("Read only container.") return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkMDHWPointsArray<Scalar>::InsertNextTuple(const double *) {
  vtkErrorMacro("Read only container.") return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::DeepCopy(vtkAbstractArray *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::DeepCopy(vtkDataArray *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::InterpolateTuple(vtkIdType, vtkIdList *,
                                                  vtkAbstractArray *,
                                                  double *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::InterpolateTuple(vtkIdType, vtkIdType,
                                                  vtkAbstractArray *, vtkIdType,
                                                  vtkAbstractArray *, double) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::SetVariantValue(vtkIdType, vtkVariant) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::RemoveTuple(vtkIdType) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkMDHWPointsArray<Scalar>::RemoveFirstTuple() {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkMDHWPointsArray<Scalar>::RemoveLastTuple() {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::SetTupleValue(vtkIdType, const Scalar *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::InsertTupleValue(vtkIdType, const Scalar *) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkMDHWPointsArray<Scalar>::InsertNextTupleValue(const Scalar *) {
  vtkErrorMacro("Read only container.") return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::SetValue(vtkIdType, Scalar) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkMDHWPointsArray<Scalar>::InsertNextValue(Scalar) {
  vtkErrorMacro("Read only container.") return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWPointsArray<Scalar>::InsertValue(vtkIdType, Scalar) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class Scalar> vtkMDHWPointsArray<Scalar>::vtkMDHWPointsArray() {}

//------------------------------------------------------------------------------
template <class Scalar> vtkMDHWPointsArray<Scalar>::~vtkMDHWPointsArray() {}
}
}

#endif // vtkMDHWPointsArray_h

// VTK-HeaderTest-Exclude: vtkMDHWPointsArray.h
