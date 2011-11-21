#include "MantidKernel/Glob.h"
#include <Poco/DirectoryIterator.h>

namespace Mantid
{
namespace Kernel
{

    /**
     *    Creates a set of files that match the given pathPattern.
     *   
     *    The path may be give in either Unix, Windows or VMS syntax and
     *    is automatically expanded by calling Path::expand().
     *   
     *    The pattern may contain wildcard expressions even in intermediate
     *    directory names (e.g. /usr/include/<I>*</I> /<I>*</I>*.h).
     *   
     *    Note that, for obvious reasons, escaping characters in a pattern
     *    with a backslash does not work in Windows-style paths.
     *   
     *    Directories that for whatever reason cannot be traversed are
     *    ignored.
     *    
     *    It seems that whatever bug Poco had is fixed now. 
     *    So calling Poco::Glob::glob(pathPattern,files,options) inside.
     *
     *    @param pathPattern :: The search pattern 
     *    @param files :: The names of the files that match the pattern
     *    @param options :: Options
     */
void Glob::glob(const Poco::Path& pathPattern, std::set<std::string>& files, int options)
{
  Poco::Glob::glob(Poco::Path(pathPattern.toString()),files,options);
}

} // namespace Kernel
} // namespace Mantid
