# import mantid algorithms, numpy and matplotlib
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
            "Creates a Grouping Workspace where all components matching the search criteria, "
            "under a given parent component, are grouped together. "
            "These groups are then subdivided by N"
        )

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
            "IncludeComponents",
            "",
            direction=Direction.Input,
            doc="Search String for determining which components should be grouped, such components MUST contain this string",
        )
        self.declareProperty(
            "ExcludeComponents",
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
            self.exec_child_alg("LoadEmptyInstrument", InstrumentName=instr, OutputWorkspace="__instr")
        except RuntimeError:
            issues["InstrumentName"] = f"Failed to find a matching instrument to the provided input: '{instr}'"
        return issues

    def PyExec(self):
        # get inputs
        instr = self.getProperty("InstrumentName").value
        ws = self.exec_child_alg("LoadEmptyInstrument", InstrumentName=instr, OutputWorkspace="__instr")

        self.target = self.getProperty("IncludeComponents").value
        self.null = self.getProperty("ExcludeComponents").value

        self.is_target = self.is_target_include if self.null == "" else self.is_target_exclude

        num_divisions = self.getProperty("GroupSubdivision").value

        exclude_branch_strings = self.getProperty("ExcludeBranches").value
        self.exclude_branches = [branch.strip() for branch in exclude_branch_strings.split(",")] if exclude_branch_strings else None

        self.info = ws.componentInfo()
        self.detinfo = ws.detectorInfo()
        self.dets = self.detinfo.detectorIDs()
        self.instr_dets = self.info.detectorsInSubtree(self.info.root())

        idx_root = self.info.root()

        direct_parents = self.direct_parents_of_targets(idx_root)

        group_string = ",".join([self.get_component_group_string(p, num_divisions) for p in direct_parents])

        logger.debug(group_string)

        group_ws, *_ = self.exec_child_alg(
            "CreateGroupingWorkspace",
            InstrumentName=instr,
            ComponentName=self.info.name(self.info.root()),
            CustomGroupingString=group_string,
            OutputWorkspace="_group_ws",
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

    def component_name_contains(self, idx, target):
        return target in self.info.name(int(idx))

    def is_target_include(self, idx):
        return self.component_name_contains(idx, self.target)

    def is_target_exclude(self, idx):
        return self.is_target_include(idx) and not self.component_name_contains(idx, self.null)

    def branch_should_be_excluded(self, idx):
        if self.exclude_branches:
            return np.any([self.component_name_contains(idx, branch) for branch in self.exclude_branches])
        return False

    def direct_parents_of_targets(self, root_idx: int):
        parents = set()
        stack = [int(root_idx)]
        while stack:
            idx = stack.pop()
            if self.branch_should_be_excluded(idx):
                continue
            children = [int(c) for c in self.info.children(idx)]
            # if any child is a target, this node is a direct parent
            if any(self.is_target(c) for c in children):
                parents.add(idx)
            stack.extend(c for c in children if not self.is_target(c))
        return sorted(parents)

    def get_det_id(self, comp_ind):
        return str(self.dets[np.where(self.instr_dets == comp_ind)][0])

    def get_component_group_string(self, parent, n=1):
        all_sub_arrays = np.array_split([self.get_det_id(det) for det in self.info.detectorsInSubtree(int(parent))], n)
        return ",".join(["+".join(sub_array) for sub_array in all_sub_arrays])


AlgorithmFactory.subscribe(CreateGroupingByComponent)
