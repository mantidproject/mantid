# pylint: disable=assignment-from-none
import mantid.simpleapi as api
import numpy as np


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


def remove_fit_workspaces(prefix):
    ws1 = prefix + '_Parameters'
    ws2 = prefix + '_NormalisedCovarianceMatrix'
    cleanup([ws1, ws2])


def do_fit_gaussian(workspace, index, logger, cleanup_fit=True):
    """
    Calculates guess values on peak centre, sigma and peak height.
    Uses them as an input to run a fit algorithm
        @ param workspace --- input workspace
        @ param index --- the spectrum with which WorkspaceIndex to fit
        @ param cleanup --- if False, the intermediate workspaces created by Fit algorithm will not be removed
        @ returns peak_centre --- fitted peak centre
        @ returns sigma --- fitted sigma
    """
    nhist = workspace.getNumberHistograms()
    if index > nhist:
        message = "Index " + str(index) + " is out of range for the workspace " + workspace.getName()
        logger.error(message)
        raise RuntimeError(message)

    x_values = np.array(workspace.readX(index))
    y_values = np.array(workspace.readY(index))

    # get peak centre position
    imax = np.argmax(y_values)
    height = y_values[imax]

    # check for zero or negative signal
    if height <= 0:
        logger.warning("Workspace %s, detector %d has maximum <= 0" % (workspace.getName(), index))
        return [0, 0]

    try_centre = x_values[imax]

    # guess sigma
    indices = np.argwhere(y_values > 0.5*height)
    nentries = len(indices)
    if nentries < 3:
        message = "Spectrum " + str(index) + " in workspace " + workspace.getName() +\
            " has too narrow peak. Cannot guess sigma. Check your data."
        logger.error(message)
        raise RuntimeError(message)
    # fwhm = sigma * (2.*np.sqrt(2.*np.log(2.)))
    fwhm = np.fabs(x_values[indices[nentries - 1, 0]] - x_values[indices[0, 0]])
    sigma = fwhm/(2.*np.sqrt(2.*np.log(2.)))

    # execute Fit algorithm
    myfunc = 'name=Gaussian, Height='+str(height)+', PeakCentre='+str(try_centre)+', Sigma='+str(sigma)
    startX = try_centre - 3.0*fwhm
    endX = try_centre + 3.0*fwhm
    prefix = "Fit" + workspace.getName() + str(index)
    retvals = api.Fit(InputWorkspace=workspace.getName(), WorkspaceIndex=index, StartX=startX, EndX=endX,
                      Function=myfunc, Output=prefix, OutputParametersOnly=True)
    if not retvals or len(retvals) < 4:
        message = "For detector " + str(index) + " in workspace " + workspace.getName() +\
            " failed to retrieve fit results. Input guess parameters are " + str(myfunc)
        logger.error(message)
        if cleanup_fit:
            remove_fit_workspaces(prefix)
        raise RuntimeError(message)

    fitStatus = retvals[0]
    paramTable = retvals[3]
    if fitStatus != 'success':
        message = "For detector " + str(index) + " in workspace " + workspace.getName() +\
            "fit has been terminated with status " + fitStatus + ". Input guess parameters are " + str(myfunc)
        logger.error(message)
        if cleanup_fit:
            remove_fit_workspaces(prefix)
        raise RuntimeError(message)
    result = paramTable.column(1)[1:3]
    if cleanup_fit:
        remove_fit_workspaces(prefix)
    # return list: [peak_centre, sigma]
    return result
