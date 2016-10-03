
#include "MantidQtAPI/SelectionNotificationService.h"

using namespace MantidQt::API;

/**
 * Empty private constructor
 */
SelectionNotificationServiceImpl::SelectionNotificationServiceImpl() {}

/**
 * Empty private destructor
 */
SelectionNotificationServiceImpl::~SelectionNotificationServiceImpl() {}

/**
 *  Method to send out the QPointSelection signal to all objects that
 *  have connected a slot to this signal.  A class just needs to call
 *  SelectionNotificationService::Instance().sendQPointSelection() with
 *  the required information.  This method then emits the corresponding
 *  signal.
 *
 *  @param lab_coords    Set true if qx, qy, and qz are in lab
 *                       coordinates, instead of sample coordinates.
 *  @param qx            The x-component of the Mantid Q-vector.
 *  @param qy            The y-component of the Mantid Q-vector.
 *  @param qz            The z-component of the Mantid Q-vector.
 */
void SelectionNotificationServiceImpl::sendQPointSelection(bool lab_coords,
                                                           double qx, double qy,
                                                           double qz) {
  emit QPointSelection_signal(lab_coords, qx, qy, qz);
  //  std::cout << "QPointSelection_signal emitted\n";
}
