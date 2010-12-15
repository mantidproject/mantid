#include "MDDataObjects/IMD_FileFormat.h"
#include "MDDataObjects/MDWorkspace.h"

#include <vector>

namespace Mantid
{
namespace MDDataObjects
{

 Kernel::Logger& IMD_FileFormat::f_log=Kernel::Logger::get("IMD_fileOperations");
//***************************************************************************************

IMD_FileFormat::IMD_FileFormat(const char *file_name):
File_name(file_name)
{
}
} // namespace MDDataObjects
} // namespace Mantid
