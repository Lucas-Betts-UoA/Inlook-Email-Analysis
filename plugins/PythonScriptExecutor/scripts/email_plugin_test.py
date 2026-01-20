import json

def process_emails(email_json_str):
    emails = json.loads(email_json_str)

    for email in emails:
        headers = email.setdefault("header", {})
        headers["X-Processed-By"] = "PythonScriptExecutor"
        print("Hello world")

    return json.dumps(emails)