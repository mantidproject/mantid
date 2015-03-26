#ifndef MANTID_KERNEL_IREMOTEJOBMANAGER_H
#define MANTID_KERNEL_IREMOTEJOBMANAGER_H

#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DateAndTime.h"

namespace Mantid {
namespace API {
/**
Common interface to different remote job managers (job schedulers, web
services, etc. such as MOAB, Platform LSF, or SLURM).

IremoteJobManager objects are (in principle) created via the
RemoteJobManagerFactory.  There are several "remote algorithms" in
Mantid: Authenticate, SubmitRemoteJob, QueryRemoteJobStatus,
etc. These algorithms are meant to use this interface to the different
specific implementations. Or, from the opposite angle, the methods of
this interface provide the functionality required by the remote
algorithms in a generic way (with respect to different job schedulers
or underlying mechanisms to handle remote jobs). So-called remote job
manager classes can implement this interface to provide
specialisations for Platform LSF, SLURM, MOAB, the Mantid web service
API, etc.

A typical sequence of calls when you use this interface would be:

1) Authenticate/log-in (authenticate())
2) Do transactions

Where the sequence of calls within a transaction is:

2.1) Start transaction (startRemoteTransaction())
2.2) Do actions
2.3) Stop transaction (stopRemoteTransaction())

In 2.2, several types of actions are possible:
- Submit a job to run on the (remote) compute resource (submitRemoteJob()).
- Get status info for one or all jobs (queryRemoteJob() and
queryAllRemoteJobs()).
- Cancel a job (abortRemoteJob()).
- Get list of available files for a transaction on the compute resource
(queryRemoteFile())
- Upload / download files ( uploadRemoteFile() and downloadRemoteFile()).


Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_API_DLL IRemoteJobManager {
public:
  virtual ~IRemoteJobManager(){};

  /**
   * Status and general information about jobs running on (remote)
   * compute resources.
   */
  struct RemoteJobInfo {
    /// Job ID, usually assigned by a job scheduler as an integer
    /// number or similar.
    std::string id;
    /// name of the job, whether given by the user or automatically
    /// assigned by the job scheduler
    std::string name;
    /// Name of the script or executable. Depending on the specific
    /// implementation, job scheduler, etc. this can be an
    /// 'application' name, a script name or different ways of
    /// specifying what is run
    std::string runnableName;
    /// Last status retrieved (typically: Pending, Running, Exited,
    /// etc.). The values are implementation/job scheduler dependent.
    std::string status;
    /// ID of the transaction where this job is included
    std::string transactionID;
    /// Date-time of submission. No particular format can be assumed
    /// from the specific remote job managers, and some of them may
    /// not provide this info
    Mantid::Kernel::DateAndTime submitDate;
    /// Date-time the job actually started running.  No particular
    /// format can be assumed
    Mantid::Kernel::DateAndTime startDate;
    /// Date-time the job finished. No particular format can be
    /// assumed
    Mantid::Kernel::DateAndTime completionTime;
  };

  /**
   * Authenticate or log-in, previous to submitting jobs, up/downloading, etc.
   *
   * @param username User name or credentials
   *
   * @param password Password (or other type of authentication token)
   * string.
   *
   * @throws std::invalid_argument If any of the inputs is not set
   * properly.
   * @throws std::runtime_error If authentication fails
   */
  virtual void authenticate(const std::string &username,
                            const std::string &password) = 0;

  /**
   * Submit a job (and implicitly request to start it) within a
   * transaction.
   *
   * @param transactionID ID obtained from a startRemoteTransaction()
   *
   * @param runnable Name of the script or executable for the
   * job. This can be a name or path to a file (implementation
   * dependent).
   *
   * @param param Parameters for the job. This is implementation
   * dependent and may be a list of command line options, the name of
   * a script or configuration file, the contents of a script to run
   * or configuration template, etc. For example, for the Mantid web
   * service API, this is the content of a python script.
   *
   * @param taskName (optional) human readable name for this job.
   *
   * @param numNodes number of nodes to use (optional and dependent on
   * implementation and compute resource)
   *
   * @parm coresPerNode number of cores to use in each node (optional
   * and dependent on implemenation and compute resource)
   *
   * @return jobID string for the job started (if successful).
   *
   * @throws std::invalid_argument If any of the inputs is not set
   * properly.
   * @throws std::runtime_error if job submission fails.
   */
  virtual std::string
  submitRemoteJob(const std::string &transactionID, const std::string &runnable,
                  const std::string &param, const std::string &taskName = "",
                  const int numNodes = 1, const int coresPerNode = 1) = 0;

  /**
   * Get/download a file from the (remote) compute resource.
   *
   * @param transactionID ID obtained from a startRemoteTransaction()
   *
   * @param remoteFileName Name of file on the (remote) compute
   * resource. This can be a full or relative path or a simple file
   * name, depending on implementation.
   *
   * @param localFileName Where to place the downloaded file on the
   * local machine.
   *
   * @throws std::invalid_argument If any of the inputs is not set
   * properly.
   * @throws std::runtime_error If the download operation fails
   */
  virtual void downloadRemoteFile(const std::string &transactionID,
                                  const std::string &remoteFileName,
                                  const std::string &localFileName) = 0;

  /**
   * Get information (status etc.) for all running jobs on the remote
   * compute resource
   *
   * @return Status and general info for all the jobs found on the
   * (remote) compute resource. Each of them should come identified by
   * its ID.
   *
   * @throws std::runtime_error If the query fails
   */
  virtual std::vector<RemoteJobInfo> queryAllRemoteJobs() const = 0;

  /**
   * Get the list of files available for a transaction at the (remote)
   * compute resource.
   *
   * @param transactionID ID obtained from startRemoteTransaction()
   *
   * @return The names of all the available files
   *
   * @throws std::invalid_argument If there's an issue with the
   * transaction ID
   *
   * @throws std::runtime_error If the query fails
   */
  virtual std::vector<std::string>
  queryRemoteFile(const std::string &transactionID) const = 0;

  /**
   * Get information (status etc.) for an (in principle) running job
   *
   * @param jobID ID of a job as obtained from submitRemoteJob()
   *
   * @return Status and general info for the job requested
   *
   * @throws std::invalid_argument If there's an issue with the
   * job ID
   *
   * @throws std::runtime_error If the query fails
   */
  virtual RemoteJobInfo queryRemoteJob(const std::string &jobID) const = 0;

  /**
   * Start a transaction before up/downloading files and submitting
   * jobs
   *
   * @return ID of the transaction as produced by the job scheduler
   * and/or remote job manager.
   *
   * @throws std::runtime_error If the transaction creation fails
   */
  virtual std::string startRemoteTransaction() = 0;

  /**
   * Finish a transaction. This implicitly can cancel all the
   * operations (jobs) associated with this transaction.
   *
   * @param transactionID An Id of a transaction, as returned by
   * startRemoteTransaction()
   *
   * @throws std::invalid_argument If there's an issue with the
   * transaction ID
   *
   * @throws std::runtime_error If the stop operation fails
   */
  virtual void stopRemoteTransaction(const std::string &transactionID) = 0;

  /**
   * Cancel a job (expected to be currently running on the remote resource)
   *
   * @param jobID ID for a job in a transaction, as returned by
   * submitRemoteJob()
   *
   * @throws std::invalid_argument If there's an issue with the
   * job ID
   * @throws std::runtime_error If the abort/cancel operation fails
   */
  virtual void abortRemoteJob(const std::string &jobID) = 0;

  /**
   * Upload file for a transaction on the rmeote compute resource
   *
   * @param transactionID ID, as you get them from
   * startRemoteTransaction()
   *
   * @param remoteFileName Name of file on the (remote) compute
   * resource. This can be a full or relative path or a simple file
   * name, depending on implementation.
   *
   * @param localFileName Path to the file to upload
   *
   * @throws std::invalid_argument If there's an issue with the
   * arguments passed
   * @throws std::runtime_error If the upload fails
   */
  virtual void uploadRemoteFile(const std::string &transactionID,
                                const std::string &remoteFileName,
                                const std::string &localFileName) = 0;
};

// shared pointer type for the IRemoteJobManager
typedef boost::shared_ptr<IRemoteJobManager> IRemoteJobManager_sptr;

} // namespace API
} // namespace Mantid

#endif // MANTID_API_IREMOTEJOBMANAGER_H
