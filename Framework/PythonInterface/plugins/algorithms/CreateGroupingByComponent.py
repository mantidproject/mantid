# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
from mantid.kernel import logger
from mantid.api import AlgorithmFactory, WorkspaceProperty, PythonAlgorithm
from mantid.kernel import (
    Direction,
    IntBoundedValidator,
)


class CreateGroupingByComponent(PythonAlgorithm):
    def category(self):
        return "Diffraction\\Engineering"

    def name(self):
        return "CreateGroupingByComponent"

    def summary(self):
        return (
            "Creates a GroupingWorkspace where all components matching the search criteria, "
            "under a given parent component, are grouped together. "
            "These groups are then subdivided by GroupSubdivision"
        )

    def seeAlso(self):
        return ["CreateGroupingWorkspace"]

    def version(self):
        return 1

    def PyInit(self):
        self.declareProperty(
            "InstrumentName",
            "",
            direction=Direction.Input,
            doc="Name of the instrument to base the GroupingWorkspace on",
        )
        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
            doc="The detector grouping workspace.",
        )
        self.declareProperty(
            "ComponentNameIncludes",
            "",
            direction=Direction.Input,
            doc="Search String for determining which components should be grouped, such components MUST contain this string",
        )
        self.declareProperty(
            "ComponentNameExcludes",
            "",
            direction=Direction.Input,
            doc="Search String for determining which components should be grouped, such components MUST NOT contain this string",
        )
        self.declareProperty(
            "ExcludeBranches",
            "",
            direction=Direction.Input,
            doc="Search String for flagging any components which should not be searched through when looking for the target strings",
        )
        positive_int_validator = IntBoundedValidator(lower=1)
        self.declareProperty(
            "GroupSubdivision",
            defaultValue=1,
            direction=Direction.Input,
            validator=positive_int_validator,
            doc="Number of subgroups to create for each collection of components. Minimum is 1.",
        )

    def validateInputs(self):
        issues = dict()
        instr = self.getProperty("InstrumentName").value

        try:
            self.exec_child_alg("LoadEmptyInstrument", InstrumentName=instr, OutputWorkspace="__instr_tmp_ws")
        except RuntimeError:
            issues["InstrumentName"] = f"Failed to find a matching instrument to the provided input: '{instr}'"
        return issues

    def PyExec(self):
        # extract inputs
        instr = self.getProperty("InstrumentName").value
        num_divisions = self.getProperty("GroupSubdivision").value
        self.include_target = self.getProperty("ComponentNameIncludes").value
        self.exclude_target = self.getProperty("ComponentNameExcludes").value
        exclude_branch_strings = self.getProperty("ExcludeBranches").value

        # create the empty instrument workspace
        ws = self.exec_child_alg("LoadEmptyInstrument", InstrumentName=instr, OutputWorkspace="__instr_tmp_ws")

        # if an exclude string is provided the is_target evaluation function should be the one which also checks exclusion
        # otherwise this can just check inclusion
        self.is_target = self._is_target_just_include if self.exclude_target == "" else self._is_target_include_and_exclude

        # parse the input string for comma separated strings, that will exclude entire branches of the instrument tree
        self.exclude_branches = [branch.strip() for branch in exclude_branch_strings.split(",")] if exclude_branch_strings else []

        # read the component and detector info
        self.info = ws.componentInfo()
        detinfo = ws.detectorInfo()
        dets = detinfo.detectorIDs()
        instr_dets = self.info.detectorsInSubtree(self.info.root())

        # create a lookup between the detectors on the instrument (by component index), and the detector id strings
        self.det_id_dict = dict(zip(instr_dets, dets))

        # find the component index of the root component
        idx_root = self.info.root()

        # find all the node ids for which a child meets the include/exclude criteria
        direct_parents = self.get_direct_parents_of_targets(idx_root)

        # create a string input to get the desired groupings
        group_string = ",".join([self.get_component_group_string(p, num_divisions) for p in direct_parents])

        # print this string in debug mode
        logger.debug(group_string)

        # use this string as input for CreateGroupingWorkspace
        group_ws, *_ = self.exec_child_alg(
            "CreateGroupingWorkspace",
            InstrumentName=instr,
            ComponentName=self.info.name(self.info.root()),
            CustomGroupingString=group_string,
            OutputWorkspace="__group_ws",
        )

        self.setProperty("OutputWorkspace", group_ws)

    def exec_child_alg(self, alg_name: str, **kwargs):
        alg = self.createChildAlgorithm(alg_name, enableLogging=False)
        alg.setAlwaysStoreInADS(False)
        alg.initialize()
        alg.setProperties(kwargs)
        alg.execute()
        out_props = tuple(alg.getProperty(prop).value for prop in alg.outputProperties())
        return out_props[0] if len(out_props) == 1 else out_props

    def get_direct_parents_of_targets(self, root_idx: int):
        """
        find all the parent nodes in the instrument tree with at least one child node which meets the search criteria
        """
        parents = set()
        stack = [root_idx]
        while stack != []:
            idx = stack.pop()
            if self._branch_should_be_excluded(idx):
                continue
            children = [int(c) for c in self.info.children(idx)]
            # if any child is a target, this node is a direct parent
            if any(self.is_target(c) for c in children):
                parents.add(idx)
            stack.extend(c for c in children if not self.is_target(c))
        return sorted(parents)

    def get_component_group_string(self, parent, n=1):
        """
        for a given parent node, find all the children which are detectors, and split them into n groups - formatted as
        a custom detector grouping string for CreateGroupingWorkspace
        """
        child_detector_ids = [str(self.det_id_dict[det_ind]) for det_ind in self.info.detectorsInSubtree(int(parent))]
        sub_divided_child_detector_ids = [arr for arr in np.array_split(child_detector_ids, n) if len(arr) > 0]
        return ",".join(["+".join(sub_array) for sub_array in sub_divided_child_detector_ids])

    def _component_name_contains(self, idx, target):
        return target in self.info.name(int(idx))

    def _is_target_just_include(self, idx):
        return self._component_name_contains(idx, self.include_target)

    def _is_target_include_and_exclude(self, idx):
        return self._is_target_include(idx) and not self._component_name_contains(idx, self.exclude_target)

    def _branch_should_be_excluded(self, idx):
        return np.any([self._component_name_contains(idx, branch) for branch in self.exclude_branches])


AlgorithmFactory.subscribe(CreateGroupingByComponent)
