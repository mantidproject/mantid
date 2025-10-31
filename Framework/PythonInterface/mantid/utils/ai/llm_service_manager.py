from dataclasses import dataclass
from typing import Callable, Optional, Any
from mantid.kernel import ConfigService
import requests


@dataclass
class LLMService:
    name: str
    generate_request: Callable[[str, str], dict[str, dict[str:str], dict[str:Any]]]  # prompt, token -> dict[url, headers, json]
    parse_response: Callable[[dict], str]  # response (pyObject representing json) -> output
    token: Optional[str] = None


class LLMServiceManagerImpl:
    _instance = None

    def __init__(self):
        raise RuntimeError("This class is a singleton, instantiate using the instance() method.")

    @classmethod
    def Instance(cls, services: Optional[list[LLMService]] = None):
        if cls._instance is None:
            cls._instance = cls.__new__(cls)

            # init variables
            cls._instance._services = {}
            cls._instance._default = None
            cls._prompt_count = 0  # test manager persistence

            # deal with services arg
            if services:
                cls._instance.register(services.pop(0), default=True)  # Make first service default
            for service in services:
                cls._instance.register(service)
        return cls._instance

    def register(self, service: LLMService, default=False):
        if service.name in self._services:
            raise ValueError(f"A service with name {service.name} already exists in the manager")
        self._services[service.name] = service
        if default:
            self._default = service

    # set the service to be called as default
    def set_default(self, service_name):
        self._default = self._services[service_name]

    # send a request to the LLM
    def prompt(self, content: str, service_name=None):
        service = self._get_specified_service_or_default(service_name)
        token = service.token if service.token else self._get_token_from_config()
        request = service.generate_request(content, token)
        missing_kwargs = []
        for kw in request:
            if kw not in ["url", "headers", "json"]:
                missing_kwargs.append(kw)
        if missing_kwargs:
            ValueError(f"Missing kwargs in generated request: {missing_kwargs}")
        response = self._send_request(request)
        output = service.parse_response(response)
        return output

    def _send_request(self, request: dict) -> dict:
        response = requests.post(**request, timeout=30)
        response.raise_for_status()
        self._prompt_count += 1
        return response.json()

    def _get_specified_service_or_default(self, service_name: str = None):
        if not service_name and not self._default:
            raise ValueError("Please set set as default or specify a service")
        elif service_name and service_name not in self._services:
            raise ValueError(f"Invalid service specified: {service_name}")
        service = self._services[service_name] if service_name else self._default
        return service

    def set_token(self, token: str, service_name: str = None):
        service = self._get_specified_service_or_default(service_name)
        service.token = token

    def _get_token_from_config(self) -> str:
        config = ConfigService.Instance()
        return config["default.aitoken"]

    # test manager persistence
    def count(self):
        return self._prompt_count
