#ifndef MANTID_MDALGORITHMS_CONVERTTOMDHELPER2_H_
#define MANTID_MDALGORITHMS_CONVERTTOMDHELPER2_H_

#include "MantidKernel/System.h"
#include "MantidMDAlgorithms/ConvertToMDParent.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
namespace MDAlgorithms
{

  /** ConvertToMDHelper : Algorithm to calculate limits for ConvertToMD
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport ConvertToMDHelper2  : public ConvertToMDParent
  {
  public:
    ConvertToMDHelper2();
    virtual ~ConvertToMDHelper2();
    
    virtual const std::string name() const;
    virtual int version() const{return 2;}

  protected: // for testing 
    void buildMinMaxWorkspaceWithMinInstrument(Mantid::API::MatrixWorkspace_const_sptr &InWS2D, const std::vector<std::string> &oterDimNames);
  private:
    virtual void initDocs();
    void exec();
    void init();
    /// pointer to the input workspace;
    Mantid::DataObjects::Workspace2D_sptr m_MinMaxWS2D;

  };


} // namespace MDAlgorithms
} // namespace Mantid

#endif  /* MANTID_MDALGORITHMS_CONVERTTOMDHELPER_H_ */
