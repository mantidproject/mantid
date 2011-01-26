#ifndef MANTID_KERNEL_MEMORY_H_
#define MANTID_KERNEL_MEMORY_H_

#include <string>

namespace Mantid
{
namespace Kernel
{

/// Convert a (number) for memory in kiB to a string with proper units.
template <typename TYPE>
std::string memToString(const TYPE mem_in_kiB);

/// Adapted from http://stackoverflow.com/questions/669438/how-to-get-memory-usage-at-run-time-in-c
void process_mem_usage(std::size_t & vm_usage, std::size_t & resident_set);

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNELMEMORY_H_ */
