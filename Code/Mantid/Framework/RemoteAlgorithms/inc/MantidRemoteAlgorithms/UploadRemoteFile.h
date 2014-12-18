#ifndef UPLOADREMOTEFILE_H_
#define UPLOADREMOTEFILE_H_

#include "MantidAPI/Algorithm.h"
namespace Mantid {
namespace RemoteAlgorithms {
/*** Upload a file to a remote compute resource

    Destination directory depends on the specified transaction ID.  See StartRemoteTransaction.
    Note that there are no workspaces associated with this algorithm.
    
    Input Properties:
    <UL>
    <LI> ComputeResource  - The name of the compute resource the file will be sent to </LI>
    <LI> TransactionID    - ID of the transaction this file belongs to.  See StartRemoteTransaction </LI>
    <LI> LocalFileName    - The name of the file to be uploaded.  This should be the full pathnam
      on the local filesystem. </LI>
    <LI> RemoteFileName   - The name to save the file as on the remote compute resource.  This is only
      name;  the actual path is determined by the compute resource. </LI>
    </UL>

    @author Ross Miller, ORNL
    @date 04/30/2013

    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

        
class DLLExport UploadRemoteFile : public API::Algorithm
{
    public:
    /// (Empty) Constructor
    UploadRemoteFile() : Mantid::API::Algorithm() {}
    /// Virtual destructor
    virtual ~UploadRemoteFile() {}
    /// Algorithm's name
    virtual const std::string name() const { return "UploadRemoteFile"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Uploads a file to the specified compute resource.";}

    /// Algorithm's version
    virtual int version() const { return (1); }
    /// Algorithm's category for identification
    virtual const std::string category() const { return "Remote"; }

    private:
    void init();
    ///Execution code
    void exec();
};

} // end namespace RemoteAlgorithms
} // end namespace Mantid
#endif /*UPLOADREMOTEFILE_H_*/
