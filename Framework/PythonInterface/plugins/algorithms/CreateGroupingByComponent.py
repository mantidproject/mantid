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
from itertools import chain
from typing import Sequence


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
        self.component_is_target = (
            self._component_is_target_just_include if self.exclude_target == "" else self._component_is_target_include_and_exclude
        )

        # parse the input string for comma separated strings, that will exclude entire branches of the instrument tree
        self.exclude_branches = [branch.strip() for branch in exclude_branch_strings.split(",")] if exclude_branch_strings else []

        for exclusion_term in self.exclude_branches:
            logger.notice(f"Excluding subtree beneath any component containing: '{exclusion_term}'")

        # read the component and detector info
        self.info = ws.componentInfo()
        self.detinfo = ws.detectorInfo()
        self.dets = self.detinfo.detectorIDs()

        # find the component index of the root component
        idx_root = self.info.root()

        # find all the node ids for which a child meets the include/exclude criteria
        sets_of_target_components = self.get_target_component_sets(idx_root)

        # create a string input to get the desired groupings
        group_string = ",".join(
            [self.get_component_group_string(component_set, num_divisions) for component_set in sets_of_target_components]
        )

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

    def get_target_component_sets(self, root_idx: int) -> Sequence[Sequence[int]]:
        """
        find all the sets of components which meet the search criteria and live under the same parent node in the instrument tree
        """
        # first check if the root component should be excluded, to break out of this whole function
        if self._branch_should_be_excluded(root_idx):
            logger.warning(f"Subtree beneath '{self.info.name(root_idx)}' has been excluded")
            return [
                [],
            ]
        # otherwise we start checking child components of parent nodes, starting with the root node
        stack = [root_idx]
        component_sets = []
        while stack != []:
            idx = stack.pop()
            grouping_set = []
            for c in self._get_children(idx):
                # if child component is a branch which should be excluded, we move on
                if self._branch_should_be_excluded(c):
                    logger.notice(f"Subtree beneath '{self.info.name(c)}' has been excluded")
                    continue
                # otherwise we check if this child component is a target
                if self.component_is_target(c):
                    # if it is, it joins the component set under this parent node
                    grouping_set.append(c)
                else:
                    # otherwise we add it to the stack of parent nodes to search
                    stack.append(c)
            # we add this set to the output and check the next parent node
            component_sets.append(grouping_set)
        return component_sets

    def get_component_group_string(self, component_set: Sequence[int], n=1) -> str:
        """
        for a given set of target components, find all the child detectors (excluding monitors) and split them into n groups -
        formatted as a custom detector grouping string for CreateGroupingWorkspace
        """
        set_det_ids = [self.info.detectorsInSubtree(component) for component in component_set]
        child_detector_ids = [str(self.dets[det_ind]) for det_ind in chain(*set_det_ids) if not self.detinfo.isMonitor(int(det_ind))]
        sub_divided_child_detector_ids = [arr for arr in np.array_split(child_detector_ids, n) if len(arr) > 0]
        return ",".join(["+".join(sub_array) for sub_array in sub_divided_child_detector_ids])

    def _component_name_contains(self, idx: int, target: str) -> bool:
        """
        check a given component (by component index) has a name which contains the target string
        """
        return target in self.info.name(int(idx))

    def _component_is_target_just_include(self, idx: int) -> bool:
        """
        check a given component (by component index) meets the search criteria when only ComponentNameIncludes is provided
        """
        return self._component_name_contains(idx, self.include_target)

    def _component_is_target_include_and_exclude(self, idx: int) -> bool:
        """
        check a given component (by component index) meets the search criteria when
        both ComponentNameIncludes and ComponentNameExcludes are provided
        """
        return self._component_is_target_just_include(idx) and not self._component_name_contains(idx, self.exclude_target)

    def _branch_should_be_excluded(self, idx: int) -> bool:
        """
        check a given component (by component index) name contains one of the exclusion terms which should prevent its subtree
        from being explored for targets
        """
        return np.any([self._component_name_contains(idx, branch) for branch in self.exclude_branches])

    def _get_children(self, idx: int) -> Sequence[int]:
        """
        for a given component index get the child component indices as a list of ints
        """
        return [int(c) for c in self.info.children(idx)]


AlgorithmFactory.subscribe(CreateGroupingByComponent)
