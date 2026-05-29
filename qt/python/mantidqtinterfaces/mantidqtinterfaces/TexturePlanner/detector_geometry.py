# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import os
import numpy as np

from mantid.simpleapi import CopySample, GroupDetectors, LoadDetectorsGroupingFile
from Engineering.EnggUtils import CALIB_DIR
from Engineering.texture.texture_helper import define_gauge_volume


class DetectorGeometry:
    """Computes and caches the per-detector-group lab-frame scattering
    directions (det_k) and reduced Q vectors (detQs_lab) for the currently
    selected detector grouping, and applies the grouping to the data workspace.
    """

    def __init__(self, model):
        self._model = model
        self.det_k = None
        self.detQs_lab = None
        # if the group_ws contains group label 0, this is the null group; we skip it
        # when iterating over spectra by starting at starting_ind
        self.starting_ind = 1
        # Detector group and source positions captured at the last regroup, used to
        # rebuild det_k / detQs_lab when only the scattering centre changes.
        self._det_positions = None
        self._source_position = None

    def recompute(self):
        m = self._model
        wsm = m.workspaces
        grouping_path = os.path.join(CALIB_DIR, m.calib_info.get_group_file())
        group_ws = GroupDetectors(
            InputWorkspace=wsm.instr_ws,
            MapFile=grouping_path,
            OutputWorkspace="group_ws",
            StoreInADS=False,
        )
        # Always regroup from the pristine ungrouped workspace; grouping the previously grouped
        # ws by a new MapFile is very slow because each detector ID has to be located inside
        # the already-merged spectra. Sync current sample (shape + material) onto the
        # ungrouped baseline first so the regrouped ws inherits the user's latest sample state.
        if wsm.ws is not None:
            CopySample(
                InputWorkspace=wsm.ws,
                OutputWorkspace=wsm.ungrouped_ws,
                CopyName=False,
                CopyEnvironment=False,
                CopyLattice=False,
            )
        wsm.ws = GroupDetectors(
            InputWorkspace=wsm.ungrouped_ws,
            MapFile=grouping_path,
            OutputWorkspace=wsm.wsname,
        )
        # GroupDetectors rebuilds wsm.ws from ungrouped_ws, which never carries the GaugeVolume
        # log, so re-apply it from the source-of-truth python state.
        if wsm.gauge_volume_str:
            define_gauge_volume(wsm.ws, wsm.gauge_volume_str)

        tmp_grp = LoadDetectorsGroupingFile(InputFile=grouping_path, OutputWorkspace="tmp_grp", StoreInADS=False)
        ydat = tmp_grp.extractY()
        self.starting_ind = int(ydat.min() == 0)

        spec_info = group_ws.spectrumInfo()
        comp_info = group_ws.componentInfo()
        self._det_positions = np.asarray([spec_info.position(i) for i in range(self.starting_ind, group_ws.getNumberHistograms())])
        self._source_position = np.asarray(comp_info.sourcePosition())

        self.recompute_scattering_geometry()

    def recompute_scattering_geometry(self):
        """Recompute det_k / detQs_lab for the current goniometer orientation.

        Reads the scattering centre lazily (it depends on the current goniometer R that the
        caller is expected to have set on the workspace) and combines it with the detector
        positions captured at the last regroup.
        """
        if self._det_positions is None:
            return
        scattering_centre = self._model.workspaces.scattering_centre
        det_vecs = self._det_positions - scattering_centre
        self.det_k = det_vecs / np.linalg.norm(det_vecs, axis=1)[:, None]
        ki = scattering_centre - self._source_position
        ki_norm = ki / np.linalg.norm(ki)
        detQs_lab = self.det_k - ki_norm
        self.detQs_lab = detQs_lab / np.linalg.norm(detQs_lab, axis=1)[:, None]
