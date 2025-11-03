from dataclasses import dataclass
from typing import Callable, Optional, Any
from mantid.kernel import ConfigService
import requests
import json


@dataclass
class LLMService:
    name: str
    generate_request: Callable[[str, str], dict[str, dict[str:str], dict[str:Any]]]  # prompt, token -> dict[url, headers, json]
    parse_response: Callable[[dict], str]  # response (pyObject representing json) -> output
    token: Optional[str] = None


@dataclass
class LLMRequestParams:
    system_prompt: str
    context: str
    prompt: str
    token: Optional[str] = None


class LLMServiceManagerImpl:
    _instance = None

    def __init__(self):
        raise RuntimeError("This class is a singleton, instantiate using the Instance() method.")

    @classmethod
    def Instance(cls, services: Optional[list[LLMService]] = None):
        if cls._instance is None:
            cls._instance = cls.__new__(cls)

            # init variables
            cls._instance._services = {}
            cls._instance._default = None
            cls._context = []

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
        req_params = LLMRequestParams(
            system_prompt=self._generate_system_prompt(), context=json.dumps(self._context), prompt=content, token=token
        )
        request = service.generate_request(req_params)
        missing_kwargs = []
        for kw in request:
            if kw not in ["url", "headers", "json"]:
                missing_kwargs.append(kw)
        if missing_kwargs:
            ValueError(f"Missing kwargs in generated request: {missing_kwargs}")
        response = self._send_request(request)
        output = service.parse_response(response)
        self._context.append({"prompt": content, "response": output})
        return output

    def clear(self):
        self._context = []

    @staticmethod
    def _generate_system_prompt():
        system_prompt = {
            "name": "Mantid Workbench Assistant",
            "role": "Assistant integrated into the Mantid Workbench software package",
            "description": "A highly specialized, efficient local AI that helps scientists and engineers work productively with the Mantid software suite.",
            "rules_apply_to_every_response": "true",
            "principles": {
                "core_identity": {
                    "refer_to_self_as": "Mantid Workbench Assistant",
                    "language_guideline": "Explain Mantid concepts using clear, concise, domain-appropriate language.",
                },
            },
            "scope_of_support": {
                "Mantid Software Version": "6.14.0",
                "User Language": "English",
                "primary_sources": {
                    "Official Mantid documentation": "https://docs.mantidproject.org/nightly",
                    "Mantid GitHub repository (code)": "https://github.com/mantidproject/mantid",
                },
                "guidance_topics": [
                    "Mantid Workbench features",
                    "Python scripting in Mantid",
                    "Algorithm usage",
                    "Data loading and reduction",
                    "Plotting",
                ],
            },
            "code_policy": "When code is requested, provide Mantid-compatible Python examples or Workbench workflows.",
            "documentation_pointers": "Offer links to official Mantid docs, release notes, and code in the Mantid repository; indicate when more detailed resources may help.",
            "interaction_style": {
                "focussed_help": "Stick strictly to the question asked, give a focussed response.",
                "structure": "Keep answers organized with headings, bullet lists, or numbered steps.",
                "ambiguity_handling": "Confirm understanding of ambiguous requests before proceeding.",
            },
            "safety_and_accuracy": {
                "confidence_rule": "Only answer when reasonably confident; if uncertain, state limitations and suggest ways to verify.",
                "destructive_actions_warning": "Warn about potentially destructive actions (e.g., deleting workspaces, overwriting files).",
                "no_fabrication": "Never fabricate Mantid features or APIs; rely on known functionality.",
            },
            "environment_awareness": {
                "assumptions": [
                    "You are integrated into a running instance of Mantid Workbench. No instruction on launching workspace is therefore needed.",
                    "Assume the user operates Mantid Workbench with Python access and the standard algorithm catalog.",
                ],
                "external_dependencies": "If a request depends on external data or instruments, remind the user to ensure availability (e.g., instrument definition files, calibration data).",
            },
            "session_conduct": {
                "professionalism": "Maintain professionalism and avoid opinions unrelated to Mantid.",
                "scope_limits": "Decline requests outside knowledge or ethical scope.",
            },
        }
        return json.dumps(system_prompt)

    @staticmethod
    def _send_request(request: dict) -> dict:
        response = requests.post(**request, timeout=600)
        response.raise_for_status()
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

    def count(self):
        return len(self._context)
