// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
//
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MDBoxBase.h"
#include "MantidDataObjects/MDEvent.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidMDAlgorithms/ConvToMDBase.h"
#include "MantidMDAlgorithms/MDEventWSWrapper.h"
#include "MantidMDAlgorithms/MDTransfFactory.h"

namespace Mantid {
// Forward declarations
namespace API {
class Progress;
}
namespace MDAlgorithms {
/** The class specializes ConvToDataObjectsBase for the case when the conversion
  occurs from Events WS to the MD events WS
  *
  * The detailed description of the algorithm is provided at
  * dev-docs/source/WritingCustomConvertToMDTransformation.rst.
*/

// Class to process event workspace by direct conversion:

class ConvToMDEventsWS : public ConvToMDBase {
public:
  size_t initialize(const MDWSDescription &WSD, std::shared_ptr<MDEventWSWrapper> inWSWrapper,
                    bool ignoreZeros) override;
  void runConversion(API::Progress *pProgress) override;

protected:
  DataObjects::EventWorkspace_const_sptr m_EventWS;

private:
  // function runs the conversion on
  size_t conversionChunk(size_t workspaceIndex) override;
  // the pointer to the source event workspace as event ws does not work through
  // the public Matrix WS interface
  /**function converts particular type of events into MD space and add these
   * events to the workspace itself    */
  template <class T> size_t convertEventList(size_t workspaceIndex);

  virtual void appendEventsFromInputWS(API::Progress *pProgress, const API::BoxController_sptr &bc);
};

} // namespace MDAlgorithms
} // namespace Mantid
