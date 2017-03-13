/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMDHWSignalArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkMDHWSignalArray - Map native Exodus II results arrays
// into the vtkDataArray interface.
//
// .SECTION Description
// Map native Exodus II results arrays into the vtkDataArray interface. Use
// the vtkCPExodusIIInSituReader to read an Exodus II file's data into this
// structure.

#ifndef vtkMDHWSignalArray_h
#define vtkMDHWSignalArray_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkGenericDataArray.h"
#include "vtkObjectFactory.h"

#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidVatesAPI/Normalization.h"

namespace Mantid {
namespace VATES {

template <class ValueTypeT>
class vtkMDHWSignalArray
    : public vtkGenericDataArray<vtkMDHWSignalArray<ValueTypeT>, ValueTypeT> {
  typedef vtkGenericDataArray<vtkMDHWSignalArray<ValueTypeT>, ValueTypeT>
      GenericDataArrayType;

public:
  typedef vtkMDHWSignalArray<ValueTypeT> SelfType;
  vtkAbstractTypeMacro(SelfType, GenericDataArrayType) typedef
      typename Superclass::ValueType ValueType;

  static vtkMDHWSignalArray *New();
  vtkMDHWSignalArray(const vtkMDHWSignalArray &) = delete;
  void operator=(const vtkMDHWSignalArray &) = delete;

  void InitializeArray(Mantid::DataObjects::MDHistoWorkspace *ws,
                       VisualNormalization normalization, std::size_t offset);
  ValueTypeT GetValue(vtkIdType valueIdx) const;
  void SetValue(vtkIdType valueIdx, ValueTypeT value);
  void GetTypedTuple(vtkIdType tupleIdx, ValueTypeT *tuple) const;
  void SetTypedTuple(vtkIdType tupleIds, const ValueTypeT *tuple);
  ValueTypeT GetTypedComponent(vtkIdType tupleIdx, int compIdx) const;
  void SetTypedComponent(vtkIdType tupleIdx, int compIdx, ValueTypeT value);

  int Allocate(vtkIdType size, vtkIdType ext = 1000) override;
  int Resize(vtkIdType numTuples) override;
  // Description:
  // This container is read only -- this method does nothing but print a
  // warning.
  void InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray *source) override;
  void InsertTuple(vtkIdType i, const float *source) override;
  void InsertTuple(vtkIdType i, const double *source) override;
  void InsertTuples(vtkIdList *dstIds, vtkIdList *srcIds,
                    vtkAbstractArray *source) override;
  void InsertTuples(vtkIdType dstStart, vtkIdType n, vtkIdType srcStart,
                    vtkAbstractArray *source) override;
  void Squeeze() override;
protected:
  bool AllocateTuples(vtkIdType numTuples);
  bool ReallocateTuples(vtkIdType numTuples);
  vtkMDHWSignalArray() = default;
  ~vtkMDHWSignalArray() = default;
  vtkObjectBase *NewInstanceInternal() const override {
    if (vtkDataArray *da = vtkDataArray::CreateDataArray(
            vtkMDHWSignalArray<ValueTypeT>::VTK_DATA_TYPE)) {
      return da;
    }
    return vtkMDHWSignalArray<ValueTypeT>::New();
  }
private:
  friend class vtkGenericDataArray<vtkMDHWSignalArray<ValueTypeT>, ValueTypeT>;
  vtkIdType m_offset{0};
  API::MDNormalization m_normalization;
  DataObjects::MDHistoWorkspace *m_ws;
};

template <class ValueTypeT>
vtkMDHWSignalArray<ValueTypeT> *vtkMDHWSignalArray<ValueTypeT>::New() {
  VTK_STANDARD_NEW_BODY(vtkMDHWSignalArray<ValueTypeT>);
}

//------------------------------------------------------------------------------
template <class ValueTypeT>
void vtkMDHWSignalArray<ValueTypeT>::InitializeArray(
    Mantid::DataObjects::MDHistoWorkspace *ws,
    Mantid::VATES::VisualNormalization normalization, std::size_t offset) {
  this->NumberOfComponents = 1;
  this->m_ws = ws;
  this->m_offset = offset;

  const vtkIdType nBinsX = m_ws->getXDimension()->getNBins();
  const vtkIdType nBinsY = m_ws->getYDimension()->getNBins();
  const vtkIdType nBinsZ = m_ws->getZDimension()->getNBins();
  this->Size = nBinsX * nBinsY * nBinsZ;
  this->MaxId = this->Size - 1;

  if (normalization == AutoSelect) {
    // enum to enum.
    m_normalization =
        static_cast<API::MDNormalization>(m_ws->displayNormalization());
  } else {
    m_normalization = static_cast<API::MDNormalization>(normalization);
  }
}

//------------------------------------------------------------------------------
template <class ValueTypeT>
ValueTypeT vtkMDHWSignalArray<ValueTypeT>::GetValue(vtkIdType idx) const {
  auto pos = m_offset + idx;
  switch (m_normalization) {
  case API::NoNormalization:
    return m_ws->getSignalAt(pos);
  case API::VolumeNormalization:
    return m_ws->getSignalAt(pos) * m_ws->getInverseVolume();
  case API::NumEventsNormalization:
    return m_ws->getSignalAt(pos) / m_ws->getNumEventsAt(pos);
  }
  // Should not reach here
  return std::numeric_limits<signal_t>::quiet_NaN();
}

//------------------------------------------------------------------------------
template <class ValueTypeT>
void vtkMDHWSignalArray<ValueTypeT>::GetTypedTuple(vtkIdType tupleId,
                                                   ValueTypeT *tuple) const {
  tuple[0] = this->GetValue(tupleId);
}

//------------------------------------------------------------------------------
template <class ValueTypeT>
ValueTypeT
vtkMDHWSignalArray<ValueTypeT>::GetTypedComponent(vtkIdType tupleIdx,
                                                  int compIdx) const {
  assert(compIdx == 0);
  (void)compIdx;
  return this->GetValue(tupleIdx);
}

//------------------------------------------------------------------------------
template <class ValueTypeT>
void vtkMDHWSignalArray<ValueTypeT>::SetValue(vtkIdType, ValueTypeT) {
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class ValueTypeT>
void vtkMDHWSignalArray<ValueTypeT>::SetTypedComponent(vtkIdType, int,
                                                       ValueTypeT) {
  vtkErrorMacro("Read only container.") return;
}

//------------------------------------------------------------------------------
template <class ValueTypeT>
bool vtkMDHWSignalArray<ValueTypeT>::AllocateTuples(vtkIdType) {
  vtkErrorMacro("Read only container.") return true;
}

//------------------------------------------------------------------------------
template <class ValueTypeT>
bool vtkMDHWSignalArray<ValueTypeT>::ReallocateTuples(vtkIdType) {
  vtkErrorMacro("Read only container.") return false;
}

//------------------------------------------------------------------------------
template <class Scalar>
int vtkMDHWSignalArray<Scalar>::Allocate(vtkIdType, vtkIdType) {
  vtkErrorMacro("Read only container.") return 0;
}

//------------------------------------------------------------------------------
template <class Scalar> int vtkMDHWSignalArray<Scalar>::Resize(vtkIdType) {
  vtkErrorMacro("Read only container.") return 0;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWSignalArray<Scalar>::InsertTuple(vtkIdType, vtkIdType,
                                             vtkAbstractArray *) {
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWSignalArray<Scalar>::InsertTuple(vtkIdType, const float *) {
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWSignalArray<Scalar>::InsertTuple(vtkIdType, const double *) {
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWSignalArray<Scalar>::InsertTuples(vtkIdList *, vtkIdList *,
                                              vtkAbstractArray *) {
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMDHWSignalArray<Scalar>::InsertTuples(vtkIdType, vtkIdType, vtkIdType,
                                              vtkAbstractArray *) {
  vtkErrorMacro("Read only container.");
}

template <class Scalar> void vtkMDHWSignalArray<Scalar>::Squeeze() {
  // noop
}

}
}
#endif // vtkMDHWSignalArray_h

// VTK-HeaderTest-Exclude: vtkMDHWSignalArray.h
