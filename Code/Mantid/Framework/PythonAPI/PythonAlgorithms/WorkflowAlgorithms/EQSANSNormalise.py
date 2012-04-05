"""*WIKI* 

Normalise detector counts by accelerator current and beam spectrum.

*WIKI*"""

import os
from MantidFramework import *
from mantidsimple import *

def find_file(filename=None, startswith=None, data_dir=None):
    """
        Returns a list of file paths for the search criteria.
        @param filename: exact name of a file. The first file found will be returned.
        @param startswith: string that files should start with.
        @param data_dir: additional directory to search
    """
    # Files found
    files_found = []
    filename = str(filename).strip()
    
    # List of directory to look into
    # The preferred way to operate is to have a symbolic link in a work directory,
    # so look in the current working directory first
    search_dirs = [os.getcwd()]
    # The second best location would be with the data itself
    if data_dir is not None:
        search_dirs.append(data_dir)
    # The standard place would be the location of the configuration files on the SNS mount
    search_dirs.append("/SNS/EQSANS/shared/instrument_configuration/")
    search_dirs.extend(ConfigService().getDataSearchDirs())
    
    # Look for specific file
    if filename is not None:
        for d in search_dirs:
            if not os.path.isdir(d):
                continue
            file_path = os.path.join(os.path.normcase(d), filename)
            if os.path.isfile(file_path):
                files_found.append(file_path)
                # If we are looking for a specific file, return right after we find the first
                if startswith is None:
                    return files_found

    # Look for files that starts with a specific string
    if startswith is not None:
        for d in search_dirs:
            if not os.path.isdir(d):
                continue
            files = os.listdir(d)
            for file in files:
                if file.startswith(startswith):
                    file_path = os.path.join(os.path.normcase(d), file)
                    files_found.append(file_path)

    return files_found

class EQSANSNormalise(PythonAlgorithm):
    """
        Normalise detector counts by accelerator current and beam spectrum.
    """
    
    def category(self):
        return "Workflow\\SANS;PythonAlgorithms"

    def name(self):
        return "EQSANSNormalise"

    def PyInit(self):
        # Input workspace
        self.declareWorkspaceProperty("InputWorkspace", "", Direction=Direction.InOut, Description="Workspace to be normalised")
        self.declareProperty("NormaliseToBeam", True, Description="If true, the data will also be normalise by the beam profile")
        self.declareProperty("BeamSpectrumFile", "", Direction=Direction.Input)
        self.declareProperty("OutputMessage", "", Direction=Direction.Output)

    def PyExec(self):
        workspace = self.getPropertyValue("InputWorkspace")
        normalize_to_beam = self.getProperty("NormaliseToBeam")
        flux_data_path = None
       
        if normalize_to_beam:
            # If a spectrum file was supplied, check if it's a valid file path
            beam_spectrum_file = self.getProperty("BeamSpectrumFile").strip()
            if len(beam_spectrum_file):
                if os.path.isfile(beam_spectrum_file):
                    flux_data_path = beam_spectrum_file 
                else:
                    mtd.sendLogMessage("EQSANSNormalise: %s is not a file" % beam_spectrum_file)
            else:
                flux_files = find_file(filename="bl6_flux_at_sample", data_dir=None)
                if len(flux_files)>0:
                    flux_data_path = flux_files[0]
                    mantid.sendLogMessage("Using beam flux file: %s" % flux_data_path)
                else:
                    mantid.sendLogMessage("Could not find beam flux file!")
                    
            if flux_data_path is not None:
                beam_flux_ws = "__beam_flux"
                LoadAscii(flux_data_path, beam_flux_ws, Separator="Tab", Unit="Wavelength")
                ConvertToHistogram(beam_flux_ws, beam_flux_ws)
                RebinToWorkspace(beam_flux_ws, workspace, beam_flux_ws)
                NormaliseToUnity(beam_flux_ws, beam_flux_ws)
                Divide(workspace, beam_flux_ws, workspace)
                mtd[workspace].getRun().addProperty_str("beam_flux_ws", beam_flux_ws, True)
            else:
                flux_data_path = "Could not find beam flux file!"
            
        NormaliseByCurrent(workspace, workspace)
        self.setProperty("OutputMessage", "Data [%s] normalized to accelerator current\n  Beam flux file: %s" % (workspace, str(flux_data_path))) 

mtd.registerPyAlgorithm(EQSANSNormalise())
