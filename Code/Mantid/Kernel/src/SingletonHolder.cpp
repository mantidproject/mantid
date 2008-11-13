#include <list>
#include <MantidKernel/SingletonHolder.h>

namespace Mantid
{
namespace Kernel
{

/// List of functions to call on program exit
static std::list<atexit_func_t>* cleanup_list = NULL;

/// Function registed to atexit() that will clean up
/// all our singletons
/// This function may be registed with atexit() more than once, so it needs to
/// clear the list once it has called all the functions
EXPORT_OPT_MANTID_KERNEL void CleanupSingletons()
{
    if (cleanup_list == NULL)
    {
	return;
    }
    std::list<atexit_func_t>::const_iterator it;
    for(it=cleanup_list->begin(); it != cleanup_list->end(); it++)
    {
	(*(*it))();
    }
    delete cleanup_list;
    cleanup_list = NULL;
}

/// Adds singleton cleanup function to our atexit list
/// functions are added to the start of the list so on deletion it is last in, first out
/// @param func Exit function to call - the singleton destructor function
EXPORT_OPT_MANTID_KERNEL void AddSingleton(atexit_func_t func)
{
    if (cleanup_list == NULL)
    {
	cleanup_list = new std::list<atexit_func_t>;
    atexit(&CleanupSingletons);
    }
    cleanup_list->push_front(func);
}

}
}
