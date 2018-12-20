# This function is here to "fix" names for TreeViews and ContextMenus. Squish really
# HATES underscores in these things.
def fix_slashes(ws):
    return ws.replace('_', '\\_')

def apply_ptw_settings():
    clickButton(":objectInspector.Apply_QPushButton")

def make_slice(axisScaleName, coordinate):
    axisScale = waitForObject(":splitter_2.%s_Mantid::Vates::SimpleGui::AxisInteractor" % axisScaleName)
    ext = None
    if axisScaleName[0] == "x":
        ext = ""
    if axisScaleName[0] == "y":
        ext = "_2"
    if axisScaleName[0] == "z":
        ext = "_3"

    scaleWidget = waitForObject(":splitter_2_QwtScaleWidget%s" % ext)

    sp = axisScale.scalePosition
    min = axisScale.getMinimum
    max = axisScale.getMaximum
    delta = max - min
    width = -1
    height = -1
    if sp in (0, 1):
        width = scaleWidget.width
        height = axisScale.height
    else:
        width = scaleWidget.height
        height = axisScale.width

    scaleFactor = height / delta

    if sp in (0, 2):
        x = 1
    else:
        x = width - 1

    if sp in (0, 1):
        scaleFactor *= -1.0
        y = scaleFactor * (coordinate - min) + height
        mouseClick(scaleWidget, x, y, 0, Qt.LeftButton)
    else:
        y = scaleFactor * (coordinate - min)
        mouseClick(scaleWidget, y, x, 0, Qt.LeftButton)
# Get the pipeline filter at a specific index position in the pqPipelineBrowserWidget. Do not
# include the server line in the index position.
def get_pipeline_filter_at_position(index):
    pipeline_model = waitForObject(":_pqPipelineModel")
    pipeline = pipeline_model.index(0, 0)
    for i in range(index):
        pipeline = pipeline_model.index(0, 0, pipeline)
        #test.log("Pipeline Filter: %s" % pipeline_model.data(pipeline).toString())
    return pipeline