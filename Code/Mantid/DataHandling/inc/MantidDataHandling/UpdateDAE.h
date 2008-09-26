#ifndef UPDATEDAE_H_
#define UPDATEDAE_H_

#include "MantidAPI/Algorithm.h"

    /** @class UpdateDAE UpdateDAE.h DataHandling/UpdateDAE.h

    Updates the data periodically in a workspace which have been previously loaded from
    the ISIS DATA acquisition system using LoadDAE algorithm. UpdateDAE must be executed
    asynchronously usunig Algorithm::executeAsync() method.

    Required Properties:
    <UL>
    <LI> Workspace - The name of the workspace to update which was previously loaded with LoadDAE</LI>
    </UL>

    Optional Properties: 
    <UL>
    <LI> update_rate  - The update period in secons.</LI>
    </UL>

    @author Roman Tolchenov, Tessella Support Services plc
    @date 22/09/08

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>. 
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
class UpdateDAE : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  UpdateDAE() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~UpdateDAE() {}
  /// Algorithm's name
  virtual const std::string name() const { return "UpdateDAE"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "DataHandling"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  /// Static reference to the logger class
  static Mantid::Kernel::Logger& g_log;
};

#endif /*UPDATEDAE_H_*/
