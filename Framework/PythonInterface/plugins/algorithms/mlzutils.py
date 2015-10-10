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


def compare_properties(lhs_run, rhs_run, plist, logger, tolerance=5e-3):
    """
    checks whether properties match in the given runs, produces warnings
        @param lhs_run Left-hand-side run
        @param rhs_run Right-hand-side run
        @param plist   List of properties to compare
        @param logger  Logger self.log()
    """
    lhs_title = ""
    rhs_title = ""
    if lhs_run.hasProperty('run_title') and rhs_run.hasProperty('run_title'):
        lhs_title = lhs_run.getProperty('run_title').value
        rhs_title = rhs_run.getProperty('run_title').value

    # for TOFTOF run_titles can be identical
    if lhs_title == rhs_title:
        if lhs_run.hasProperty('run_number') and rhs_run.hasProperty('run_number'):
            lhs_title = str(lhs_run.getProperty('run_number').value)
            rhs_title = str(rhs_run.getProperty('run_number').value)

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
                elif lhs_property.type == 'number':
                    if abs(lhs_property.value - rhs_property.value) > tolerance:
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


def compare_mandatory(wslist, plist, logger, tolerance=0.01):
    """
    Compares properties which are required to be the same.
    Produces error message and throws exception if difference is observed
    or if one of the sample logs is not found.
    Important: exits after the first difference is observed. No further check is performed.
        @param wslist  List of workspaces
        @param plist   List of properties to compare
        @param logger  Logger self.log()
        @param tolerance  Tolerance for comparison of the double values.
    """
    # retrieve the workspaces, form dictionary {wsname: run}
    runs = {}
    for wsname in wslist:
        wks = api.AnalysisDataService.retrieve(wsname)
        runs[wsname] = wks.getRun()

    for prop in plist:
        properties = []
        for wsname in wslist:
            run = runs[wsname]
            if not run.hasProperty(prop):
                message = "Workspace " + wsname + " does not have sample log " + prop
                logger.error(message)
                raise RuntimeError(message)

            curprop = run.getProperty(prop)
            if curprop.type == 'string':
                properties.append(curprop.value)
            elif curprop.type == 'number':
                properties.append(int(curprop.value/tolerance))
            else:
                message = "Unknown type " + str(curprop.type) + " for the sample log " +\
                    prop + " in the workspace " + wsname
                logger.error(message)
                raise RuntimeError(message)
        # this should never happen, but lets check
        nprop = len(properties)
        if nprop != len(wslist):
            message = "Error. Number of properties " + str(nprop) + " for property " + prop +\
                " is not equal to number of workspaces " + str(len(wslist))
            logger.error(message)
            raise RuntimeError(message)
        pvalue = properties[0]
        if properties.count(pvalue) != nprop:
            message = "Sample log " + prop + " is not identical in the given list of workspaces. \n" +\
                "Workspaces: " + ", ".join(wslist) + "\n Values: " + str(properties)
            logger.error(message)
            raise RuntimeError(message)
