#ifndef LOADPRENEXUSMONITORS_H_
#define LOADPRENEXUSMONITORS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
namespace DataHandling
{
/** @class Mantid::DataHandling::LoadPreNeXusMonitors

    A data loading routine for SNS PreNeXus beam monitor (histogram) files.

    @author Stuart Campbell, SNS ORNL
    @date 20/08/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
*/
class DLLExport LoadPreNeXusMonitors: public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  LoadPreNeXusMonitors() :
    Mantid::API::Algorithm()
  {
  }
  /// Virtual destructor
  virtual ~LoadPreNeXusMonitors()
  {
  }
  /// Algorithm's name
  virtual const std::string name() const
  {
    return "LoadPreNeXusMonitors";
  }
  /// Algorithm's version
  virtual int version() const
  {
    return (1);
  }
  /// Algorithm's category for identification
  virtual const std::string category() const
  {
    return "DataHandling";
  }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  /// Number of monitors
  size_t nMonitors;

  /// Set to true when instrument geometry was loaded.
  bool instrument_loaded_correctly;

  void runLoadInstrument(const std::string& instrument, API::MatrixWorkspace_sptr localWorkspace);

};

} // namespace DataHandling
} // namespace Mantid

#endif /*LOADPRENEXUSMONITORS_H_*/
