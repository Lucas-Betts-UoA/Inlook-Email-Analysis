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

    py::class_<AttributeBagString, AttributeBagValueInterface>(m, "AttributeBagString")
        .def(py::init<std::string>());
    py::class_<AttributeBagBoolean, AttributeBagValueInterface>(m, "AttributeBagBoolean")
        .def(py::init<bool>());
    py::class_<AttributeBagInteger, AttributeBagValueInterface>(m, "AttributeBagInteger")
        .def(py::init<int>());
    py::class_<AttributeBagDouble, AttributeBagValueInterface>(m, "AttributeBagDouble")
        .def(py::init<double>());
    py::class_<AttributeBagBinary, AttributeBagValueInterface>(m, "AttributeBagBinary")
        .def(py::init([](py::bytes b) {
            std::string s = b;
            std::vector<uint8_t> binaryData(s.begin(), s.end());
            return new AttributeBagBinary(binaryData);
        }));


    py::class_<EmailBody>(m, "EmailBody")
        .def("get_all_body_data", &EmailBody::getAllBodyData);

    py::class_<StandardEmailBody, EmailBody>(m, "StandardEmailBody")
        .def(py::init<std::string>())
        .def("get_content", &StandardEmailBody::getAllBodyData);


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
        .def("get_content", &MIMEMultipartBodies::getAllBodyData);

    py::class_<Email>(m, "Email")
        .def(py::init<>())
        .def("is_multipart", &Email::getIsMIMEMultipart)
        .def("get_headers", &Email::getHeader)
        .def("get_header_values", &Email::getHeaderValues)
        .def("get_body", &Email::getBody, py::return_value_policy::reference)
        .def("get_attribute_keys", &Email::getAttributeKeys)
        .def("get_attribute", &Email::getAttributeValue, py::return_value_policy::reference)
        .def("insert_attribute", [](Email &e, const std::string &key, const std::string &val) {
             e.insertAttribute(key, std::make_unique<AttributeBagString>(val));
        })
        .def("insert_attribute", [](Email &e, const std::string &key, bool val) {
             e.insertAttribute(key, std::make_unique<AttributeBagBoolean>(val));
        })
        .def("insert_attribute", [](Email &e, const std::string &key, int val) {
             e.insertAttribute(key, std::make_unique<AttributeBagInteger>(val));
        })
        .def("insert_attribute", [](Email &e, const std::string &key, double val) {
             e.insertAttribute(key, std::make_unique<AttributeBagDouble>(val));
        })
        .def("insert_attribute", [](Email &e, const std::string &key, py::bytes val) {
             std::string s = val;
             std::vector<uint8_t> vec(s.begin(), s.end());
             e.insertAttribute(key, std::make_unique<AttributeBagBinary>(vec));
        })
        .def_property_readonly("unique_hash", &Email::getUniqueHash);
}