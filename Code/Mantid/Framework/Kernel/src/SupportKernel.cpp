#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cmath>
#include <vector>

#include "MantidKernel/Support.h"
#include "MantidKernel/SupportTempCode.h"

namespace Mantid
{
namespace  StrFunc
{

/// \cond TEMPLATE 

template DLLExport int section(std::string&,double&);
template DLLExport int section(std::string&,float&);
template DLLExport int section(std::string&,int&);
template DLLExport int section(std::string&,std::string&);

template DLLExport int sectPartNum(std::string&,double&);
template DLLExport int sectPartNum(std::string&,int&);
template DLLExport int sectionMCNPX(std::string&,double&);

template DLLExport int convert(const std::string&,double&);
template DLLExport int convert(const std::string&,std::string&);
template DLLExport int convert(const std::string&,int&);
template DLLExport int convert(const char*,std::string&);
template DLLExport int convert(const char*,double&);
template DLLExport int convert(const char*,int&);

template DLLExport int convPartNum(const std::string&,double&);
template DLLExport int convPartNum(const std::string&,int&);

template DLLExport int setValues(const std::string&,const std::vector<int>&,std::vector<double>&);

template DLLExport int writeFile(const std::string&,const double,const std::vector<double>&);
template DLLExport int writeFile(const std::string&,const std::vector<double>&,const std::vector<double>&,const std::vector<double>&);
template DLLExport int writeFile(const std::string&,const std::vector<double>&,const std::vector<double>&);
template DLLExport int writeFile(const std::string&,const std::vector<float>&,const std::vector<float>&);
template DLLExport int writeFile(const std::string&,const std::vector<float>&,const std::vector<float>&,const std::vector<float>&);

/// \endcond TEMPLATE 

}  // NAMESPACE StrFunc

}  // NAMESPACE Mantid
