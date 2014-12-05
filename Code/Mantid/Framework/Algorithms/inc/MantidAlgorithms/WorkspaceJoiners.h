#ifndef MANTID_ALGORITHMS_WORKSPACEJOINERS_H_
#define MANTID_ALGORITHMS_WORKSPACEJOINERS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace Algorithms
{
  /** A base class to hold code common to two algorithms that bolt two workspaces
      together spectra-wise - ConjoinWorkspaces & AppendSpectra.
      The main difference between these two is that the latter has an OutputWorkspace
      property into which the result is stored, whereas the former puts the result
      into a workspace with the same name as the first input workspace.

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
  class DLLExport WorkspaceJoiners : public API::Algorithm
  {
  public:
    WorkspaceJoiners();
    virtual ~WorkspaceJoiners();

    virtual const std::string category() const;
    
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Join two workspaces together by appending their spectra.";}
    

  protected:
    API::MatrixWorkspace_sptr execWS2D(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2);
    API::MatrixWorkspace_sptr execEvent();

    using Mantid::API::Algorithm::validateInputs;
    void validateInputs(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2);
    void getMinMax(API::MatrixWorkspace_const_sptr ws, specid_t& min, specid_t& max);

    /// Abstract method to be implemented in concrete algorithm classes
    virtual void fixSpectrumNumbers(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2, API::MatrixWorkspace_sptr output) = 0;

    API::Progress *m_progress;                         ///< Progress reporting object
    DataObjects::EventWorkspace_const_sptr event_ws1;  ///< First event workspace input.
    DataObjects::EventWorkspace_const_sptr event_ws2;  ///< Second event workspace input.
  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_WORKSPACEJOINERS_H_ */
