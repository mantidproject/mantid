#ifndef REMOTE_SCARFTOMORECONSTRUCTION_H_
#define REMOTE_SCARFTOMORECONSTRUCTION_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace RemoteAlgorithms {
/***
    Algorithm to initiate, query about, or cancel a tomographic
    reconstruction on the SCARF computer cluster at RAL.
    The algorithm can be used to send different commands to the job
    queue, for example: log in, log out, start a reconstruction job,
    retrieve information about jobs or to cancel a job.

    Output Properties: None.
    If the authentication is successfull, a cookie is received that is stored
    internally and re-used for all subsequent interactions with the compute
    resource.

    Copyright &copy; 2014-2015 ISIS Rutherford Appleton Laboratory,
    NScD Oak Ridge National Laboratory & European Spallation Source

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

class SCARFTomoReconstruction : public Mantid::API::Algorithm {
public:
  /// Constructor
  SCARFTomoReconstruction();
  /// Virtual destructor
  virtual ~SCARFTomoReconstruction() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SCARFTomoReconstruction"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Perform a control action on tomographic reconstruction jobs, on "
      "the SCARF computer cluster at RAL, STFC (http://www.scarf.rl.ac.uk/)";
  }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Remote"; }

protected:
  /// different methods (HTTP requests) to process reconstruction job commands
  virtual void doLogin(const std::string &username, const std::string &password);
  virtual void doLogout(const std::string &username);
  virtual bool doPing();
  virtual void doSubmit(const std::string &username);
  virtual void doQueryStatus(const std::string &username);
  virtual void doQueryStatusById(const std::string& username,
                                 const std::string& jobId);
  virtual void doCancel(const std::string &username,
                        const std::string& jobId);
  virtual void doUploadFile(const std::string &username,
                            const std::string &destDir,
                            const std::string &filename);
  virtual void doDownload(const std::string &username,
                          const std::string &jobId,
                          const std::string &fname,
                          const std::string &localDir);

private:
  void init();
  /// Execution code
  void exec();

  // helper for the submit request
  std::string buildSubmitBody(const std::string &appName,
                              const std::string &boundary,
                              const std::string &inputFiles,
                              const std::string &inputArgs);
  /// lower level helper to encode parameters
  void encodeParam(std::string &body, const std::string &boundary,
                   const std::string &paramName, const std::string &paramVal);

  // cookie obtained after logging in
  struct Token {
    Token(std::string& u, std::string& t): m_url(u), m_token_str(t) {};
    std::string m_url;
    std::string m_token_str;
  };
  typedef std::pair<std::string, SCARFTomoReconstruction::Token> UsernameToken;

  /// check if output file is writeable, overwritten, etc.
  const std::string checkDownloadOutputFile(const std::string &localPath,
                                            const std::string &fname);

  /// helper to fetch and save one file from the compute resource
  void getOneJobFile(const std::string &jobId, const std::string &remotePath,
                     const std::string &localPath, const Token &t);

  /// helper to fetch and save all the files for a remote job
  void getAllJobFiles(const std::string &jobId, const std::string &localDir,
                      const Token &t);

  class Action {
  public:
    typedef enum {LOGIN=0, LOGOUT, SUBMIT, QUERYSTATUS, QUERYSTATUSBYID,
                  PING, CANCEL, UPLOAD, DOWNLOAD, UNDEF} Type;
  };

  // helper methods
  Action::Type getAction();

  // options passed to the algorithm
  Action::Type m_action;

  std::string m_jobID;
  std::string m_nxTomoPath;
  std::string m_parameterPath;
  std::string m_outputPath;

  // resource name
  static const std::string m_SCARFComputeResource;

  // HTTP specifics for SCARF (IBM LSF PAC)
  static std::string m_acceptType;

  // store for username-token pairs
  static std::map<std::string, Token> m_tokenStash;
};

} // end namespace RemoteAlgorithms
} // end namespace Mantid
#endif /*REMOTE_SCARFTOMORECONSTRUCTION_H_*/
