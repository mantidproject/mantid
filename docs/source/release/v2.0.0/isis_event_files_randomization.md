(v2.0.0-isis_event_files_randomization)=

# ISIS event files randomization

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

Events at ISIS are recorded with finite resolution which is effectively binning of the data. If the time of an event is set to the centre of the bin it may lead to unwanted artefacts in the processed data.

![Event artefacts1.png](../../images/2_0_release/Event_artefacts1.png)![Event artefacts2.png](../../images/2_0_release/Event_artefacts2.png)

To prevent this instead of putting into the centres

![CentreBin.png](../../images/2_0_release/CentreBin.png)

the events are randomized within their bins

![Randomized.png](../../images/2_0_release/Randomized.png)
