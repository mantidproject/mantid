""" This module contains bundle definitions for passing reduction settings between functions."""
from __future__ import (absolute_import, division, print_function)
from collections import namedtuple

# The ReductionSettingBundle contains the information and data for starting a SANSReductionCore reduction.
# This is:
# 1. The state object for this particular reduction.
# 2. The data type, i.e. if can or sample
# 3. The reduction mode, ie HAB, LAB, All or Merged.
# 4. A boolean flag if the output should also provide parts
# 5. A handle to the scatter workspace (sample or can)
# 6. A handle to the scatter monitor workspace (sample or can)
# 7. A handle to the transmission workspace (sample or can)
# 8. A handle to the direct workspace (sample or can)
ReductionSettingBundle = namedtuple('ReductionSettingBundle', 'state, data_type, reduction_mode, '
                                                              'output_parts, '
                                                              'scatter_workspace, '
                                                              'scatter_monitor_workspace, '
                                                              'transmission_workspace, '
                                                              'direct_workspace')

# The MergeBundle contains:
# 1. Handle to a merged workspace
# 2. The shift factor which was used for the merge.
# 3. The scale factor which was used for the merge.
MergeBundle = namedtuple('MergeBundle', 'merged_workspace, shift, scale')

# The OutputBundle contains.
# 1. The state object for this particular reduction.
# 2. The data type, i.e. if can or sample
# 3. The reduction mode, ie HAB, LAB, All or Merged.
# 3. Handle to the output workspace of the reduction.
OutputBundle = namedtuple('OutputBundle', 'state, data_type, reduction_mode, output_workspace')

# The OutputPartsBundle tuple contains information for partial output workspaces from a SANSReductionCore operation.
# 1. The state object for this particular reduction.
# 2. The data type, i.e. if can or sample
# 3. The reduction mode, ie HAB, LAB, All or Merged.
# 4. Handle to the partial output workspace which contains the counts.
# 5. Handle to the partial output workspace which contains the normalization.
OutputPartsBundle = namedtuple('OutputPartsBundle', 'state, data_type, reduction_mode, '
                                                    'output_workspace_count, output_workspace_norm')

OutputTransmissionBundle = namedtuple('OutputTransmissionBundle', 'state, data_type, calculated_transmission_workspace,'
                                                                  ' unfitted_transmission_workspace')
