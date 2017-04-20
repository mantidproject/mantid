/*=========================================================================

   Program: ParaView
   Module:    pqCameraReaction.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "MantidVatesSimpleGuiViewWidgets/pqCameraReactionNonOrthogonalAxes.h"

#include "pqActiveObjects.h"
#include "pqModelTransformSupportBehavior.h"
#include "pqPipelineRepresentation.h"
#include "pqRenderView.h"

#include "vtkCamera.h"
#include "vtkDataObject.h"
#include "vtkMatrix4x4.h"
#include "vtkMatrixToLinearTransform.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSourceProxy.h"

#include <algorithm>
#include <array>

namespace {
static vtkSMSourceProxy *
FindVisibleProducerWithChangeOfBasisMatrix(pqView *view) {
  foreach (pqRepresentation *repr, view->getRepresentations()) {
    pqDataRepresentation *drepr = qobject_cast<pqDataRepresentation *>(repr);
    if (!drepr || !drepr->isVisible()) {
      continue;
    }

    vtkPVDataInformation *info = drepr->getInputDataInformation();
    vtkPVArrayInformation *cobm =
        info->GetArrayInformation("ChangeOfBasisMatrix", vtkDataObject::FIELD);
    vtkPVArrayInformation *bbimc =
        cobm
            ? info->GetArrayInformation("BoundingBoxInModelCoordinates",
                                        vtkDataObject::FIELD)
            : nullptr;
    if (cobm && bbimc) {
      return vtkSMSourceProxy::SafeDownCast(drepr->getInput()->getProxy());
    }
  }
  return nullptr;
}
}

//-----------------------------------------------------------------------------
pqCameraReactionNonOrthogonalAxes::pqCameraReactionNonOrthogonalAxes(
    QAction *parentObject, pqCameraReactionNonOrthogonalAxes::Mode mode)
    : Superclass(parentObject) {
  this->ReactionMode = mode;
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView *)),
                   this, SLOT(updateEnableState()), Qt::QueuedConnection);
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqCameraReactionNonOrthogonalAxes::updateEnableState() {
  pqView *view = pqActiveObjects::instance().activeView();
  pqPipelineSource *source = pqActiveObjects::instance().activeSource();
  pqRenderView *rview = qobject_cast<pqRenderView *>(view);
  if (view && this->ReactionMode == RESET_CAMERA) {
    this->parentAction()->setEnabled(true);
  } else if (rview) {
    if (this->ReactionMode == ZOOM_TO_DATA) {
      this->parentAction()->setEnabled(source != 0);
    } else {
      // Check hints to see if actions should be disabled
      bool cameraResetButtonsEnabled = true;
      vtkPVXMLElement *hints = rview->getHints();
      if (hints) {
        cameraResetButtonsEnabled =
            hints->FindNestedElementByName("DisableCameraToolbarButtons") ==
            nullptr;
      }

      this->parentAction()->setEnabled(cameraResetButtonsEnabled);
    }
  } else {
    this->parentAction()->setEnabled(false);
  }
}

//-----------------------------------------------------------------------------
void pqCameraReactionNonOrthogonalAxes::onTriggered() {
  switch (this->ReactionMode) {
  case RESET_CAMERA:
    this->resetCamera();
    break;

  case RESET_POSITIVE_U:
    this->resetPositiveU();
    break;

  case RESET_POSITIVE_V:
    this->resetPositiveV();
    break;

  case RESET_POSITIVE_W:
    this->resetPositiveW();
    break;

  case RESET_NEGATIVE_U:
    this->resetNegativeU();
    break;

  case RESET_NEGATIVE_V:
    this->resetNegativeV();
    break;

  case RESET_NEGATIVE_W:
    this->resetNegativeW();
    break;

  case ZOOM_TO_DATA:
    this->zoomToData();
    break;

  case ROTATE_CAMERA_CW:
    this->rotateCamera(90.0);
    break;

  case ROTATE_CAMERA_CCW:
    this->rotateCamera(-90.0);
    break;
  }
}

//-----------------------------------------------------------------------------
void pqCameraReactionNonOrthogonalAxes::resetCamera() {
  pqView *view = pqActiveObjects::instance().activeView();
  if (view) {
    view->resetDisplay();
  }
}

//-----------------------------------------------------------------------------
void pqCameraReactionNonOrthogonalAxes::resetDirection(double look_x,
                                                       double look_y,
                                                       double look_z,
                                                       double up_x, double up_y,
                                                       double up_z) {
  pqRenderView *ren =
      qobject_cast<pqRenderView *>(pqActiveObjects::instance().activeView());
  if (ren) {
    double look[3] = {look_x, look_y, look_z};
    vtkSMSourceProxy *nonOrthogonalSource =
        FindVisibleProducerWithChangeOfBasisMatrix(ren);
    if (nonOrthogonalSource) {
      vtkTuple<double, 16> cobm =
          pqModelTransformSupportBehavior::getChangeOfBasisMatrix(
              nonOrthogonalSource);
      vtkNew<vtkMatrix4x4> mat;
      mat->DeepCopy(cobm.GetData());
      vtkNew<vtkMatrixToLinearTransform> transform;
      transform->SetInput(mat.Get());
      transform->TransformPoint(look, look);
      // vtkPerspectiveTransform::SetupCamera will orthogonalize viewUp.
    }
    ren->resetViewDirection(look[0], look[1], look[2], up_x, up_y, up_z);
  }
}

//-----------------------------------------------------------------------------
void pqCameraReactionNonOrthogonalAxes::resetPositiveU() {
  pqCameraReactionNonOrthogonalAxes::resetDirection(1, 0, 0, 0, 0, 1);
}

//-----------------------------------------------------------------------------
void pqCameraReactionNonOrthogonalAxes::resetNegativeU() {
  pqCameraReactionNonOrthogonalAxes::resetDirection(-1, 0, 0, 0, 0, 1);
}

//-----------------------------------------------------------------------------
void pqCameraReactionNonOrthogonalAxes::resetPositiveV() {
  pqCameraReactionNonOrthogonalAxes::resetDirection(0, 1, 0, 0, 0, 1);
}

//-----------------------------------------------------------------------------
void pqCameraReactionNonOrthogonalAxes::resetNegativeV() {
  pqCameraReactionNonOrthogonalAxes::resetDirection(0, -1, 0, 0, 0, 1);
}

//-----------------------------------------------------------------------------
void pqCameraReactionNonOrthogonalAxes::resetPositiveW() {
  pqCameraReactionNonOrthogonalAxes::resetDirection(0, 0, 1, 0, 1, 0);
}

//-----------------------------------------------------------------------------
void pqCameraReactionNonOrthogonalAxes::resetNegativeW() {
  pqCameraReactionNonOrthogonalAxes::resetDirection(0, 0, -1, 0, 1, 0);
}

//-----------------------------------------------------------------------------
void pqCameraReactionNonOrthogonalAxes::zoomToData() {
  pqRenderView *renModule =
      qobject_cast<pqRenderView *>(pqActiveObjects::instance().activeView());
  pqPipelineRepresentation *repr = qobject_cast<pqPipelineRepresentation *>(
      pqActiveObjects::instance().activeRepresentation());
  if (renModule && repr) {
    vtkSMRenderViewProxy *rm = renModule->getRenderViewProxy();
    rm->ZoomTo(repr->getProxy());
    renModule->render();
  }
}

//-----------------------------------------------------------------------------
void pqCameraReactionNonOrthogonalAxes::rotateCamera(double angle) {
  pqRenderView *renModule =
      qobject_cast<pqRenderView *>(pqActiveObjects::instance().activeView());
  if (renModule) {
    renModule->getRenderViewProxy()->GetActiveCamera()->Roll(angle);
    renModule->render();
  }
}
