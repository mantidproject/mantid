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
    *    @param base :: The base directory: the search is done only within this directory. base cannot 
    *                contain wildcards.
    *    @param pathPattern :: The search pattern 
    *    @param files :: The names of the files that match the pattern
    *    @param options :: Options
    */
void Glob::glob(const std::string& base, const std::string& pathPattern, std::set<std::string>& files, int options)
{
    glob(Poco::Path(Poco::Path::expand(base), Poco::Path::PATH_GUESS),Poco::Path(Poco::Path::expand(pathPattern), Poco::Path::PATH_GUESS), files, options);
}

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
     *    @param pathPattern :: The search pattern 
     *    @param files :: The names of the files that match the pattern
     *    @param options :: Options
     */
void Glob::glob(const Poco::Path& pathPattern, std::set<std::string>& files, int options)
{
  Poco::Path pattern(pathPattern);
  pattern.makeDirectory(); // to simplify pattern handling later on
  Poco::Path base(pattern);
  Poco::Path absBase(base);
  absBase.makeAbsolute();
  Poco::Path testBase(base);
  while (base.depth() > 0 && base[base.depth() - 1] != "..") 
  {
    try
    {
      testBase.popDirectory();
      Poco::DirectoryIterator it(testBase);
    }
    catch(...)
    {
      break;
    }
    base.popDirectory();
    absBase.popDirectory();
  }
  if (pathPattern.isDirectory()) options |= GLOB_DIRS_ONLY;
  collect(pattern, absBase, base, pathPattern[base.depth()], files, options);             
}

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
     *    @param base :: The base directory: the search is done only within this directory. base cannot 
     *                contain wildcards.
     *    @param pathPattern :: The search pattern 
     *    @param files :: The names of the files that match the pattern
     *    @param options :: Options
     */
void Glob::glob(const Poco::Path& base, const Poco::Path& pathPattern, std::set<std::string>& files, int options)
{
  Poco::Path pattern(pathPattern);
  pattern.makeDirectory(); // to simplify pattern handling later on
  Poco::Path base1(base);
  base1.makeDirectory();
  Poco::Path absBase(base1);
  absBase.makeAbsolute();
  
  if (pathPattern.isDirectory()) options |= GLOB_DIRS_ONLY;
  collect(pattern, absBase, base1, pathPattern[base1.depth()], files, options);           
}

} // namespace Kernel
} // namespace Mantid
