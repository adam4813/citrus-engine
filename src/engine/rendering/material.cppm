module;

#include <memory>
#include <unordered_map>
#include <string>

export module engine.rendering:material;

import :types;
import :shader;

export namespace engine::rendering {
    struct MaterialCreateInfo {
        ShaderId shader = INVALID_SHADER;
        std::unordered_map<std::string, int> int_properties;
        std::unordered_map<std::string, float> float_properties;
        std::unordered_map<std::string, Vec2> vec2_properties;
        std::unordered_map<std::string, Vec3> vec3_properties;
        std::unordered_map<std::string, Vec4> vec4_properties;
        std::unordered_map<std::string, TextureId> texture_properties;
    };

    class Material {
    public:
        Material();

        explicit Material(ShaderId shader);

        ~Material();

        // Property setters
        void SetShader(ShaderId shader);

        void SetProperty(const std::string &name, int value);

        void SetProperty(const std::string &name, float value);

        void SetProperty(const std::string &name, const Vec2 &value);

        void SetProperty(const std::string &name, const Vec3 &value);

        void SetProperty(const std::string &name, const Vec4 &value);

        void SetTexture(const std::string &name, TextureId texture);

        // Property getters
        template<typename T>
        T GetProperty(const std::string &name) const;

        TextureId GetTexture(const std::string &name) const;

        ShaderId GetShader() const;

        // Apply material (set all uniforms)
        void Apply(const Shader &shader) const;

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

    class MaterialManager {
    public:
        MaterialManager();

        ~MaterialManager();

        // Material creation
        MaterialId CreateMaterial(const std::string &name, const MaterialCreateInfo &info);

        MaterialId CreateMaterial(const std::string &name, ShaderId shader);

        // Material access
        Material &GetMaterial(MaterialId id);

        const Material &GetMaterial(MaterialId id) const;

        MaterialId FindMaterial(const std::string &name) const;

        // Resource management
        void DestroyMaterial(MaterialId id);

        bool IsValid(MaterialId id) const;

        void Clear();

        // Get default materials
        MaterialId GetDefaultMaterial() const;

        MaterialId GetSpriteMaterial() const;

        MaterialId GetUnlitMaterial() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };
}
