#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cmath>
#include <vector>

#include "MantidGeometry/Math/Matrix.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/Support.h"
#include "MantidKernel/SupportTempCode.h"

namespace Mantid
{
namespace  StrFunc
{

/// \cond TEMPLATE 

template DLLExport int section(std::string&,Geometry::V3D&);

template DLLExport int convert(const std::string&,Geometry::V3D&);

/// \endcond TEMPLATE 

}  // NAMESPACE StrFunc

}  // NAMESPACE Mantid
