# API Reference

The Citrus Engine API documentation is generated from Doxygen comments in the source code.

## Documentation Coverage

API documentation is being added to the codebase. As classes and functions are documented with Doxygen comments, they will appear here automatically.

## Modules

The engine is organized into the following C++20 modules:

- **engine.platform** - Platform abstraction (windowing, input, system functions)
- **engine.rendering** - Graphics rendering system (OpenGL-based)
- **engine.scene** - Scene management and entity organization
- **engine.components** - Standard ECS components
- **engine.assets** - Asset loading and management
- **engine.ui** - User interface system (ImGui integration)

## Adding Documentation

To document a class or function, add Doxygen comments to the source code:

```cpp
/**
 * @brief Brief description of the class.
 * 
 * Detailed description explaining what the class does and how to use it.
 */
export class MyClass {
public:
    /**
     * @brief Brief description of the method.
     * @param param1 Description of first parameter
     * @return Description of return value
     */
    int MyMethod(int param1);
};
```

The documentation will be automatically generated and published when changes are pushed to the repository.

## Doxygen Output

For detailed API documentation including all classes, namespaces, and files, build Doxygen HTML locally with `doxygen Doxyfile`. The output will be in `docs/_doxygen/html/index.html`.
