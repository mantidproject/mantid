#include "MantidDataObjects/MDBoxSaveable.h"
#include "MantidDataObjects/MDBox.h"

namespace Mantid {
namespace DataObjects {

MDBoxSaveable::MDBoxSaveable(API::IMDNode *const Host) : m_MDNode(Host) {}

/** flush data out of the file buffer to the HDD */
void MDBoxSaveable::flushData() const {
  m_MDNode->getBoxController()->getFileIO()->flushData();
}

//-----------------------------------------------------------------------------------------------
/** Physically save the box data. Tries to load any previous data from HDD
 *  Private function called from the DiskBuffer.
 */
void MDBoxSaveable::save() const {
  /**Save the box at the disk position defined by this class. The IMDNode has to
   * be file backed for this method to work */
  API::IBoxControllerIO *fileIO = m_MDNode->getBoxController()->getFileIO();
  if (this->wasSaved()) {
    auto loader = const_cast<MDBoxSaveable *>(this);
    loader->load();
  }

  m_MDNode->saveAt(fileIO, this->getFilePosition());
  this->m_wasSaved = true;
}

/** Loads the data from HDD if these data has not been loaded before.
  * private function called from the DiskBuffer
 */
void MDBoxSaveable::load() {
  // Is the data in memory right now (cached copy)?
  if (!m_isLoaded) {
    API::IBoxControllerIO *fileIO = m_MDNode->getBoxController()->getFileIO();
    m_MDNode->loadAndAddFrom(fileIO, this->getFilePosition(),
                             this->getFileSize());
    this->setLoaded(true);
  }
}
}
}