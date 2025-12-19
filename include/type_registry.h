// GrlRPC Type Registry Header
// Implements: Requirements 5.1, 5.4
// Provides type registration and name lookup functionality

#ifndef GRLRPC_TYPE_REGISTRY_H
#define GRLRPC_TYPE_REGISTRY_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <typeinfo>
#include <cxxabi.h>
#include <memory>
#include <cstdlib>

namespace grlrpc {

// ============================================================================
// Type Name Demangling Utility
// ============================================================================

/**
 * @brief Demangle a C++ type name from its mangled form
 * @param mangled_name The mangled type name (from typeid().name())
 * @return The demangled, human-readable type name
 */
inline std::string DemangleTypeName(const char* mangled_name) {
    int status = 0;
    char* demangled = abi::__cxa_demangle(mangled_name, nullptr, nullptr, &status);
    
    if (status == 0 && demangled != nullptr) {
        std::string result(demangled);
        std::free(demangled);
        return result;
    }
    
    // If demangling fails, return the original mangled name
    return std::string(mangled_name);
}

/**
 * @brief Get demangled type name for a type T
 * @tparam T The type to get the name for
 * @return The demangled type name
 */
template<typename T>
std::string GetDemangledTypeName() {
    return DemangleTypeName(typeid(T).name());
}

// ============================================================================
// TypeRegistry Singleton
// Manages type name registration and lookup
// Implements Requirements 5.1, 5.4
// ============================================================================

class TypeRegistry {
public:
    /**
     * @brief Get the singleton instance of TypeRegistry
     * @return Reference to the TypeRegistry singleton
     */
    static TypeRegistry& Instance() {
        static TypeRegistry instance;
        return instance;
    }

    /**
     * @brief Register a type with an optional custom name
     * @tparam T The type to register
     * @param custom_name Optional custom name for the type (uses demangled name if empty)
     * 
     * Requirement 5.1: When registering a type, the system stores the type name
     * and its metadata in the registry
     */
    template<typename T>
    void RegisterType(const std::string& custom_name = "") {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string type_id = typeid(T).name();
        std::string type_name = custom_name.empty() ? DemangleTypeName(type_id.c_str()) : custom_name;
        
        // Store mapping from mangled name to custom/demangled name
        type_names_[type_id] = type_name;
        
        // Store reverse mapping for lookup by name
        name_to_type_id_[type_name] = type_id;
    }

    /**
     * @brief Get the registered name for a type
     * @tparam T The type to look up
     * @return The registered type name, or empty string if not registered
     * 
     * Requirement 5.4: When a type is not registered, the system returns empty value
     */
    template<typename T>
    std::string GetTypeName() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string type_id = typeid(T).name();
        auto it = type_names_.find(type_id);
        
        if (it != type_names_.end()) {
            return it->second;
        }
        
        // Type not registered, return empty string (Requirement 5.4)
        return "";
    }

    /**
     * @brief Check if a type is registered
     * @tparam T The type to check
     * @return true if the type is registered, false otherwise
     */
    template<typename T>
    bool IsTypeRegistered() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string type_id = typeid(T).name();
        return type_names_.find(type_id) != type_names_.end();
    }

    /**
     * @brief Check if a type name is registered
     * @param type_name The type name to check
     * @return true if the type name is registered, false otherwise
     */
    bool HasTypeName(const std::string& type_name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return name_to_type_id_.find(type_name) != name_to_type_id_.end();
    }

    /**
     * @brief Get all registered type names
     * @return Vector of all registered type names
     */
    std::vector<std::string> GetAllTypeNames() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<std::string> names;
        names.reserve(type_names_.size());
        
        for (const auto& pair : type_names_) {
            names.push_back(pair.second);
        }
        
        return names;
    }

    /**
     * @brief Get the number of registered types
     * @return The count of registered types
     */
    size_t GetRegisteredTypeCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return type_names_.size();
    }

    /**
     * @brief Clear all type registrations (mainly for testing)
     */
    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        type_names_.clear();
        name_to_type_id_.clear();
    }

private:
    TypeRegistry() = default;
    TypeRegistry(const TypeRegistry&) = delete;
    TypeRegistry& operator=(const TypeRegistry&) = delete;

    mutable std::mutex mutex_;
    
    // Map from mangled type name (typeid().name()) to registered name
    std::unordered_map<std::string, std::string> type_names_;
    
    // Reverse map from registered name to mangled type name
    std::unordered_map<std::string, std::string> name_to_type_id_;
};

// ============================================================================
// REGISTER_TYPE Macro
// Provides automatic type registration at static initialization time
// ============================================================================

/**
 * @brief Macro to register a type with the TypeRegistry
 * @param type_class The class/type to register
 * 
 * Usage:
 *   REGISTER_TYPE(MyClass);
 *   
 * This creates a static registrar that registers the type during
 * static initialization.
 */
#define REGISTER_TYPE(type_class) \
    namespace { \
        struct type_class##_TypeRegistrar { \
            type_class##_TypeRegistrar() { \
                grlrpc::TypeRegistry::Instance().RegisterType<type_class>(); \
            } \
        }; \
        static type_class##_TypeRegistrar type_class##_type_registrar_instance; \
    }

/**
 * @brief Macro to register a type with a custom name
 * @param type_class The class/type to register
 * @param custom_name The custom name to use for the type
 * 
 * Usage:
 *   REGISTER_TYPE_WITH_NAME(MyClass, "my_custom_name");
 */
#define REGISTER_TYPE_WITH_NAME(type_class, custom_name) \
    namespace { \
        struct type_class##_TypeRegistrar { \
            type_class##_TypeRegistrar() { \
                grlrpc::TypeRegistry::Instance().RegisterType<type_class>(custom_name); \
            } \
        }; \
        static type_class##_TypeRegistrar type_class##_type_registrar_instance; \
    }

} // namespace grlrpc

#endif // GRLRPC_TYPE_REGISTRY_H
