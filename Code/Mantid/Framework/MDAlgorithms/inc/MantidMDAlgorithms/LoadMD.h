#ifndef MANTID_MDEVENTS_LOADMD_H_
#define MANTID_MDEVENTS_LOADMD_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include <boost/scoped_ptr.hpp>

namespace Mantid
{
namespace MDAlgorithms
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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport LoadMD : public API::IFileLoader<Kernel::NexusDescriptor>
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

    /// Returns a confidence value that this algorithm can load a file
    int confidence(Kernel::NexusDescriptor & descriptor) const;

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

    /// Helper method
    template<typename MDE, size_t nd>
    void doLoad(typename MDEvents::MDEventWorkspace<MDE, nd>::sptr ws);

    void loadExperimentInfos(boost::shared_ptr<Mantid::API::MultipleExperimentInfos> ws);

    void loadSlab(std::string name, void * data, MDEvents::MDHistoWorkspace_sptr ws, NeXus::NXnumtype dataType);
    void loadHisto();

    void loadDimensions();

    /// Load all the affine matricies
    void loadAffineMatricies(API::IMDWorkspace_sptr ws);
    /// Load a given affine matrix
    API::CoordTransform *loadAffineMatrix(std::string entry_name);

    /// Open file handle
    boost::scoped_ptr<::NeXus::File> m_file;

    /// Name of that file
    std::string m_filename;

    /// Number of dimensions in loaded file
    size_t m_numDims;

    /// Each dimension object loaded.
    std::vector<Mantid::Geometry::IMDDimension_sptr> m_dims;
    ///load only the box structure with empty boxes but do not tload boxes events
    bool m_BoxStructureOnly;

  };


} // namespace MDEvents
} // namespace Mantid

#endif  /* MANTID_MDEVENTS_LOADMD_H_ */
