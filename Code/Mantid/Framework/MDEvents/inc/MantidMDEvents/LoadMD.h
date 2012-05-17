#ifndef MANTID_MDEVENTS_LOADMD_H_
#define MANTID_MDEVENTS_LOADMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidNexusCPP/NeXusFile.hpp"
#include "MantidAPI/IDataFileChecker.h"

namespace Mantid
{
namespace MDEvents
{

  /** Load a .nxs file into a MDEventWorkspace.
    
    @author Janik Zikovsky
    @date 2011-07-12

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport LoadMD : public API::IDataFileChecker
  {
  public:
    LoadMD();
    ~LoadMD();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "LoadMD";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "MDAlgorithms";}

    /// do a quick check that this file can be loaded
    bool quickFileCheck(const std::string& filePath,size_t nread,const file_header& header);
    /// check the structure of the file and  return a value between 0 and 100 of how much this file can be loaded
    int fileCheck(const std::string& filePath);

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

    /// Helper method
    template<typename MDE, size_t nd>
    void doLoad(typename MDEventWorkspace<MDE, nd>::sptr ws);

    void loadExperimentInfos(boost::shared_ptr<Mantid::API::MultipleExperimentInfos> ws);

    void loadSlab(std::string name, void * data, MDHistoWorkspace_sptr ws, NeXus::NXnumtype dataType);
    void loadHisto();

    void loadDimensions();

    /// Open file handle
    ::NeXus::File * file;

    /// Name of that file
    std::string m_filename;

    /// Number of dimensions in loaded file
    size_t m_numDims;

    /// Each dimension object loaded.
    std::vector<Mantid::Geometry::IMDDimension_sptr> m_dims;

  };


} // namespace MDEvents
} // namespace Mantid

#endif  /* MANTID_MDEVENTS_LOADMD_H_ */
