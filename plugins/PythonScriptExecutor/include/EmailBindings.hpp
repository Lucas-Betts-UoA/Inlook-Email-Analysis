#pragma once
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include "Email.hpp"
#include "EmailBody.hpp"
#include "AttributeBagValueInterface.hpp"

namespace py = pybind11;

PYBIND11_EMBEDDED_MODULE(email_core, m) {
    py::class_<AttributeBagValueInterface>(m, "AttributeBagValueInterface")
        .def("to_string", &AttributeBagValueInterface::toString)
        .def("serialize", &AttributeBagValueInterface::serializeToString);

    py::class_<EmailBody>(m, "EmailBody")
        .def("get_all_body_data", &EmailBody::getAllBodyData);

    py::class_<StandardEmailBody, EmailBody>(m, "StandardEmailBody")
        .def(py::init<std::string>())
        .def("get_content", &StandardEmailBody::getAllBodyData)
        .def("set_content", &StandardEmailBody::setContent);


    py::class_<MIMEMultipartPart>(m, "MIMEMultipartPart")
        .def(py::init<std::string>())
        .def("get_body", &MIMEMultipartPart::getBody)
        .def("get_headers", [](const MIMEMultipartPart& part) {
            py::dict pyHeaders;
            for (const auto& [key, valVec] : part.getHeader()) {
                py::list pyValVec;
                for (const auto& val : valVec) pyValVec.append(val);
                pyHeaders[key.c_str()] = pyValVec;
            }
            return pyHeaders;
        })
        .def("get_header_keys", &MIMEMultipartPart::getHeaderKeys)
        .def("get_header_values", &MIMEMultipartPart::getHeaderValues);

    py::class_<MIMEMultipartBodies, EmailBody>(m, "MIMEMultipartBodies")
        .def(py::init<>())
        .def("get_parts", &MIMEMultipartBodies::getMultipartParts)

        .def("add_part", [](MIMEMultipartBodies &self,
                                    const std::map<std::string, std::vector<std::string>> &headers,
                                    const std::string &content) {
            std::pmr::map<std::string, std::vector<std::string>> pmrHeaders;
            for (const auto &[k,v] : headers) {
                pmrHeaders[k] = v;
            }
            self.addPart(pmrHeaders, content);
        })
        .def("get_content", &MIMEMultipartBodies::getAllBodyData);

    py::class_<Email>(m, "Email")
        .def(py::init<>())
        .def_property("is_multipart", &Email::getIsMIMEMultipart, &Email::setIsMIMEMultipart)
        .def("get_headers", &Email::getHeader)
        .def("set_header", &Email::setHeader)
        .def("get_header_values", &Email::getHeaderValues)
        .def("get_body", &Email::getBody, py::return_value_policy::reference)
        .def("get_attribute_keys", &Email::getAttributeKeys)
        .def("get_attribute", &Email::getAttributeValue, py::return_value_policy::reference)
        .def_property_readonly("unique_hash", &Email::getUniqueHash);
}