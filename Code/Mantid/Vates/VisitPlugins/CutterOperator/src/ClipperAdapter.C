#include "ClipperAdapter.h"
#include "vtkVisItClipper.h"
#include "vtkUnstructuredGrid.h"

ClipperAdapter::ClipperAdapter(vtkVisItClipper* pClipper) :
  m_clipper(pClipper) //Initialised with adaptee.
{
}

void ClipperAdapter::SetInput(::vtkDataSet* in_ds)
{
  m_clipper->SetInput(in_ds);
}

void ClipperAdapter::SetClipFunction(vtkImplicitFunction* func)
{
  m_clipper->SetClipFunction(func);
}

void ClipperAdapter::SetInsideOut(bool insideout)
{
  m_clipper->SetInsideOut(insideout);
}

void ClipperAdapter::SetRemoveWholeCells(bool removeWholeCells)
{
  m_clipper->SetRemoveWholeCells(removeWholeCells);
}

void ClipperAdapter::SetOutput(vtkUnstructuredGrid* out_ds)
{
  m_clipper->SetOutput(out_ds);
}

void ClipperAdapter::Update()
{
  m_clipper->Update();
}

void ClipperAdapter::Delete()
{
  delete this;
}

ClipperAdapter::~ClipperAdapter()
{
  m_clipper->Delete();
}
