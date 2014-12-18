#ifndef MANTID_DATAHANDLING_LOADNXSPE_H_
#define MANTID_DATAHANDLING_LOADNXSPE_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/IFileLoader.h"

namespace Mantid
{
namespace DataHandling
{

  /** LoadNXSPE : Algorithm to load an NXSPE file into a workspace2D. It will create a "new" instrument, that can be overwritten later by the LoadInstrument algorithm
    Properties:
    <ul>
    <li>Filename  - the name of the file to read from.</li>
    <li>Workspace - the workspace name that will be created and hold the loaded data.</li>
    </ul>
    @author Andrei Savici, ORNL
    @date 2011-08-14

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport LoadNXSPE  : public API::IFileLoader<Kernel::NexusDescriptor>
  {
  public:
    LoadNXSPE();
    ~LoadNXSPE();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "LoadNXSPE";};
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return " Algorithm to load an NXSPE file into a workspace2D.";}

    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "DataHandling\\Nexus;DataHandling\\SPE;Inelastic";}

    /// Returns a confidence value that this algorithm can load a file
    virtual int confidence(Kernel::NexusDescriptor & descriptor) const;

    /// Confidence in identifier.
    static int identiferConfidence(const std::string& value);

  private:
    
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();
    /// Function to return a cuboid shape, with widths dx,dy,dz
    Geometry::Object_sptr createCuboid(double dx,double dy, double dz);


  };


} // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_LOADNXSPE_H_ */
