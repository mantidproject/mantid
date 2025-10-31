from .llm_service_manager import LLMService


def _isis_default_generate_request(prompt: str, token: str) -> dict[str:str]:
    url = "http://172.16.114.118:8080/v1/chat/completions"
    headers = {"Authorization": f"Bearer {token}", "Content-Type": "application/json"}
    json_data = {
        "model": "qwen2.5-7b-instruct",
        "messages": [{"role": "user", "content": f"{prompt}"}],
        "max_tokens": 1000,
    }
    return {"url": url, "headers": headers, "json": json_data}


def _isis_default_parse_response(response: dict) -> str:
    return response["choices"][0]["message"]["content"]


isis_default = LLMService(name="isis_default", generate_request=_isis_default_generate_request, parse_response=_isis_default_parse_response)
