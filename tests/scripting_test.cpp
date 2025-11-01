#include <gtest/gtest.h>

import engine.scripting;

using namespace engine::scripting;

class ScriptingSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

// Test basic initialization
TEST_F(ScriptingSystemTest, can_initialize_lua_backend) {
    ScriptingSystem scripting(ScriptLanguage::Lua);
    EXPECT_EQ(scripting.GetLanguage(), ScriptLanguage::Lua);
}

// Test registering and calling a global function
TEST_F(ScriptingSystemTest, can_register_and_call_global_function) {
    ScriptingSystem scripting(ScriptLanguage::Lua);
    
    // Register a simple add function
    auto add_func = [](int a, int b) -> int {
        return a + b;
    };
    scripting.RegisterGlobalFunction("add", std::function<int(int, int)>(add_func));
    
    // Execute a script that uses the function
    bool result = scripting.ExecuteString(
        "function get_result()\n"
        "  return add(5, 3)\n"
        "end"
    );
    EXPECT_TRUE(result);
    
    // Call the script function
    int value = scripting.CallFunction<int>("get_result");
    EXPECT_EQ(value, 8);
}

// Test registering function with different types
TEST_F(ScriptingSystemTest, can_handle_multiple_types) {
    ScriptingSystem scripting(ScriptLanguage::Lua);
    
    // Register multiply function with doubles
    scripting.RegisterGlobalFunction("multiply", 
        std::function<double(double, double)>([](double a, double b) {
            return a * b;
        })
    );
    
    bool result = scripting.ExecuteString("result = multiply(2.5, 4.0)");
    EXPECT_TRUE(result);
}

// Test string operations
TEST_F(ScriptingSystemTest, can_handle_strings) {
    ScriptingSystem scripting(ScriptLanguage::Lua);
    
    // Register a concat function
    scripting.RegisterGlobalFunction("concat",
        std::function<std::string(std::string, std::string)>(
            [](std::string a, std::string b) {
                return a + b;
            }
        )
    );
    
    bool result = scripting.ExecuteString("result = concat('Hello', 'World')");
    EXPECT_TRUE(result);
}

// Test class-like registration with fluent interface
TEST_F(ScriptingSystemTest, can_register_class_methods) {
    ScriptingSystem scripting(ScriptLanguage::Lua);
    
    // Register a "class" with methods
    scripting.RegisterClass("Math")
        .Method("AddTwo", std::function<int(int, int)>([](int a, int b) {
            return a + b;
        }))
        .Method("Square", std::function<int(int)>([](int x) {
            return x * x;
        }));
    
    // Use the registered functions
    bool result = scripting.ExecuteString(
        "sum = Math_AddTwo(10, 20)\n"
        "squared = Math_Square(5)"
    );
    EXPECT_TRUE(result);
}

// Test executing Lua script strings
TEST_F(ScriptingSystemTest, can_execute_lua_scripts) {
    ScriptingSystem scripting(ScriptLanguage::Lua);
    
    bool result = scripting.ExecuteString(
        "function TestFunc()\n"
        "  return 42\n"
        "end"
    );
    EXPECT_TRUE(result);
    
    // Call the defined function
    int value = scripting.CallFunction<int>("TestFunc");
    EXPECT_EQ(value, 42);
}

// Test function pointer registration
TEST_F(ScriptingSystemTest, can_register_function_pointers) {
    ScriptingSystem scripting(ScriptLanguage::Lua);
    
    // Define a free function
    auto free_func = [](int x, int y) -> int { return x - y; };
    
    // Register using function pointer syntax
    scripting.RegisterGlobalFunction("subtract", std::function<int(int, int)>(free_func));
    
    bool result = scripting.ExecuteString("diff = subtract(10, 3)");
    EXPECT_TRUE(result);
}
