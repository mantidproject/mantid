#ifndef MANTID_DATAHANDLING_LOAD_H_
#define MANTID_DATAHANDLING_LOAD_H_

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
      /// Override setPropertyValue
      virtual void setPropertyValue(const std::string &name, const std::string &value);

    private:
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
      /// Create the concrete instance use for the actual loading.
      API::IDataFileChecker_sptr createLoader(const std::string & name, const double startProgress = -1.0, 
					      const double endProgress=-1.0, const bool logging = true) const;
      /// Set the output workspace(s)
      void setOutputWorkspace(const API::IDataFileChecker_sptr loader);
      /// Retrieve a pointer to the output workspace from the sub algorithm
      API::Workspace_sptr getOutputWorkspace(const API::IDataFileChecker_sptr loader) const;
      

    private:
      /// The base properties
      std::set<std::string> m_baseProps;
     };

  } // namespace DataHandling
} // namespace Mantid

#endif  /*  MANTID_DATAHANDLING_LOAD_H_  */
