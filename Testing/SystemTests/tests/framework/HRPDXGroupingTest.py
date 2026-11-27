# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Check the HRPD-X detector grouping files in instrument/Grouping against the HRPD-X detector layout.

HRPD-X detector IDs have the form X-YY-ZZZ: bank X, module YY, pixel ZZZ.

    Bank  Modules                      Pixels per module
    1     1-8 (ring around the beam)   286: inner ring 1-132, outer ring 133-286
    2     1-7 north, 8-14 south        402
    3     1-5 north, 6-10 south        456
    4     1-5 north, 6-10 south        276

Modules on each side of banks 2-4 are numbered from top to bottom; bank 1 module 1 is at the top, 3 and 7
are on the north and south horizontal axis and 5 is at the bottom.
"""

import os

import systemtesting
from mantid import config
from mantid.simpleapi import LoadDetectorsGroupingFile

PIXELS_PER_MODULE = {1: 286, 2: 402, 3: 456, 4: 276}
ALL_MODULES = {1: range(1, 9), 2: range(1, 15), 3: range(1, 11), 4: range(1, 11)}
# Modules centred on the horizontal scattering plane
EQUATORIAL_MODULES = {1: (3, 7), 2: (4, 11), 3: (3, 8), 4: (3, 8)}
# The three modules either side of the horizontal plane, i.e. excluding the top and bottom modules
CENTRAL_MODULES = {1: (2, 3, 4, 6, 7, 8), 2: (3, 4, 5, 10, 11, 12), 3: (2, 3, 4, 7, 8, 9), 4: (2, 3, 4, 7, 8, 9)}
B1_INNER_PIXELS = range(1, 133)
B1_OUTER_PIXELS = range(133, 287)


def detectors(bank, modules, pixels=None):
    pixels = pixels if pixels is not None else range(1, PIXELS_PER_MODULE[bank] + 1)
    return {bank * 100000 + module * 1000 + pixel for module in modules for pixel in pixels}


def per_bank(modules):
    """One group per bank."""
    return [detectors(bank, modules[bank]) for bank in (1, 2, 3, 4)]


def per_bank_b1_split(modules):
    """One group per bank, with bank 1 split into its inner and outer rings."""
    return [
        detectors(1, modules[1], B1_INNER_PIXELS),
        detectors(1, modules[1], B1_OUTER_PIXELS),
    ] + [detectors(bank, modules[bank]) for bank in (2, 3, 4)]


def per_module_b1_split():
    """One group per module, with each bank 1 module split into its inner and outer rings."""
    groups = []
    for module in ALL_MODULES[1]:
        groups += [detectors(1, [module], B1_INNER_PIXELS), detectors(1, [module], B1_OUTER_PIXELS)]
    for bank in (2, 3, 4):
        groups += [detectors(bank, [module]) for module in ALL_MODULES[bank]]
    return groups


# Expected groups for each HRPDX_Grouping_<n>.xml file, in group ID order starting at 1
EXPECTED_GROUPS = {
    1: per_module_b1_split(),
    2: per_bank(ALL_MODULES),
    3: per_bank_b1_split(ALL_MODULES),
    4: per_bank(EQUATORIAL_MODULES),
    5: per_bank_b1_split(EQUATORIAL_MODULES),
    6: per_bank(CENTRAL_MODULES),
    7: per_bank_b1_split(CENTRAL_MODULES),
}


def groups_in_workspace(grouping_ws):
    """Return {group ID: set of detector IDs} for a GroupingWorkspace, ignoring ungrouped (group 0) detectors."""
    groups = {}
    for index, group in enumerate(grouping_ws.extractY()[:, 0].astype(int)):
        if group != 0:
            groups.setdefault(int(group), set()).update(int(d) for d in grouping_ws.getSpectrum(index).getDetectorIDs())
    return groups


class HRPDXGroupingFilesTest(systemtesting.MantidSystemTest):
    def runTest(self):
        grouping_dir = os.path.join(config["instrumentDefinition.directory"], "Grouping")
        problems = []
        for number, expected_list in EXPECTED_GROUPS.items():
            filename = f"HRPDX_Grouping_{number}.xml"
            grouping_ws = LoadDetectorsGroupingFile(
                InputFile=os.path.join(grouping_dir, filename), OutputWorkspace=f"HRPDX_Grouping_{number}"
            )
            self.assertEqual(grouping_ws.getInstrument().getName(), "HRPD-X", f"{filename}: wrong instrument")

            expected = {group_id: dets for group_id, dets in enumerate(expected_list, start=1)}
            actual = groups_in_workspace(grouping_ws)
            if set(actual) != set(expected):
                problems.append(f"{filename}: group IDs {sorted(actual)}, expected 1-{len(expected)}")
                continue
            for group_id, expected_dets in expected.items():
                missing = sorted(expected_dets - actual[group_id])
                extra = sorted(actual[group_id] - expected_dets)
                if missing or extra:
                    problems.append(
                        f"{filename} group {group_id}: {len(missing)} detectors missing (e.g. {missing[:5]}), "
                        f"{len(extra)} unexpected (e.g. {extra[:5]})"
                    )
        if problems:
            self.fail("HRPD-X grouping files do not match the detector layout:\n" + "\n".join(problems))
