#include "WStoVisConverter.h"

WStoVisConverter::WStoVisConverter(MDWorkspace* ws) : m_ws(ws)
{
}

vtkUnstructuredGrid* WStoVisConverter::convert()
{
	return vtkUnstructuredGrid::New();
}
