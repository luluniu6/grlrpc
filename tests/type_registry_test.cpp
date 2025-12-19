// GrlRPC Type Registry Tests
// Tests for: Requirements 5.1, 5.4

#include <iostream>
#include <cassert>
#include "type_registry.h"

// Test classes
class TestClass1 {
public:
    int value;
};

class TestClass2 {
public:
    std::string name;
};

namespace nested {
    class NestedClass {
    public:
        double data;
    };
}

int main() {
    auto& registry = grlrpc::TypeRegistry::Instance();
    
    // Clear any previous registrations
    registry.Clear();
    
    // Test 1: Register type and get name (Requirement 5.1)
    std::cout << "Test 1: Register type and get name..." << std::endl;
    registry.RegisterType<TestClass1>();
    std::string name1 = registry.GetTypeName<TestClass1>();
    std::cout << "  TestClass1 registered as: " << name1 << std::endl;
    assert(!name1.empty());
    assert(name1.find("TestClass1") != std::string::npos);
    std::cout << "  PASSED" << std::endl;
    
    // Test 2: Register type with custom name
    std::cout << "Test 2: Register type with custom name..." << std::endl;
    registry.RegisterType<TestClass2>("CustomName");
    std::string name2 = registry.GetTypeName<TestClass2>();
    std::cout << "  TestClass2 registered as: " << name2 << std::endl;
    assert(name2 == "CustomName");
    std::cout << "  PASSED" << std::endl;
    
    // Test 3: Check if type is registered
    std::cout << "Test 3: Check if type is registered..." << std::endl;
    assert(registry.IsTypeRegistered<TestClass1>());
    assert(registry.IsTypeRegistered<TestClass2>());
    assert(!registry.IsTypeRegistered<nested::NestedClass>());
    std::cout << "  PASSED" << std::endl;
    
    // Test 4: Unregistered type returns empty string (Requirement 5.4)
    std::cout << "Test 4: Unregistered type returns empty string..." << std::endl;
    std::string name3 = registry.GetTypeName<nested::NestedClass>();
    assert(name3.empty());
    std::cout << "  PASSED" << std::endl;
    
    // Test 5: Register nested class
    std::cout << "Test 5: Register nested class..." << std::endl;
    registry.RegisterType<nested::NestedClass>();
    std::string name4 = registry.GetTypeName<nested::NestedClass>();
    std::cout << "  nested::NestedClass registered as: " << name4 << std::endl;
    assert(!name4.empty());
    assert(name4.find("NestedClass") != std::string::npos);
    std::cout << "  PASSED" << std::endl;
    
    // Test 6: Get all type names
    std::cout << "Test 6: Get all type names..." << std::endl;
    auto all_names = registry.GetAllTypeNames();
    std::cout << "  Registered types: " << all_names.size() << std::endl;
    assert(all_names.size() == 3);
    std::cout << "  PASSED" << std::endl;
    
    // Test 7: HasTypeName
    std::cout << "Test 7: HasTypeName..." << std::endl;
    assert(registry.HasTypeName("CustomName"));
    assert(!registry.HasTypeName("NonExistent"));
    std::cout << "  PASSED" << std::endl;
    
    // Test 8: DemangleTypeName utility
    std::cout << "Test 8: DemangleTypeName utility..." << std::endl;
    std::string demangled = grlrpc::GetDemangledTypeName<TestClass1>();
    std::cout << "  Demangled TestClass1: " << demangled << std::endl;
    assert(demangled.find("TestClass1") != std::string::npos);
    std::cout << "  PASSED" << std::endl;
    
    // Test 9: Clear registry
    std::cout << "Test 9: Clear registry..." << std::endl;
    registry.Clear();
    assert(registry.GetRegisteredTypeCount() == 0);
    assert(!registry.IsTypeRegistered<TestClass1>());
    std::cout << "  PASSED" << std::endl;
    
    std::cout << "\nAll tests passed!" << std::endl;
    return 0;
}
