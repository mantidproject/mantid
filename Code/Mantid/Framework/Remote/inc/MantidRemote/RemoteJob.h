#ifndef REMOTEJOB_H
#define REMOTEJOB_H

/*
 * This is a class for keeping track of remote jobs that have been submitted.
 * Not sure it really needs to be in its own file.
 * This might potentially need to be subclassed for jobs from different job
 * managers (MWS vs. Condor, for example) but I don't think so.
 */

#include "MantidRemote/DllConfig.h"
#include "MantidKernel/DateAndTime.h"

#include <string>
#include <map>
class RemoteJobManager;

class MANTID_REMOTE_DLL RemoteJob
{
public:

    enum JobStatus { JOB_COMPLETE, JOB_RUNNING, JOB_QUEUED, JOB_ABORTED, JOB_REMOVED, JOB_DEFERRED, JOB_IDLE, JOB_STATUS_UNKNOWN };
    // Note: make sure JOB_STATUS_UNKNOWN is always the last option.  There's some code that keys on it to set maximum allowable sizes

    RemoteJob( const std::string & jobId, RemoteJobManager * manager, JobStatus status, const std::string &name,
               Mantid::Kernel::DateAndTime submitTime = Mantid::Kernel::DateAndTime())
        : m_jobId( jobId),
          m_manager( manager),
          m_status( status),
          m_algName( name)
    {
      if (submitTime != Mantid::Kernel::DateAndTime())
      {
        m_submitTime = submitTime;
      }
      else
      {
        // default to current time
        m_submitTime = Mantid::Kernel::DateAndTime::getCurrentTime();
      }
      m_startTime = Mantid::Kernel::DateAndTime::minimum();
      m_completionTime = Mantid::Kernel::DateAndTime::minimum();
    }
    // At this point, the default copy constructor and assignment operator are valid and
    // useful.  If that changes, we'll either need to explicitly implement them or else
    // declare them private.

    // Allow for sorting based on the job id
    bool operator< ( const RemoteJob & rval) const { return (m_jobId.compare(rval.m_jobId) < 0); }

    // Set the start time of the job
    void setStartTime(Mantid::Kernel::DateAndTime time) { m_startTime = time; }

    // Set the completion time of the job
    void setCompletionTime(Mantid::Kernel::DateAndTime time) { m_completionTime = time; }

    // Returns a string representation of m_status
    std::string statusString() const
    {
      switch (m_status)
      {
      case JOB_COMPLETE:
        return "Complete";
      case JOB_RUNNING:
        return "Running";
      case JOB_QUEUED:
        return "Queued";
      case JOB_ABORTED:
        return "Aborted";
      case JOB_REMOVED:
        return "Removed";
      case JOB_DEFERRED:
        return "Deferred";
      case JOB_IDLE:
        return "Idle";
      case JOB_STATUS_UNKNOWN:
        return "Unknown";
      }

      // we should never make it here
      return "Impossible job status.  Tell the Mantid developers to update " __FILE__;
    }
    
    std::string m_jobId;            // Returned by RemoteJobManager::submitJob()
    RemoteJobManager *m_manager;    // Pointer to the job manager that was used to submit the job in the first place
    JobStatus m_status;             // Job is running, held, aborted, etc...
    std::string m_algName;          // A meaningful name that can be displayed in the GUI ("Hello World", "NOMAD Reduce", etc..)
    Mantid::Kernel::DateAndTime m_submitTime;   // Time when the job was submitted
    Mantid::Kernel::DateAndTime m_startTime;
    Mantid::Kernel::DateAndTime m_completionTime;

    /************************
    // Not sure if I need these....
    std::string m_stdOut;   // Name (on the remote system) where stdout was written (in case we want to download it)
    std::string m_stdErr;   // Same, but for stderr
    ******************/

private:
    RemoteJob();  // Left unimplemented to ensure nobody can call the default constructor
};



#endif /* REMOTEJOB_H */
