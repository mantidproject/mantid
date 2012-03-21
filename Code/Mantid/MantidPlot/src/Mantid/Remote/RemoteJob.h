#ifndef REMOTEJOB_H
#define REMOTEJOB_H

/*
 * This is a class for keeping track of remote jobs that have been submitted.
 * Not sure it really needs to be in its own file.
 * This might potentially need to be subclassed for jobs from different job
 * managers (MWS vs. Condor, for example) but I don't think so.
 */


#include <string>
class RemoteJob
{
public:

    // At this point, default constructor, copy constructor and assignment operator are all
    // valid and useful.  If that changes, we'll either need to explicitly implement them
    // or else declare them private.
    
    std::string m_jobId;    // Returned by RemoteJobManager::submitJob()
    int m_status;           // Job is running, held, aborted, etc...  Probably should be an enum
    std::string m_name;     // A meaningful name that can be displayed in the GUI ("Hello World", "NOMAD Reduce", etc..)
    std::string m_stdOut;   // Name (on the remote system) where stdout was written (in case we want to download it)
    std::string m_stdErr;   // Same, but for stderr
};



#endif /* REMOTEJOB_H */
