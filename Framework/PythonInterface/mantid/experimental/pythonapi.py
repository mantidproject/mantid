import mantid


def _pythonapi_create_algorithm_function_post_process_lhs_info(lhs):
    modified_names = [ 'algorithm-internal-temporary_{}'.format(id(i)) for i in lhs[1] ]
    return (lhs[0], modified_names)

def _pythonapi_create_algorithm_function_get_initial_workspaces(args, kwargs):
    initial_workspaces = {}
    for i in list(args) + kwargs.values():
        if isinstance(i, _api.Workspace):
            name = 'algorithm-internal-temporary_{}'.format(id(i))
            initial_workspaces[name] = i
            _api.AnalysisDataService[name] = i
    return initial_workspaces

def _pythonapi_gather_returns_internal_get_workspace_from_ADS(value_str, output_workspaces):
    ws = mantid.experimental.convert_weak_ptr_to_shared_ptr(_api.AnalysisDataService[value_str])
    if value_str in output_workspaces:
        mantid.experimental.swap_shared_ptrs(output_workspaces[value_str], ws)
        retval = output_workspaces[value_str]
    else:
        retval = ws
    del _api.AnalysisDataService[value_str]
    return retval

def _pythonapi_gather_returns_cleanup_workspaces(workspaces):
    # Remove remaining temporary workspaces. Note that some may already have been deleted by _gather_returns, but ADS does not mind double deletes.
    for i in workspaces.keys():
        del _api.AnalysisDataService[i]


mantid.simpleapi._create_algorithm_function_post_process_lhs_info = _pythonapi_create_algorithm_function_post_process_lhs_info
mantid.simpleapi._create_algorithm_function_get_initial_workspaces = _pythonapi_create_algorithm_function_get_initial_workspaces
mantid.simpleapi._gather_returns_internal_get_workspace_from_ADS = _pythonapi_gather_returns_internal_get_workspace_from_ADS
mantid.simpleapi._gather_returns_cleanup_workspaces = _pythonapi_gather_returns_cleanup_workspaces


globals().update(vars(mantid.simpleapi))
