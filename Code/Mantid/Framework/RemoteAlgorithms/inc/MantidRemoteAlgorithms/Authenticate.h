#ifndef AUTHENTICATE_H_
#define AUTHENTICATE_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace RemoteAlgorithms {
/*** Authenticate to the remote compute resource.

    Input Properties:
    <UL>
    <LI> ComputeResource  - The name of the compute resource that will execute
   the job </LI>
    <LI> UserName         - User name on the compute resource </LI>
    <LI> Password         - Password for the compute resource </LI>
    </UL>

    Output Properties: None.
    If the authentication is successfull, a cookie is received that is stored
   internally and
    re-used for all subsequent interactions with the compute resource.


    @author Ross Miller, ORNL
    @date 04/30/2013

    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */

class Authenticate : public Mantid::API::Algorithm {
public:
  /// (Empty) Constructor
  Authenticate() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~Authenticate() {}
  /// Algorithm's name
  virtual const std::string name() const { return "Authenticate"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Authenticate to the remote compute resource.";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Remote"; }

private:
  void init();
  /// Execution code
  void exec();
};

} // end namespace RemoteAlgorithms
} // end namespace Mantid
#endif /*AUTHENTICATE_H_*/
