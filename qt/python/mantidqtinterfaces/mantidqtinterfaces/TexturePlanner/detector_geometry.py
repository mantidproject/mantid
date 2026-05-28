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
from Engineering.texture.texture_helper import define_gauge_volume, get_scattering_centre


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
        if wsm.gauge_volume_str:
            define_gauge_volume(wsm.ws, wsm.gauge_volume_str)

        tmp_grp = LoadDetectorsGroupingFile(InputFile=grouping_path, OutputWorkspace="tmp_grp", StoreInADS=False)
        ydat = tmp_grp.extractY()
        self.starting_ind = int(ydat.min() == 0)
        starting_ind = self.starting_ind

        spec_info = group_ws.spectrumInfo()
        comp_info = group_ws.componentInfo()

        # if the sample is partially illuminated the scattering vectors should be taken from the centre of mass of the
        # illuminated region
        scattering_centre = get_scattering_centre(wsm.ws)

        self.det_k = np.asarray(
            [
                (spec_info.position(i) - scattering_centre) / np.linalg.norm(spec_info.position(i) - scattering_centre)
                for i in range(starting_ind, group_ws.getNumberHistograms())
            ]
        )
        ki = scattering_centre - np.array(comp_info.sourcePosition())
        ki_norm = ki / np.linalg.norm(ki)
        detQs_lab = self.det_k - ki_norm
        self.detQs_lab = detQs_lab / np.linalg.norm(detQs_lab, axis=1)[:, None]
