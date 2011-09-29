#ifndef MANTID_DATAHANDLING_LOAD_H_
#define MANTID_DATAHANDLING_LOAD_H_
/*WIKI* 


The Load algorithm is a more intelligent algorithm than most other load algorithms. When passed a filename it attempts to search the existing load [[:Category:Algorithms|algorithms]] and find the most appropriate to load the given file. The specific load algorithm is then run as a child algorithm with the exception that it logs messages to the Mantid logger.

==== Specific Load Algorithm Properties ====

Each specific loader will have its own properties that are appropriate to it:  SpectrumMin and SpectrumMax for ISIS RAW/NeXuS, FilterByTofMin and FilterByTofMax for Event data. The Load algorithm cannot know about these properties until it has been told the filename and found the correct loader. Once this has happened the properties of the specific Load algorithm are redeclared on to that copy of Load.

== Usage ==

==== Python ====

Given the variable number and types of possible arguments that Load can take, its simple Python function cannot just list the properties as arguments like the others do. Instead the Python function <code>Load</code> can handle any number of arguments. The OutputWorkspace and Filename arguments are the exceptions in that they are always checked for. A snippet regarding usage from the <code>help(Load)</code> is shown below
<div style="border:1pt dashed blue; background:#f9f9f9;padding: 1em 0;">
<source lang="python">
# Simple usage, ISIS NeXus file
Load('INSTR00001000.nxs', 'run_ws')

# ISIS NeXus with SpectrumMin and SpectrumMax = 1
Load('INSTR00001000.nxs', 'run_ws', SpectrumMin=1,SpectrumMax=1)

# SNS Event NeXus with precount on
Load('INSTR_1000_event.nxs', 'event_ws', Precount=True)

# A mix of keyword and non-keyword is also possible
Load('event_ws', Filename='INSTR_1000_event.nxs',Precount=True)
</source></div>


*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IDataFileChecker.h"
#include <list>

namespace Mantid
{
  namespace DataHandling
  { 
   
    /**
    Loads a workspace from a data file. The algorithm tries to determine the actual type
    of the file (raw, nxs, ...) and use the specialized loading algorith to load it.

    @author Roman Tolchenov, Tessella plc
    @date 38/07/2010

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>. 
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */ 
    class DLLExport Load : public API::IDataFileChecker
    {
    public:
      /// Default constructor
      Load();
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "Load"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }
      /// Category
      virtual const std::string category() const { return "DataHandling"; }
      /// Aliases
      virtual const std::string alias() const { return "load"; }
      /// Override setPropertyValue
      virtual void setPropertyValue(const std::string &name, const std::string &value);

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      /// Quick file always returns false here
      virtual bool quickFileCheck(const std::string& filePath,size_t nread,const file_header& header);
      /// file check by looking at the structure of the data file
      virtual int fileCheck(const std::string& filePath);
      /// This method returns shared pointer to a load algorithm which got 
      /// the highest preference after file check. 
      API::IDataFileChecker_sptr getFileLoader(const std::string& filePath);
      /// Declare any additional input properties from the concrete loader
      void declareLoaderProperties(const API::IDataFileChecker_sptr loader);
      
      /// Initialize the static base properties
      void init();
      /// Execute
      void exec();
      /// Overrides the cancel() method to call m_loader->cancel()
      void cancel()const;
      /// Create the concrete instance use for the actual loading.
      API::IDataFileChecker_sptr createLoader(const std::string & name, const double startProgress = -1.0, 
					      const double endProgress=-1.0, const bool logging = true) const;
      /// Set the loader option for use as a sub algorithm.
      void setUpLoader(API::IDataFileChecker_sptr loader, const double startProgress = -1.0, 
		       const double endProgress=-1.0,  const bool logging = true) const;
      /// Set the output workspace(s)
      void setOutputWorkspace(const API::IDataFileChecker_sptr loader);
      /// Retrieve a pointer to the output workspace from the sub algorithm
      API::Workspace_sptr getOutputWorkspace(const std::string & propName, 
					     const API::IDataFileChecker_sptr loader) const;
      

    private:
      /// The base properties
      std::set<std::string> m_baseProps;
      /// The actual loader
      API::IDataFileChecker_sptr m_loader;
     };

  } // namespace DataHandling
} // namespace Mantid

#endif  /*  MANTID_DATAHANDLING_LOAD_H_  */
