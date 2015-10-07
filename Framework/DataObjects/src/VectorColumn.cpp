#include "MantidAPI/ColumnFactory.h"
#include "MantidDataObjects/VectorColumn.h"

namespace Mantid {
namespace DataObjects {
// Please feel free to declare new types as you need them. Syntax is:
// DECLARE_VECTORCOLUMN(type, name-of-the-type);

// IMPORTANT: When you do add new column types, please update the following
// places as well:
//   - ARRAY_TYPES in ITableWorkspace.cpp, so that column is exported to Python
//   properly
//   - IF_VECTOR_COLUMN in NexusFileIO.cpp, so that column is save to Nexus file
//   properly
//   - IF_VECTOR_COLUMN in LoadNexusProcessed.cpp, so that column can be loaded
//   from Nexus file

DECLARE_VECTORCOLUMN(int, vector_int)
DECLARE_VECTORCOLUMN(double, vector_double)
} // namespace DataObjects
} // namespace Mantid
