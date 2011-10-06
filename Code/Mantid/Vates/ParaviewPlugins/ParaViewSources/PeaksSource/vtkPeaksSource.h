#ifndef _vtkPeaksSource_h
#define _vtkPeaksSource_h

#include "vtkPolyDataAlgorithm.h"
#include <string>

/**
    Source for fetching Peaks Workspace out of the Mantid Analysis Data Service
    and converting them into vtkDataSets as part of the pipeline source.

    @author Michael Reuter, NSSD ORNL
    @date 06/10/2011

    Copyright &copy; 2007-11 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

class VTK_EXPORT vtkPeaksSource : public vtkPolyDataAlgorithm
{
public:
  static vtkPeaksSource *New();
  vtkTypeRevisionMacro(vtkPeaksSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void SetWsName(std::string wsName);
  /// Update the algorithm progress.
  void updateAlgorithmProgress(double);

protected:
  vtkPeaksSource();
  ~vtkPeaksSource();
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  /// Name of the workspace.
  std::string m_wsName;

  vtkPeaksSource(const vtkPeaksSource&);
  void operator = (const vtkPeaksSource&);
};
#endif
