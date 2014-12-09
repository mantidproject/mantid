#ifndef MANTID_CRYSTAL_CLEARUB_H_
#define MANTID_CRYSTAL_CLEARUB_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Crystal
{

  /** ClearUB : Clear the UB matrix from a workspace by removing the oriented lattice.
    
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
  class DLLExport ClearUB  : public API::Algorithm
  {
  public:
    ClearUB();
    virtual ~ClearUB();
    
    virtual const std::string name() const;
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Clears the UB by removing the oriented lattice from the sample.";}

    virtual int version() const;
    virtual const std::string category() const;

  protected:

    bool doExecute(Mantid::API::Workspace * const ws, bool dryRun);

  private:
    bool clearSingleExperimentInfo(Mantid::API::ExperimentInfo * const experimentInfo, const bool dryRun) const;
    void init();
    void exec();


  };


} // namespace Crystal
} // namespace Mantid

#endif  /* MANTID_CRYSTAL_CLEARUB_H_ */
