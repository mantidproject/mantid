from collections import namedtuple

ReductionSettingBundle = namedtuple('ReductionSettingBundle', 'state, data_type, reduction_mode, '
                                                              'output_parts, '
                                                              'scatter_workspace, '
                                                              'scatter_monitor_workspace, '
                                                              'transmission_workspace, '
                                                              'direct_workspace')


MergeBundle = namedtuple('MergeBundle', 'merged_workspace, shift, scale')


OutputBundle = namedtuple('OutputBundle', 'state, data_type, reduction_mode, output_workspace')


OutputPartsBundle = namedtuple('OutputPartsBundle', 'state, data_type, reduction_mode, '
                                                    'output_workspace_count, output_workspace_norm')
