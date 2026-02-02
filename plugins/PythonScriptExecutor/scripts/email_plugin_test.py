import email_core

def process_emails(email_list):
    print(f"Received {len(email_list)} emails from C++")

    for email in email_list:
        file_identifier = email.get_attribute("File identifier")


        print(f"Processing: {file_identifier.to_string()}")

    return