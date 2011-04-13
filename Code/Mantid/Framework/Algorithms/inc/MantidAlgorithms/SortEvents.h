#ifndef MANTID_ALGORITHMS_SORTEVENTS_H_
#define MANTID_ALGORITHMS_SORTEVENTS_H_
    
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** Takes an EventWorkspace and sorts by TOF or frame_index.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the workspace to take as input. Must contain event data. </LI>
    <LI> SortByTof - check to sort by Time of Flight; uncheck to sort by frame index.</LI>
    </UL>

    @author Janik Zikovsky, SNS
    @date Friday, August 13, 2010.

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport SortEvents : public API::Algorithm
{
public:
  /// Default constructor
  SortEvents() : API::Algorithm() {};
  /// Destructor
  virtual ~SortEvents() {};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "SortEvents";}
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1;}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "General";}

protected:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Overridden Algorithm methods
  void init();
  virtual void exec();

};

} // namespace Algorithms
} // namespace Mantid


#endif  /* MANTID_ALGORITHMS_SORTEVENTS_H_ */
