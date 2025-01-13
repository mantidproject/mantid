# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, no-init
import os
from mantid import config
from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty, mtd, FileProperty, FileAction
from mantid.kernel import Direction, logger, IntArrayProperty


def export_masks(ws, fileName="", returnMasksOnly=False):
    """Exports masks applied to Mantid workspace
    (e.g. drawn using the instrument view) and write these masks
    into the old fashioned ASCII .msk file containing masked spectra numbers.

    The file is Libisis/Mantid old ISIS format compatible and can be read by Libisis
    or Mantid LoadMasks algorithm

    If optional parameter fileName is present, the masks are saved
    in the file with this name
    Otherwise, the file with the name equal to the workspace
    name and the extension .msk is used.

    If returnMasks is set to True, the function does not write to file but returns
    list of masks instead.
    """
    # get pointer to the workspace
    if isinstance(ws, str):
        pws = mtd[ws]
    else:
        pws = ws

    ws_name = pws.name()
    nhist = pws.getNumberHistograms()

    no_detectors = 0
    masks = []
    specInfo = pws.spectrumInfo()
    for i in range(nhist):
        # set provisional spectra ID
        ms = i + 1
        try:
            sp = pws.getSpectrum(i)
            # got real spectra ID, which would correspond real spectra num to spectra ID map
            ms = sp.getSpectrumNo()
        # pylint: disable=W0703
        except Exception:
            logger.notice("Can not retrieve spectra No: " + str(i) + ". Have masked it")
            masks.append(ms)
            continue
        try:
            if specInfo.isMasked(i):
                masks.append(ms)
        # pylint: disable=W0703
        except Exception:
            no_detectors = no_detectors + 1
            masks.append(ms)
            continue

    filename = ""
    if len(fileName) == 0:
        filename = os.path.join(config.getString("defaultsave.directory"), ws_name + ".msk")
    else:
        filename = fileName

    nMasks = len(masks)
    if nMasks == 0:
        if returnMasksOnly:
            logger.warning("Workspace {0} have no masked spectra. File {1} have not been created".format(ws_name, filename))
        else:
            logger.notice("Workspace " + ws_name + " have no masked spectra")
        return masks

    logger.notice("Workspace {0} has {1} masked spectra, including {2} spectra without detectors".format(ws_name, nMasks, no_detectors))

    if not returnMasksOnly:
        writeISISmasks(filename, masks, 8)
    return masks


def flushOutString(f, OutString, BlockSize, BlockLimit):
    """Internal function for writeISISmasks procedure,
    which writes down specified number of mask blocks,
    not to exceed the specified number of masks in row.
    """
    BlockSize += 1
    if BlockSize >= BlockLimit:
        if len(OutString) > 0:
            f.write(OutString + "\n")
        OutString = ""
        BlockSize = 0
    return (f, BlockSize, OutString)


def writeISISmasks(filename, masks, nSpectraInRow=8):
    """Function writes input array in the form of ISSI mask file array
       This is the helper function for export_mask procedure,
       which can be used separately

       namely, if one have array 1,2,3,4, 20, 30,31,32
       file will have the following ASCII stings:
       1-4 20 30-32

       nSpectaInRow indicates the number of the separate spectra ID (numbers) which the program
       needs to fit into one row. For the example above the number has to be 5 or more
       to fit all spectra into a single row. Setting it to one will produce 8 rows with single number in each.

    Usage:
    >>writeISISmasks(fileName,masks)
    where:
    fileName  -- the name of the output file
    masks     -- the array with data
    """
    ext = os.path.splitext(filename)[1]
    if len(ext) == 0:
        filename = filename + ".msk"

    OutString = ""
    LastSpectraN = ""
    BlockSize = 0
    iDash = 0
    im1 = masks[0]

    with open(filename, "w") as f:
        # prepare and write mask data in conventional msk format
        # where adjusted spectra are separated by "-" sign
        for i in masks:
            if len(OutString) == 0:
                OutString = str(i)
                (f, BlockSize, OutString) = flushOutString(f, OutString, BlockSize, nSpectraInRow)
                im1 = i
                continue
            # if the current spectra is different from the previous one by 1 only, we may want to skip it
            if im1 + 1 == i:
                LastSpectraN = str(i)
                iDash += 1
            else:  # it is different and should be dealt separately
                if iDash > 0:
                    OutString = OutString + "-" + LastSpectraN
                    iDash = 0
                    LastSpectraN = ""
                    # write the string if it is finished
                    (f, BlockSize, OutString) = flushOutString(f, OutString, BlockSize, nSpectraInRow)

                if len(OutString) == 0:
                    OutString = str(i)
                else:
                    OutString = OutString + " " + str(i)
                # write the string if it is finished
                (f, BlockSize, OutString) = flushOutString(f, OutString, BlockSize, nSpectraInRow)
            # endif

            # current spectra is the previous now
            im1 = i
        # end masks loop
        if iDash > 0:
            OutString = OutString + "-" + LastSpectraN
        (f, OutString, BlockSize) = flushOutString(f, OutString, BlockSize, 0)


class ExportSpectraMask(PythonAlgorithm):
    """Export workspace's mask"""

    def category(self):
        """Return category"""
        return "DataHandling\\Masking"

    def seeAlso(self):
        return ["SaveMask", "ExportSpectraMask"]

    def name(self):
        """Return name"""
        return "ExportSpectraMask"

    def summary(self):
        return "Returns list of spectra numbers which are masked on a workspace and can save these numbers as legacy .msk file."

    def PyInit(self):
        """Declare properties"""
        self.declareProperty(WorkspaceProperty("Workspace", "", Direction.Input), "The workspace to export masks from.")

        self.declareProperty(
            FileProperty(name="Filename", defaultValue="", action=FileAction.OptionalSave, extensions=[".msk"], direction=Direction.Input),
            doc="The name or full path to the file to save mask to."
            " If empty, the name of the input workspace and default save directory are used.",
        )
        self.declareProperty(
            "ExportMaskOnly",
            False,
            "If true, algorithm will not save mask in a fileand only returns the list containing numbers of masked spectra.",
            Direction.Input,
        )
        self.declareProperty(IntArrayProperty(name="SpectraMasks", direction=Direction.Output), doc="List of the masked  spectra numbers.")
        return

    def PyExec(self):
        """Main execution body"""
        # get parameters
        ws = self.getProperty("Workspace").value
        out_file_name = self.getProperty("Filename").value
        export_masks_only = self.getProperty("ExportMaskOnly").value

        masks = export_masks(ws, out_file_name, export_masks_only)
        self.setProperty("SpectraMasks", masks)
        return


AlgorithmFactory.subscribe(ExportSpectraMask)
