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
/// This function may be registed more than once, so needs to
/// clear the list once it has called all the functions
void CleanupSingletons(void)
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

/// Add s singleton cleanup function to our atexit list
void AddSingleton(atexit_func_t func)
{
    if (cleanup_list == NULL)
    {
	cleanup_list = new std::list<atexit_func_t>;
    }
    cleanup_list->push_front(func);
}

}
}
