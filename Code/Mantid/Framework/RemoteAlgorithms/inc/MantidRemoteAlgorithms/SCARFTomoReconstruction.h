#ifndef REMOTE_SCARFTOMORECONSTRUCTION_H_
#define REMOTE_SCARFTOMORECONSTRUCTION_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace RemoteAlgorithms {
/***
    Algorithm to initiate, query about, or cancel a tomographic
    reconstruction on SCARF at RAL.
    The algorithm can be used to send different commands to the job
    queue, for example: start a reconstruction job, retrieve
    information about a job or to cancel jobs.

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
  /// (Empty) Constructor
  SCARFTomoReconstruction() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~SCARFTomoReconstruction() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SCARFTomoReconstruction"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Perform a tomographic reconstruction action on the SCARF computer "
      "cluster at RAL";
  }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Remote"; }

private:
  void init();
  /// Execution code
  void exec();

  /// methods to process reconstruction job commands
  void doCreate();
  void doStatus();
  void doCancel();

  // Member vars
  std::string m_userName;
  std::string m_password;
  std::string m_operation;
  std::string m_nxTomoPath;
  std::string m_jobID;
  std::string m_parameterPath;
};

} // end namespace RemoteAlgorithms
} // end namespace Mantid
#endif /*REMOTE_SCARFTOMORECONSTRUCTION_H_*/
