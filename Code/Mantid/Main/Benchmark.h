#ifndef MANTID_MAIN_BENCHMARK_H_
#define MANTID_MAIN_BENCHMARK_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace1D.h" 
#include "MantidDataObjects/ManagedWorkspace2D.h" 

/** @class Benchmark Benchmark.h Kernel/Benchmark.h

Benchmark is a class that performs several timed operations.

@author Nick Draper, Tessella Support Services plc
@date 16/01/2008

Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratories

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
class Benchmark 
{
public:
  /// No-arg Constructor
  Benchmark() { }

  /// Runs a timed addition of two workspaces
  void RunPlusTest();
  /// Runs a timed addition of two workspaces
  void RunPlusTest(int detectorCount, int timeBinCount);

  static Mantid::DataObjects::Workspace2D_sptr Create2DWorkspace(int xlen, int ylen);

  static Mantid::DataObjects::Workspace2D_sptr Create2DWorkspace154(int xlen, int ylen);
  static Mantid::DataObjects::Workspace2D_sptr Create2DWorkspace123(int xlen, int ylen);
  static Mantid::DataObjects::Workspace2D_sptr Create2DWorkspace154Hist(int xlen, int ylen);
  static Mantid::DataObjects::Workspace2D_sptr Create2DWorkspace123Hist(int xlen, int ylen);
  static Mantid::DataObjects::Workspace1D_sptr Create1DWorkspaceFib(int size);


};

template<typename T>
class FibSeries
{
private:
  T x1;  /// Initial value 1;
  T x2;  /// Initial value 2;

public:

  FibSeries() : x1(1),x2(1) {}
  T operator()() { const T out(x1+x2); x1=x2; x2=out;  return out; }
};

#endif /*MANTID_MAIN_BENCHMARK_H_*/

