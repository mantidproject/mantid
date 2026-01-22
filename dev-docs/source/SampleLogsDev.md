# Sample Logs

```{contents}
:local:
```

The following information will be useful to you if you want to write an
[Algorithm](algorithm) that manipulates sample log data.

## Sample logs

When you load a nexus file using Mantid, a set of sample logs relevant
to the experimental run are loaded into the workspace's run object,
which can be accessed through:

``` cpp
#include "MantidAPI/Run.h"

// Declare property with default settings
// IndexType::WorkspaceIndex is default
MatrixWorkspace_sptr workspace;
....
Run &run = workspace->mutableRun()
// get all log data
auto logData = run.getLogData()
// access individual log
auto protonCharge = run.getLogData("proton_charge")
```

Log data can be added and removed from the run object using the
<span class="title-ref">addLogData</span> and
<span class="title-ref">removeLogData</span> methods.

## Multiperiod workspace sample logs

When you load multiperiod data using [Algm Load Isisnexus V2](algm-LoadISISNexus-v2) or
[Algm Load Event Nexus](algm-LoadEventNexus) a set of periods logs, describing the period
information for each workspace are created using the
<span class="title-ref">ISISRunLogs</span> class. This class adds three
logs, which are summarised as:

### period n

This log contains times and boolean flags describing when a period
started, but not necessarily when data collection for that period
occurred. For example, a typical sample log entry for this log would
read:

<table style="width:92%;">
<colgroup>
<col style="width: 31%" />
<col style="width: 30%" />
<col style="width: 29%" />
</colgroup>
<thead>
<tr class="header">
<th>Time</th>
<th>Boolean flag value</th>
<th>Description</th>
</tr>
</thead>
<tbody>
<tr class="odd">
<td>2020-Oct-20 12:19:40</td>
<td>0</td>
<td><ul>
<li></li>
</ul></td>
</tr>
<tr class="even">
<td>2020-Oct-20 12:19:41</td>
<td>1</td>
<td>nth period started</td>
</tr>
<tr class="odd">
<td>2020-Oct-20 12:19:48</td>
<td>0</td>
<td>nth period ended</td>
</tr>
</tbody>
</table>

### running

This log contains a series of times and boolean describing when a data
collection was occurring, for example:

<table style="width:99%;">
<colgroup>
<col style="width: 31%" />
<col style="width: 30%" />
<col style="width: 36%" />
</colgroup>
<thead>
<tr class="header">
<th>Time</th>
<th>Boolean flag value</th>
<th>Description</th>
</tr>
</thead>
<tbody>
<tr class="odd">
<td>2020-Oct-20 12:19:40</td>
<td>0</td>
<td><ul>
<li></li>
</ul></td>
</tr>
<tr class="even">
<td>2020-Oct-20 12:19:41</td>
<td>1</td>
<td>Data collection started</td>
</tr>
<tr class="odd">
<td>2020-Oct-20 12:19:46</td>
<td>0</td>
<td>Data collection ended</td>
</tr>
</tbody>
</table>

### periods

This log contains times and period numbers describing when each period
started, e.g.

| Time                 | Period number | Description      |
|----------------------|---------------|------------------|
| 2020-Oct-20 12:19:40 | 1             | Period 1 started |
| 2020-Oct-20 12:19:49 | 2             | Period 2 started |
| 2020-Oct-20 12:19:56 | 3             | Period 3 started |

These three logs are constructed from the ICP_EVENT which tells you when
various “events” of significance to the software happen, such as
beginning or end of a run or a change of period number.

### Filtering

For multiperiod workspaces, time-series data, such as Theta will be
filtered by combining the <span class="title-ref">period n</span> and
<span class="title-ref">running</span> logs, to create a filter
describing when data collection for each period occurred. For example, a
filter for the nth period would be described as follows, where we note
that each period is described entirely be two boolean values, the first
describing when the data collection started for that period and the
second stating when data collection ended.

<table style="width:99%;">
<colgroup>
<col style="width: 26%" />
<col style="width: 25%" />
<col style="width: 47%" />
</colgroup>
<thead>
<tr class="header">
<th>Time</th>
<th>Boolean flag</th>
<th>Description</th>
</tr>
</thead>
<tbody>
<tr class="odd">
<td>2020-Oct-20 12:19:40</td>
<td>0</td>
<td><ul>
<li></li>
</ul></td>
</tr>
<tr class="even">
<td>2020-Oct-20 12:19:41</td>
<td>1</td>
<td>Data collection for period n started</td>
</tr>
<tr class="odd">
<td>2020-Oct-20 12:19:46</td>
<td>0</td>
<td>Data collection for period n finished</td>
</tr>
</tbody>
</table>

This filtering is performed by the static method
<span class="title-ref">ISISRunLogs::applyLogFiltering</span>.

### Event data

For event data, an additional <span class="title-ref">period_log</span>
is present in the workspace, which is created from the
<span class="title-ref">framelog/period_log</span> entry. This entry
contains the value of various items on a frame by frame (pulse by pulse)
basis. So it is showing you what the neutron acquisition electronics
believe the period number to be at the point that bit of neutron data
was recorded.

The <span class="title-ref">framelog/period_log</span> entry will
therefore contain a list of <span class="title-ref">m</span> times and
the corresponding period number which was active during that frame:

| Time                 | Period number | Description   |
|----------------------|---------------|---------------|
| 2020-Oct-20 12:19:40 | 1             | Period 1 data |
| 2020-Oct-20 12:19:41 | 1             | Period 1 data |
| 2020-Oct-20 12:19:44 | 1             | Period 1 data |
| 2020-Oct-20 12:19:50 | 2             | Period 2 data |
| 2020-Oct-20 12:19:51 | 2             | Period 2 data |
| 2020-Oct-20 12:19:52 | 3             | Period 3 data |
| 2020-Oct-20 12:20:01 | 3             | Period 3 data |
| 2020-Oct-20 12:20:02 | 3             | Period 3 data |

This log is therefore a combination of the
<span class="title-ref">period n</span> and
<span class="title-ref">running logs</span> defined above, whereby it
records the times and periods during data collection. However, rather
than recording a single value describing when the data collection for
that period started, the <span class="title-ref">framelog</span> data
contains a discrete number of time recordings corresponding to the
period that each "bit" of neutron data was collected in.
