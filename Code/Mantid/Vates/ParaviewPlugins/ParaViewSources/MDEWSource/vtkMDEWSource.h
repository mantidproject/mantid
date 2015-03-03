#ifndef _vtkMDEWSource_h 
#define _vtkMDEWSource_h

#include "vtkUnstructuredGridAlgorithm.h"
#include <string>

namespace Mantid
{
  namespace VATES
  {
    class MDLoadingPresenter;
  }
}

/*  Source for fetching Multidimensional Workspace out of the Mantid Analysis Data Service
    and converting them into vtkDataSets as part of the pipeline source.

    @author Owen Arnold @ Tessella
    @date 08/09/2011

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>

*/

// cppcheck-suppress class_X_Y
class VTK_EXPORT vtkMDEWSource : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkMDEWSource *New();
  vtkTypeMacro(vtkMDEWSource, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void SetWsName(std::string wsName);
  void SetDepth(int depth);

  //------- MDLoadingView methods ----------------
  virtual double getTime() const;
  virtual size_t getRecursionDepth() const;
  virtual bool getLoadInMemory() const;
  //----------------------------------------------

  /// Update the algorithm progress.
  void updateAlgorithmProgress(double, const std::string& message);
  /// Getter for the input geometry xml
  const char* GetInputGeometryXML();
  /// Getter for the special coodinate value
  int GetSpecialCoordinates();
  /// Getter for the workspace name
  const char* GetWorkspaceName();
  /// Getter for the workspace type
  char* GetWorkspaceTypeName();
  /// Getter for the minimum value of the workspace data.
  double GetMinValue();
  /// Getter for the maximum value of the workspace data.
  double GetMaxValue();
  /// Getter for the instrument associated with the workspace data.
  const char* GetInstrument();

protected:
  vtkMDEWSource();
  ~vtkMDEWSource();
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  
  /// Name of the workspace.
  std::string m_wsName;

  /// Recursion depth.
  size_t m_depth;

  /// Time.
  double m_time;

  /// MVP presenter.
  Mantid::VATES::MDLoadingPresenter* m_presenter;

  /// Cached typename.
  std::string typeName;


  // This is part of a workaround for a ParaView providing not the start time of 
  // of current data set. 
  ///Startup flag
  bool m_isStartup;

  // This is part of a workaround for a ParaView providing not the start time of 
  // of current data set. 
  /// Startup time value
  double m_startupTimeValue;

  vtkMDEWSource(const vtkMDEWSource&);
  void operator = (const vtkMDEWSource&);
  void setTimeRange(vtkInformationVector* outputVector);
};
#endif
