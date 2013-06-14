#ifndef SUBMITREMOTEJOB_H_
#define SUBMITREMOTEJOB_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace RemoteAlgorithms {
/*** Submit a job to be executed on the remote compute resource.
    
    Input Properties:
    <UL>
    <LI> ComputeResource  - The name of the compute resource that will execute the job </LI>
    <LI> NumNodes         - The number of nodes to reserve for this job </LI>
    <LI> CoresPerNode     - The number of cores this job will use on each node </LI>
    <LI> TaskName         - A short, human readable identifier for the job </LI>  
    <LI> UserName         - User name on the compute resource </LI>
    <LI> GroupName        - Group name on the compute resource </LI>
    <LI> Password         - Password for the compute resource </LI>
    <LI> TransactionID    - ID of the transaction this job belongs to.  See StartRemoteTransaction </LI>
    <LI> ScriptName       - The name of the script that will actually be executed </LI>
    <LI> ScriptArguments  - Any arguments that should be passed to the script.  (Optional) </LI>
    </UL>

    Output Properties:
    <UL>
    <LI> JobID            - An ID for tracking the status of the submitted job (Queued, Running,
      Completed, Error, etc..) </LI>
    <\UL>
    
    @author Ross Miller, ORNL
    @date 04/30/2013

    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

class SubmitRemoteJob : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  SubmitRemoteJob() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~SubmitRemoteJob() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SubmitRemoteJob"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Remote"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

};

} // end namespace RemoteAlgorithms
} // end namespace Mantid
#endif /*SUBMITREMOTEJOB_H_*/
