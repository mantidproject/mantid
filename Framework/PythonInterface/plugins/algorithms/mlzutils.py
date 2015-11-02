import mantid.simpleapi as api
from mantid.api import AlgorithmManager
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


def do_fit_gaussian(workspace, index, logger):
    """
    Calculates guess values on peak centre, sigma and peak height.
    Uses them as an input to run a fit algorithm
        @ param workspace --- input workspace
        @ param index --- the spectrum with which WorkspaceIndex to fit
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

    # create and execute Fit algorithm
    myfunc = 'name=Gaussian, Height='+str(height)+', PeakCentre='+str(try_centre)+', Sigma='+str(sigma)
    startX = try_centre - 3.0*fwhm
    endX = try_centre + 3.0*fwhm
    prefix = "Fit" + workspace.getName() + str(index)
    fit_alg = AlgorithmManager.createUnmanaged('Fit')
    fit_alg.initialize()
    fit_alg.setChild(True)
    api.set_properties(fit_alg, Function=myfunc, InputWorkspace=workspace, CreateOutput=True, Output=prefix)
    fit_alg.setProperty('StartX', startX)
    fit_alg.setProperty('EndX', endX)
    fit_alg.setProperty('WorkspaceIndex', index)
    fit_successful = fit_alg.execute()
    param_table = fit_alg.getProperty('OutputParameters').value

    if not fit_successful:
        message = "For detector " + str(index) + " in workspace " + workspace.getName() +\
            "fit was not successful. Input guess parameters are " + str(myfunc)
        logger.error(message)
        raise RuntimeError(message)

    # return list: [peak_centre, sigma]
    return param_table.column(1)[1:3]
