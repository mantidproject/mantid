# -----------------------------------------------------------------
# Schemer definitions for SANSState general
# -----------------------------------------------------------------
sans_state_type = "state_type"
sans_state_event_to_histogram_type = "SANSStateEventToHistogram"


# -----------------------------------------------------------------
# Schema definitions for ISIS
# -----------------------------------------------------------------

# Schema for SANSStateEventToHistogram for ISIS
convert_event_to_histogram_schema_isis = {
    sans_state_type: {"type": basestring, "required": True},
    "wavelength_low": {"type": float, "required": True},
    "wavelength_high": {"type": float, "required": True},
    "wavelength_step": {"type": float, "required": True},
    "wavelength_step_type": {"type": basestring, "required": True}
}


# -----------------------------------------------------------------
# Schemer definitions for other facility
# -----------------------------------------------------------------

