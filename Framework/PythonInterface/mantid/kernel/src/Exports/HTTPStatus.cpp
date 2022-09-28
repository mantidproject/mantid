// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/InternetHelper.h"
#include "MantidPythonInterface/core/Policies/VectorToNumpy.h"

#include <boost/python/enum.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Kernel::InternetHelper;
namespace Policies = Mantid::PythonInterface::Policies;
using namespace boost::python;

void export_HTTPStatus() {
  enum_<InternetHelper::HTTPStatus>("HTTPStatus")
      .value("CONTINUE", InternetHelper::HTTPStatus::CONTINUE)
      .value("SWITCHING_PROTOCOLS", InternetHelper::HTTPStatus::SWITCHING_PROTOCOLS)
      .value("OK", InternetHelper::HTTPStatus::OK)
      .value("CREATED", InternetHelper::HTTPStatus::CREATED)
      .value("ACCEPTED", InternetHelper::HTTPStatus::ACCEPTED)
      .value("NONAUTHORITATIVE", InternetHelper::HTTPStatus::NONAUTHORITATIVE)
      .value("NO_CONTENT", InternetHelper::HTTPStatus::NO_CONTENT)
      .value("RESET_CONTENT", InternetHelper::HTTPStatus::RESET_CONTENT)
      .value("PARTIAL_CONTENT", InternetHelper::HTTPStatus::PARTIAL_CONTENT)
      .value("MULTIPLE_CHOICES", InternetHelper::HTTPStatus::MULTIPLE_CHOICES)
      .value("MOVED_PERMANENTLY", InternetHelper::HTTPStatus::MOVED_PERMANENTLY)
      .value("FOUND", InternetHelper::HTTPStatus::FOUND)
      .value("SEE_OTHER", InternetHelper::HTTPStatus::SEE_OTHER)
      .value("NOT_MODIFIED", InternetHelper::HTTPStatus::NOT_MODIFIED)
      .value("USEPROXY", InternetHelper::HTTPStatus::USEPROXY)
      .value("TEMPORARY_REDIRECT", InternetHelper::HTTPStatus::TEMPORARY_REDIRECT)
      .value("BAD_REQUEST", InternetHelper::HTTPStatus::BAD_REQUEST)
      .value("UNAUTHORIZED", InternetHelper::HTTPStatus::UNAUTHORIZED)
      .value("PAYMENT_REQUIRED", InternetHelper::HTTPStatus::PAYMENT_REQUIRED)
      .value("FORBIDDEN", InternetHelper::HTTPStatus::FORBIDDEN)
      .value("NOT_FOUND", InternetHelper::HTTPStatus::NOT_FOUND)
      .value("METHOD_NOT_ALLOWED", InternetHelper::HTTPStatus::METHOD_NOT_ALLOWED)
      .value("NOT_ACCEPTABLE", InternetHelper::HTTPStatus::NOT_ACCEPTABLE)
      .value("PROXY_AUTHENTICATION_REQUIRED", InternetHelper::HTTPStatus::PROXY_AUTHENTICATION_REQUIRED)
      .value("REQUEST_TIMEOUT", InternetHelper::HTTPStatus::REQUEST_TIMEOUT)
      .value("CONFLICT", InternetHelper::HTTPStatus::CONFLICT)
      .value("GONE", InternetHelper::HTTPStatus::GONE)
      .value("LENGTH_REQUIRED", InternetHelper::HTTPStatus::LENGTH_REQUIRED)
      .value("PRECONDITION_FAILED", InternetHelper::HTTPStatus::PRECONDITION_FAILED)
      .value("REQUESTENTITYTOOLARGE", InternetHelper::HTTPStatus::REQUESTENTITYTOOLARGE)
      .value("REQUESTURITOOLONG", InternetHelper::HTTPStatus::REQUESTURITOOLONG)
      .value("UNSUPPORTEDMEDIATYPE", InternetHelper::HTTPStatus::UNSUPPORTEDMEDIATYPE)
      .value("REQUESTED_RANGE_NOT_SATISFIABLE", InternetHelper::HTTPStatus::REQUESTED_RANGE_NOT_SATISFIABLE)
      .value("EXPECTATION_FAILED", InternetHelper::HTTPStatus::EXPECTATION_FAILED)
      .value("I_AM_A_TEAPOT", InternetHelper::HTTPStatus::I_AM_A_TEAPOT)
      .value("INTERNAL_SERVER_ERROR", InternetHelper::HTTPStatus::INTERNAL_SERVER_ERROR)
      .value("NOT_IMPLEMENTED", InternetHelper::HTTPStatus::NOT_IMPLEMENTED)
      .value("BAD_GATEWAY", InternetHelper::HTTPStatus::BAD_GATEWAY)
      .value("SERVICE_UNAVAILABLE", InternetHelper::HTTPStatus::SERVICE_UNAVAILABLE)
      .value("GATEWAY_TIMEOUT", InternetHelper::HTTPStatus::GATEWAY_TIMEOUT)
      .value("VERSION_NOT_SUPPORTED", InternetHelper::HTTPStatus::VERSION_NOT_SUPPORTED)
      .export_values();
}
