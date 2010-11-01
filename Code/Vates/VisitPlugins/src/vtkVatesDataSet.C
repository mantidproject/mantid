#include "vtkVatesDataSet.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkVatesDataSet, "1.00");
vtkStandardNewMacro(vtkVatesDataSet);


void vtkVatesDataSet::CopyMetaData(vtkVatesDataSet* other)
{

}

//void vtkVatesDataSet::SetMDFilePath(std::string mdFilePath)
//{
	//this->m_MDFilePath = mdFilePath;
//}

std::string vtkVatesDataSet::GetMDFilePath()
{
	//return this->m_MDFilePath;
	return "";
}
