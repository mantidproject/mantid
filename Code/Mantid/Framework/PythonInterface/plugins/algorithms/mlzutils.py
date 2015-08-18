import mantid.simpleapi as api


def cleanup(wslist):
    """
    deletes workspaces from list
        @param wslist List of names of workspaces to delete.
    """
    for wsname in wslist:
        if api.AnalysisDataService.doesExist(wsname):
            api.DeleteWorkspace(wsname)
    return


def same_dimensions(wslist):
    """
    Checks whether all given workspaces have
    the same number of dimensions
    and the same number of histograms
    and the same number of bins
        @param wslist List of workspace names
        @returns True if all dimensions are the same or raises exception if not
    """
    ndims = []
    nhists = []
    nbins = []
    for wsname in wslist:
        wks = api.AnalysisDataService.retrieve(wsname)
        ndims.append(wks.getNumDims())
        nhists.append(wks.getNumberHistograms())
        nbins.append(wks.blocksize())

    ndi = ndims[0]
    nhi = nhists[0]
    nbi = nbins[0]
    if ndims.count(ndi) == len(ndims) and nhists.count(nhi) == len(nhists) and nbins.count(nbi) == len(nbins):
        return True
    else:
        raise RuntimeError("Error: all input workspaces must have the same dimensions!.")


def ws_exist(wslist, logger):
    """
    Checks whether all workspaces from the given list exist
        @param wslist List of workspaces
        @param logger Logger self.log()
        @returns True if all workspaces exist or raises exception if not
    """
    for wsname in wslist:
        if not api.AnalysisDataService.doesExist(wsname):
            message = "Workspace " + wsname + " does not exist!"
            logger.error(message)
            raise RuntimeError(message)

    return True


def compare_properties(lhs_run, rhs_run, plist, logger):
    """
    checks whether properties match in the given runs, produces warnings
        @param lhs_run Left-hand-side run
        @param rhs_run Right-hand-side run
        @param plist   List of properties to compare
        @param logger  Logger self.log()
    """
    lhs_title = ""
    rhs_title = ""
    if lhs_run.hasProperty('run_title'):
        lhs_title = lhs_run.getProperty('run_title').value
    if rhs_run.hasProperty('run_title'):
        rhs_title = rhs_run.getProperty('run_title').value

    for property_name in plist:
        if lhs_run.hasProperty(property_name) and rhs_run.hasProperty(property_name):
            lhs_property = lhs_run.getProperty(property_name)
            rhs_property = rhs_run.getProperty(property_name)
            if lhs_property.type == rhs_property.type:
                if lhs_property.type == 'string':
                    if lhs_property.value != rhs_property.value:
                        message = "Property " + property_name + " does not match! " + \
                            lhs_title + ": " + lhs_property.value + ", but " + \
                            rhs_title + ": " + rhs_property.value
                        logger.warning(message)
                if lhs_property.type == 'number':
                    if abs(lhs_property.value - rhs_property.value) > 5e-3:
                        message = "Property " + property_name + " does not match! " + \
                            lhs_title + ": " + str(lhs_property.value) + ", but " + \
                            rhs_title + ": " + str(rhs_property.value)
                        logger.warning(message)
            else:
                message = "Property " + property_name + " does not match! " + \
                    lhs_title + ": " + str(lhs_property.value) + " has type " + \
                    str(lhs_property.type) + ", but " + rhs_title + ": " + \
                    str(rhs_property.value) + " has type " + str(rhs_property.type)
                logger.warning(message)
        else:
            message = "Property " + property_name + " is not present in " +\
                lhs_title + " or " + rhs_title + " - skipping comparison."
            logger.warning(message)
    return
