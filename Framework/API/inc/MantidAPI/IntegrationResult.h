namespace Mantid {
namespace API {

struct MANTID_API_DLL IntegrationResult {
  double result;
  double error;
  size_t intervals;

  int errorCode;
  bool success;
};

} // namespace API
} // namespace Mantid
