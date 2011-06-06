/*
 * IntegratePeakTimeSlices.h
 *
 *  Created on: May 5, 2011
 *      Author: ruth
 */

#ifndef INTEGRATEPEAKTIMESLICES_H_
#define INTEGRATEPEAKTIMESLICES_H_
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"
namespace Mantid
{
using namespace DataObjects;
using namespace API;
namespace Algorithms
{
/**
 Integrates each time slice using the BivariateNormal formula, adding the results

 @author Ruth Mikkelson, SNS, ORNL
 @date 06/06/2011

 Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport IntegratePeakTimeSlices: public API::Algorithm
{
public:
  /// Default constructor
  IntegratePeakTimeSlices();
  /// Destructor
  virtual ~IntegratePeakTimeSlices();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "IntegratePeakTimeSlices"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Diffraction"; }

private:
  MatrixWorkspace_sptr inputW;  ///< A pointer to the input workspace, the data set
  TableWorkspace_sptr outputW; ///< A pointer to the output workspace

  // Overridden Algorithm methods
  void init();
  void exec();





};

} // namespace Algorithm
} // namespace Mantid

#endif /* INTEGRATEPEAKTIMESLICES_H_ */
