import email_core

def process_emails(email_list):
    print(f"Received {len(email_list)} emails from C++")

    for email in email_list:
        file_identifier = email.get_attribute("File identifier")

        print(f"Processing: {file_identifier.to_string()}")
        email.insert_attribute("AttributeBagString python test", "Hello world")
        email.insert_attribute("AttributeBagInteger python test", 10542)
        email.insert_attribute("AttributeBagBoolean python test", True)
        email.insert_attribute("AttributeBagDouble python test", 0.98)
        email.insert_attribute("AttributeBagBinary python test", b'\xDE\xAD')

        string_test = email.get_attribute("AttributeBagString python test")

        print(type(string_test))

        print(f"AttributeBagString test: {string_test.to_string()}")

        body = email.get_body()

        if isinstance(body, email_core.StandardEmailBody):
            print(f"Standard Body: {body.get_content()}")

        elif isinstance(body, email_core.MIMEMultipartBodies):
            print("Multipart Email detected:")

            for part in body.get_parts():
                part_headers = part.get_headers()
                content_type = part_headers.get("Content-Type", ["unknown"])[0]

                print(f" - Part Type: {content_type}")
                print(f" - Content Snippet: {part.get_body()[:50]}...")

    return