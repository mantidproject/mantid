from .llm_service_manager import LLMService, LLMRequestParams
import json


def _isis_default_generate_request(params: LLMRequestParams) -> dict[str:str]:
    # "http://172.16.114.118:8080/v1/chat/completions"
    url = "http://172.16.114.118:8080/prompt"
    headers = {"Authorization": f"Bearer {params.token}", "Content-Type": "application/json"}
    content = {"context": params.context, "user prompt": params.prompt} if params.context else {"user prompt": params.prompt}
    json_data = {
        "model": "qwen2.5-7b-instruct",
        "messages": [{"role": "system", "content": f"{params.system_prompt}"}, {"role": "user", "content": f"{json.dumps(content)}"}],
        "max_tokens": 10000,
    }
    return {"url": url, "headers": headers, "json": json_data}


def _isis_default_parse_response(response: dict) -> str:
    response = response["choices"][0]["message"]["content"]
    return response


isis_default = LLMService(name="isis_default", generate_request=_isis_default_generate_request, parse_response=_isis_default_parse_response)
