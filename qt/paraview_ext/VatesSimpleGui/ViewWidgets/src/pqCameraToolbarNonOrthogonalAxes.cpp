/*=========================================================================

   Program: ParaView
   Module:    pqCameraToolbar.cxx

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
#include "MantidVatesSimpleGuiViewWidgets/pqCameraToolbarNonOrthogonalAxes.h"
#include "ui_pqCameraToolbarNonOrthogonalAxes.h"

#include "MantidVatesSimpleGuiViewWidgets/pqCameraReactionNonOrthogonalAxes.h"
#include "pqActiveObjects.h"
#include "pqRenderViewSelectionReaction.h"

//-----------------------------------------------------------------------------
void pqCameraToolbarNonOrthogonalAxes::constructor() {
  Ui::pqCameraToolbarNonOrthogonalAxes ui;
  ui.setupUi(this);
  new pqCameraReactionNonOrthogonalAxes(
      ui.actionResetCamera, pqCameraReactionNonOrthogonalAxes::RESET_CAMERA);
  new pqCameraReactionNonOrthogonalAxes(
      ui.actionZoomToData, pqCameraReactionNonOrthogonalAxes::ZOOM_TO_DATA);
  new pqCameraReactionNonOrthogonalAxes(
      ui.actionPositiveU, pqCameraReactionNonOrthogonalAxes::RESET_POSITIVE_U);
  new pqCameraReactionNonOrthogonalAxes(
      ui.actionNegativeU, pqCameraReactionNonOrthogonalAxes::RESET_NEGATIVE_U);
  new pqCameraReactionNonOrthogonalAxes(
      ui.actionPositiveV, pqCameraReactionNonOrthogonalAxes::RESET_POSITIVE_V);
  new pqCameraReactionNonOrthogonalAxes(
      ui.actionNegativeV, pqCameraReactionNonOrthogonalAxes::RESET_NEGATIVE_V);
  new pqCameraReactionNonOrthogonalAxes(
      ui.actionPositiveW, pqCameraReactionNonOrthogonalAxes::RESET_POSITIVE_W);
  new pqCameraReactionNonOrthogonalAxes(
      ui.actionNegativeW, pqCameraReactionNonOrthogonalAxes::RESET_NEGATIVE_W);
  new pqCameraReactionNonOrthogonalAxes(
      ui.actionRotate90degCW,
      pqCameraReactionNonOrthogonalAxes::ROTATE_CAMERA_CCW);
  new pqCameraReactionNonOrthogonalAxes(
      ui.actionRotate90degCCW,
      pqCameraReactionNonOrthogonalAxes::ROTATE_CAMERA_CW);

  new pqRenderViewSelectionReaction(ui.actionZoomToBox, NULL,
                                    pqRenderViewSelectionReaction::ZOOM_TO_BOX);

  this->ZoomToDataAction = ui.actionZoomToData;
  this->ZoomToDataAction->setEnabled(
      pqActiveObjects::instance().activeSource() != 0);

  QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView *)),
                   this, SLOT(updateEnabledState()));
  QObject::connect(&pqActiveObjects::instance(),
                   SIGNAL(sourceChanged(pqPipelineSource *)), this,
                   SLOT(updateEnabledState()));
}

//-----------------------------------------------------------------------------
void pqCameraToolbarNonOrthogonalAxes::updateEnabledState() {
  pqView *view = pqActiveObjects::instance().activeView();
  pqPipelineSource *source = pqActiveObjects::instance().activeSource();
  this->ZoomToDataAction->setEnabled(source && view);
}
