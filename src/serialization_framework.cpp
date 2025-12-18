// GrlRPC Serialization Framework Implementation
// Implements: Requirements 3.1, 3.2, 3.3, 3.4, 5.1, 5.2, 5.3

#include "serialization_framework.h"

namespace grlrpc {

// Most of the implementation is in the header file as templates and inline functions.
// This file is kept for any non-template implementations that may be needed.

// Helper function to convert FieldType to string (for debugging/logging)
std::string FieldTypeToString(FieldType type) {
    switch (type) {
        case FieldType::INT32:  return "INT32";
        case FieldType::INT64:  return "INT64";
        case FieldType::UINT32: return "UINT32";
        case FieldType::UINT64: return "UINT64";
        case FieldType::FLOAT:  return "FLOAT";
        case FieldType::DOUBLE: return "DOUBLE";
        case FieldType::STRING: return "STRING";
        case FieldType::BOOL:   return "BOOL";
        case FieldType::BYTES:  return "BYTES";
        case FieldType::MESSAGE: return "MESSAGE";
        default: return "UNKNOWN";
    }
}

// Helper function to convert string to FieldType
FieldType StringToFieldType(const std::string& str) {
    if (str == "INT32")  return FieldType::INT32;
    if (str == "INT64")  return FieldType::INT64;
    if (str == "UINT32") return FieldType::UINT32;
    if (str == "UINT64") return FieldType::UINT64;
    if (str == "FLOAT")  return FieldType::FLOAT;
    if (str == "DOUBLE") return FieldType::DOUBLE;
    if (str == "STRING") return FieldType::STRING;
    if (str == "BOOL")   return FieldType::BOOL;
    if (str == "BYTES")  return FieldType::BYTES;
    if (str == "MESSAGE") return FieldType::MESSAGE;
    return FieldType::STRING; // Default
}

} // namespace grlrpc
