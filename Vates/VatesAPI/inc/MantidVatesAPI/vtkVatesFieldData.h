#ifndef VTKVATESFIELDDATA_H
#define VTKVATESFIELDDATA_H

#include <vtkFieldData.h>

class vtkVATESFieldData : public vtkFieldData
{
public:
  static vtkVATESFieldData *New();
  void PrintSelf(ostream& os, vtkIndent indent);
};

#endif // VTKVATESFIELDDATA_H
