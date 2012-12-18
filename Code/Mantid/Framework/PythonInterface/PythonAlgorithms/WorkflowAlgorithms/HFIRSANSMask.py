"""*WIKI* 

Apply mask to SANS detector

*WIKI*"""

import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *
from reduction_workflow.instruments.sans import hfir_instrument

class HFIRSANSMask(PythonAlgorithm):
    """
        Normalise detector counts by the sample thickness
    """
    
    def category(self):
        return "Workflow\\SANS;PythonAlgorithms"

    def name(self):
        return "HFIRSANSMask"

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("Workspace", "", 
                                                     direction=Direction.InOut))

        self.declareProperty(IntArrayProperty("MaskedDetectorList", values=[],
                                              direction=Direction.Input),
                             "List of detector IDs to be masked")
        
        self.declareProperty(IntArrayProperty("MaskedEdges", values=[0,0,0,0],
                                              direction=Direction.Input),
                             "Number of pixels to mask on the edges: X-low, X-high, Y-low, Y-high")
        
        sides = [ "None", "Front", "Back"]
        self.declareProperty("MaskedSide", "None",
                             StringListValidator(sides))
    
        self.declareProperty("OutputMessage", "", 
                             direction=Direction.Output, doc = "Output message")

    def PyExec(self):
        workspace = self.getProperty("Workspace").value

        # Apply saved mask as needed
        self._apply_saved_mask(workspace)

        # Mask a detector side
        self._mask_detector_side(workspace)

        # Mask edges
        edge_str = self.getPropertyValue("MaskedEdges")
        
        edges = self.getProperty("MaskedEdges").value
        if len(edges)==4:
            masked_pixels = hfir_instrument.get_masked_pixels(edges[0], edges[1],
                                                              edges[2], edges[3],
                                                              workspace)
            self._mask_pixels(masked_pixels, workspace)

        # Mask a list of detectors
        masked_dets = self.getProperty("MaskedDetectorList").value
        if len(masked_dets)>0:
            api.MaskDetectors(Workspace=workspace, DetectorList=masked_dets)
        
        self.setProperty("OutputMessage", "Mask applied")

    def _mask_pixels(self, pixel_list, workspace):
        if len(pixel_list)>0:
            # Transform the list of pixels into a list of Mantid detector IDs
            masked_detectors = hfir_instrument.get_detector_from_pixel(pixel_list)
            # Mask the pixels by passing the list of IDs
            api.MaskDetectors(Workspace=workspace, DetectorList = masked_detectors)
                
    def _apply_saved_mask(self, workspace):
        # Check whether the workspace has mask information
        if workspace.getRun().hasProperty("rectangular_masks"):
            mask_str = workspace.getRun().getProperty("rectangular_masks").value
            try:
                rectangular_masks = pickle.loads(mask_str)
            except:
                rectangular_masks = []
                toks = mask_str.split(',')
                for item in toks:
                    if len(item)>0:
                        c = item.strip().split(' ')
                        if len(c)==4:
                            rectangular_masks.append([int(c[0]), int(c[2]), int(c[1]), int(c[3])])
            masked_pixels = []
            for rec in rectangular_masks:
                try:
                    for ix in range(x_min, x_max+1):
                        for iy in range(y_min, y_max+1):
                            masked_pixels.append([ix, iy])
                except:
                    Logger.get("HFIRSANSMask").error("Badly defined mask from configuration file: %s" % str(rec))
            self._mask_pixels(masked_pixels, workspace)
                
    def _mask_detector_side(self, workspace):
        """
            Mask the back side or front side as needed
        """
        side = self.getProperty("MaskedSide").value
        if side == "Front":
            side_to_mask = 0
        elif side == "Back":
            side_to_mask = 1
        else:
            return
        
        nx = int(workspace.getInstrument().getNumberParameter("number-of-x-pixels")[0])
        ny = int(workspace.getInstrument().getNumberParameter("number-of-y-pixels")[0])
        id_side = []
        
        for iy in range(ny):
            for ix in range(side_to_mask, nx+side_to_mask, 2):
                id_side.append([iy,ix])

        self._mask_pixels(id_side, workspace)
       
registerAlgorithm(HFIRSANSMask())
