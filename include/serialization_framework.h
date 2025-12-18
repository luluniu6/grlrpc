// GrlRPC Serialization Framework Header
// Implements: Requirements 3.1, 3.2, 3.3, 3.4, 5.1, 5.2, 5.3

#ifndef GRLRPC_SERIALIZATION_FRAMEWORK_H
#define GRLRPC_SERIALIZATION_FRAMEWORK_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <any>
#include <unordered_map>
#include <mutex>
#include <typeinfo>
#include <cxxabi.h>

namespace grlrpc {

// ============================================================================
// Field Type Enumeration
// ============================================================================

enum class FieldType {
    INT32,
    INT64,
    UINT32,
    UINT64,
    FLOAT,
    DOUBLE,
    STRING,
    BOOL,
    BYTES,
    MESSAGE
};

// ============================================================================
// Field Descriptor
// ============================================================================

struct FieldDescriptor {
    std::string name;
    FieldType type;
    int field_number;
    std::function<std::any(const void*)> getter;
    std::function<void(void*, const std::any&)> setter;
};

// ============================================================================
// Message Descriptor
// ============================================================================

struct MessageDescriptor {
    std::string message_name;
    std::vector<FieldDescriptor> fields;
    
    void AddField(const FieldDescriptor& field) {
        fields.push_back(field);
    }
    
    const FieldDescriptor* GetField(const std::string& name) const {
        for (const auto& field : fields) {
            if (field.name == name) {
                return &field;
            }
        }
        return nullptr;
    }
    
    const FieldDescriptor* GetFieldByNumber(int number) const {
        for (const auto& field : fields) {
            if (field.field_number == number) {
                return &field;
            }
        }
        return nullptr;
    }
};


// ============================================================================
// ISerializer Interface (Generic reflection-based serializer)
// ============================================================================

class ISerializer {
public:
    virtual ~ISerializer() = default;
    
    // Serialize object to string using reflection metadata
    virtual bool Serialize(const void* obj, const MessageDescriptor& desc, 
                          std::string& output) = 0;
    
    // Deserialize string to object using reflection metadata
    virtual bool Deserialize(const std::string& input, void* obj, 
                            const MessageDescriptor& desc) = 0;
    
    // Get serializer name (e.g., "json", "binary")
    virtual std::string GetName() const = 0;
};

// ============================================================================
// ITypeSerializer Interface (Type-specific high-performance serializer)
// ============================================================================

template<typename T>
class ITypeSerializer {
public:
    virtual ~ITypeSerializer() = default;
    
    // Serialize typed object to string
    virtual bool Serialize(const T& obj, std::string& output) = 0;
    
    // Deserialize string to typed object
    virtual bool Deserialize(const std::string& input, T& obj) = 0;
    
    // Get serializer name
    virtual std::string GetName() const = 0;
};

// ============================================================================
// Type-erased wrapper for ITypeSerializer
// ============================================================================

class ITypeSerializerBase {
public:
    virtual ~ITypeSerializerBase() = default;
    virtual std::string GetName() const = 0;
};

template<typename T>
class TypeSerializerWrapper : public ITypeSerializerBase {
public:
    explicit TypeSerializerWrapper(std::unique_ptr<ITypeSerializer<T>> serializer)
        : serializer_(std::move(serializer)) {}
    
    std::string GetName() const override {
        return serializer_->GetName();
    }
    
    ITypeSerializer<T>* Get() { return serializer_.get(); }
    
private:
    std::unique_ptr<ITypeSerializer<T>> serializer_;
};


// ============================================================================
// ReflectionRegistry Singleton
// Stores type metadata for reflection-based serialization
// ============================================================================

class ReflectionRegistry {
public:
    static ReflectionRegistry& Instance() {
        static ReflectionRegistry instance;
        return instance;
    }
    
    // Register a message descriptor for a type
    void RegisterType(const std::string& type_name, const MessageDescriptor& descriptor) {
        std::lock_guard<std::mutex> lock(mutex_);
        descriptors_[type_name] = descriptor;
    }
    
    // Get message descriptor for a type
    const MessageDescriptor* GetDescriptor(const std::string& type_name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = descriptors_.find(type_name);
        if (it != descriptors_.end()) {
            return &it->second;
        }
        return nullptr;
    }
    
    // Check if type is registered
    bool HasType(const std::string& type_name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return descriptors_.find(type_name) != descriptors_.end();
    }
    
    // Get all registered type names
    std::vector<std::string> GetRegisteredTypes() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> types;
        for (const auto& pair : descriptors_) {
            types.push_back(pair.first);
        }
        return types;
    }
    
    // Clear all registrations (mainly for testing)
    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        descriptors_.clear();
    }

private:
    ReflectionRegistry() = default;
    ReflectionRegistry(const ReflectionRegistry&) = delete;
    ReflectionRegistry& operator=(const ReflectionRegistry&) = delete;
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, MessageDescriptor> descriptors_;
};


// ============================================================================
// SerializerRegistry Singleton
// Manages both generic and type-specific serializers
// ============================================================================

class SerializerRegistry {
public:
    static SerializerRegistry& Instance() {
        static SerializerRegistry instance;
        return instance;
    }
    
    // Register a generic serializer
    void RegisterSerializer(const std::string& name, std::unique_ptr<ISerializer> serializer) {
        std::lock_guard<std::mutex> lock(mutex_);
        serializers_[name] = std::move(serializer);
    }
    
    // Get a generic serializer by name
    ISerializer* GetSerializer(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = serializers_.find(name);
        if (it != serializers_.end()) {
            return it->second.get();
        }
        return nullptr;
    }
    
    // Register a type-specific serializer
    template<typename T>
    void RegisterTypeSerializer(const std::string& serializer_name, 
                               std::unique_ptr<ITypeSerializer<T>> serializer) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string key = MakeTypeSerializerKey<T>(serializer_name);
        type_serializers_[key] = std::make_unique<TypeSerializerWrapper<T>>(std::move(serializer));
    }
    
    // Get a type-specific serializer
    template<typename T>
    ITypeSerializer<T>* GetTypeSerializer(const std::string& serializer_name) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string key = MakeTypeSerializerKey<T>(serializer_name);
        auto it = type_serializers_.find(key);
        if (it != type_serializers_.end()) {
            auto* wrapper = dynamic_cast<TypeSerializerWrapper<T>*>(it->second.get());
            if (wrapper) {
                return wrapper->Get();
            }
        }
        return nullptr;
    }
    
    // Check if type-specific serializer exists
    template<typename T>
    bool HasTypeSerializer(const std::string& serializer_name) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string key = MakeTypeSerializerKey<T>(serializer_name);
        return type_serializers_.find(key) != type_serializers_.end();
    }
    
    // Clear all registrations (mainly for testing)
    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        serializers_.clear();
        type_serializers_.clear();
    }

private:
    SerializerRegistry() = default;
    SerializerRegistry(const SerializerRegistry&) = delete;
    SerializerRegistry& operator=(const SerializerRegistry&) = delete;
    
    template<typename T>
    std::string MakeTypeSerializerKey(const std::string& serializer_name) {
        return std::string(typeid(T).name()) + ":" + serializer_name;
    }
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::unique_ptr<ISerializer>> serializers_;
    std::unordered_map<std::string, std::unique_ptr<ITypeSerializerBase>> type_serializers_;
};


// ============================================================================
// SerializerFactory
// Factory class for creating and accessing serializers
// ============================================================================

class SerializerFactory {
public:
    // Serialize object using type-specific serializer if available, 
    // otherwise fall back to generic serializer
    template<typename T>
    static bool Serialize(const T& obj, const std::string& serializer_name, 
                         std::string& output) {
        // Try type-specific serializer first (Requirement 3.3)
        auto* type_serializer = SerializerRegistry::Instance().GetTypeSerializer<T>(serializer_name);
        if (type_serializer) {
            return type_serializer->Serialize(obj, output);
        }
        
        // Fall back to generic serializer (Requirement 3.4)
        auto* generic_serializer = SerializerRegistry::Instance().GetSerializer(serializer_name);
        if (generic_serializer) {
            std::string type_name = GetDemangled<T>();
            const auto* descriptor = ReflectionRegistry::Instance().GetDescriptor(type_name);
            if (descriptor) {
                return generic_serializer->Serialize(&obj, *descriptor, output);
            }
        }
        
        return false;
    }
    
    // Deserialize string to object
    template<typename T>
    static bool Deserialize(const std::string& input, T& obj, 
                           const std::string& serializer_name) {
        // Try type-specific serializer first (Requirement 3.3)
        auto* type_serializer = SerializerRegistry::Instance().GetTypeSerializer<T>(serializer_name);
        if (type_serializer) {
            return type_serializer->Deserialize(input, obj);
        }
        
        // Fall back to generic serializer (Requirement 3.4)
        auto* generic_serializer = SerializerRegistry::Instance().GetSerializer(serializer_name);
        if (generic_serializer) {
            std::string type_name = GetDemangled<T>();
            const auto* descriptor = ReflectionRegistry::Instance().GetDescriptor(type_name);
            if (descriptor) {
                return generic_serializer->Deserialize(input, &obj, *descriptor);
            }
        }
        
        return false;
    }
    
    // Get demangled type name
    template<typename T>
    static std::string GetDemangled() {
        int status;
        char* demangled = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status);
        if (status == 0 && demangled) {
            std::string result(demangled);
            free(demangled);
            return result;
        }
        return typeid(T).name();
    }
};


// ============================================================================
// Helper Functions and Macros
// ============================================================================

// Helper function to add a field to a descriptor
template<typename ClassType, typename FieldType>
void AddFieldToDescriptor(MessageDescriptor& desc, 
                         const std::string& name,
                         grlrpc::FieldType type,
                         int field_number,
                         FieldType ClassType::* member_ptr) {
    FieldDescriptor field;
    field.name = name;
    field.type = type;
    field.field_number = field_number;
    
    // Create getter using member pointer
    field.getter = [member_ptr](const void* obj) -> std::any {
        const ClassType* typed_obj = static_cast<const ClassType*>(obj);
        return typed_obj->*member_ptr;
    };
    
    // Create setter using member pointer
    field.setter = [member_ptr](void* obj, const std::any& value) {
        ClassType* typed_obj = static_cast<ClassType*>(obj);
        typed_obj->*member_ptr = std::any_cast<FieldType>(value);
    };
    
    desc.AddField(field);
}

// Macro to simplify field registration
#define GRLRPC_REGISTER_FIELD(desc, class_type, field_name, field_type, field_num) \
    grlrpc::AddFieldToDescriptor<class_type, decltype(class_type::field_name)>( \
        desc, #field_name, field_type, field_num, &class_type::field_name)

// Macro to begin type registration
#define GRLRPC_BEGIN_TYPE_REGISTRATION(class_type) \
    static void RegisterReflection() { \
        grlrpc::MessageDescriptor desc; \
        desc.message_name = #class_type;

// Macro to end type registration
#define GRLRPC_END_TYPE_REGISTRATION(class_type) \
        grlrpc::ReflectionRegistry::Instance().RegisterType(#class_type, desc); \
    }

// Macro for complete type registration with fields
#define GRLRPC_REGISTER_TYPE(class_type, ...) \
    namespace { \
        struct class_type##_Registrar { \
            class_type##_Registrar() { \
                grlrpc::MessageDescriptor desc; \
                desc.message_name = #class_type; \
                __VA_ARGS__ \
                grlrpc::ReflectionRegistry::Instance().RegisterType(#class_type, desc); \
            } \
        }; \
        static class_type##_Registrar class_type##_registrar_instance; \
    }

} // namespace grlrpc

#endif // GRLRPC_SERIALIZATION_FRAMEWORK_H
