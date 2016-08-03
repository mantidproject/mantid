from SANS2.Common.SANSFileInformation import (SANSFileInformationFactory)


def get_instrument_from_state_data(data_info):
    data_info.validate()
    sample_scatter_file = data_info.sample_scatter

    file_info_factory = SANSFileInformationFactory()
    file_info = file_info_factory.create_sans_file_information(sample_scatter_file)
    return file_info.get_instrument()