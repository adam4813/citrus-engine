#include "editor_scene.h"

#include "commands/clipboard_commands.h"
#include "commands/entity_commands.h"
#include "editor_utils.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace editor {

EditorScene::EditorScene() = default;

EditorScene::~EditorScene() = default;

void EditorScene::Initialize(engine::Engine& engine) {
	std::cout << "EditorScene: Initializing 2D Scene Editor..." << std::endl;

	// Store engine reference for methods that need it
	engine_ = &engine;

	// Scene system is already initialized by Engine::Initialize()
	// Create a new empty scene for editing
	const auto& scene_manager = engine::scene::GetSceneManager();
	editor_scene_id_ = scene_manager.CreateScene("UntitledScene");
	scene_manager.SetActiveScene(editor_scene_id_);

	state_.current_file_path = "";

	// Create editor camera (not part of the scene, used for viewport navigation)
	// Manually created in Flecs ECS world, so it isn't under the scene root entity.
	// TODO: Ensure this is excluded from scene serialization (Phase 1.5)
	editor_camera_ = engine.ecs.GetWorld().entity("EditorCamera");
	editor_camera_.set<engine::components::Transform>({{0.0f, 0.0f, 5.0f}}); // Position at z=5
	editor_camera_.set<engine::components::Camera>({
			.target = {0.0f, 0.0f, 0.0f},
			.up = {0.0f, 1.0f, 0.0f},
			.fov = 60.0f,
			.aspect_ratio = 16.0f / 9.0f,
			.near_plane = 0.1f,
			.far_plane = 100.0f,
	});
	engine.ecs.SetActiveCamera(editor_camera_);

	// Wire up panel callbacks
	EditorCallbacks callbacks;
	callbacks.on_entity_selected = [this](const engine::ecs::Entity entity) { OnEntitySelected(entity); };
	callbacks.on_entity_deleted = [this](const engine::ecs::Entity entity) { OnEntityDeleted(entity); };
	callbacks.on_scene_modified = [this]() { OnSceneModified(); };
	callbacks.on_show_rename_dialog = [this](const engine::ecs::Entity entity) { OnShowRenameDialog(entity); };
	callbacks.on_add_child_entity = [this](const engine::ecs::Entity parent) { OnAddChildEntity(parent); };
	callbacks.on_add_component = [this](const engine::ecs::Entity entity, const std::string& component_name) {
		OnAddComponent(entity, component_name);
	};
	callbacks.on_asset_selected = [this](const engine::scene::AssetType type, const std::string& name) {
		OnAssetSelected(type, name);
	};
	callbacks.on_asset_deleted = [this](const engine::scene::AssetType type, const std::string& name) {
		OnAssetDeleted(type, name);
	};
	callbacks.on_scene_camera_changed = [this](const engine::ecs::Entity camera) { scene_active_camera_ = camera; };
	callbacks.on_execute_command = [this](std::unique_ptr<ICommand> command) {
		command_history_.Execute(std::move(command));
	};
	callbacks.on_instantiate_prefab = [this](const std::string& prefab_path) {
		auto& scene_manager = engine::scene::GetSceneManager();
		if (auto* scene = scene_manager.TryGetScene(editor_scene_id_)) {
			if (const auto entity = engine::scene::PrefabUtility::InstantiatePrefab(
						prefab_path, scene, engine_->ecs, selected_entity_);
				entity.is_valid()) {
				OnEntitySelected(entity);
				OnSceneModified();
			}
		}
	};
	callbacks.on_copy_entity = [this]() { CopyEntity(); };
	callbacks.on_paste_entity = [this]() { PasteEntity(); };
	callbacks.on_duplicate_entity = [this]() { DuplicateEntity(); };
	callbacks.on_open_tileset = [this](const std::string& path) { tileset_editor_panel_.OpenTileset(path); };
	callbacks.on_open_data_table = [this](const std::string& path) { data_table_editor_panel_.OpenTable(path); };
	callbacks.on_open_file = [this](const std::string& path) {
		code_editor_panel_.OpenFile(path);
		code_editor_panel_.SetVisible(true);
	};

	hierarchy_panel_.SetCallbacks(callbacks);
	hierarchy_panel_.SetWorld(&engine.ecs);
	properties_panel_.SetCallbacks(callbacks);
	asset_browser_panel_.SetCallbacks(callbacks);
	viewport_panel_.SetCallbacks(callbacks);

	// Register example node types for the graph editor
	RegisterExampleGraphNodes();

	// Register shader-specific node types for the shader editor
	RegisterShaderGraphNodes();

	// Register texture-specific node types for the texture editor
	RegisterTextureGraphNodes();

	// Create a demo graph so the panel isn't empty
	CreateExampleGraph();

	std::cout << "EditorScene: Initialized with new scene" << std::endl;
}

void EditorScene::Shutdown(engine::Engine& engine) {
	std::cout << "EditorScene: Shutting down..." << std::endl;

	// Destroy editor camera
	if (editor_camera_.is_valid()) {
		editor_camera_.destruct();
	}

	// Cleanup scene system
	engine::scene::ShutdownSceneSystem();

	std::cout << "EditorScene: Shutdown complete" << std::endl;
}

void EditorScene::Update(engine::Engine& engine, const float delta_time) {
	// Cache delta_time for RenderUI camera controls
	last_delta_time_ = delta_time;

	// Update the active scene if in play mode
	if (state_.is_running) {
		auto& scene_manager = engine::scene::GetSceneManager();
		scene_manager.Update(delta_time);
	}
}

void EditorScene::Render(engine::Engine& engine) const {
	// Render the active scene if in play mode
	if (state_.is_running) {
		auto& scene_manager = engine::scene::GetSceneManager();
		scene_manager.Render();
	}
}

void EditorScene::RenderUI(engine::Engine& engine) {
	const ImGuiID dockspace_id = ImGui::GetID("My Dockspace");
	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	// Handle keyboard shortcuts
	const ImGuiIO& io = ImGui::GetIO();
	if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z) && !io.KeyShift) {
		command_history_.Undo();
	}
	if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) {
		command_history_.Redo();
	}
	// Alternative redo shortcut: Ctrl+Shift+Z
	if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z)) {
		command_history_.Redo();
	}

	// Clipboard shortcuts
	if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C)) {
		CopyEntity();
	}
	if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_X)) {
		CutEntity();
	}
	if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V)) {
		PasteEntity();
	}
	if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D)) {
		DuplicateEntity();
	}

	// Create settings
	if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
		ImGuiID dock_id_left = 0;
		ImGuiID dock_id_main = dockspace_id;
		ImGuiID dock_id_bottom = 0;
		ImGui::DockBuilderSplitNode(dock_id_main, ImGuiDir_Left, 0.20f, &dock_id_left, &dock_id_main);
		ImGui::DockBuilderSplitNode(dock_id_main, ImGuiDir_Down, 0.25f, &dock_id_bottom, &dock_id_main);
		ImGuiID dock_id_left_top = 0;
		ImGuiID dock_id_left_bottom = 0;
		ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Up, 0.50f, &dock_id_left_top, &dock_id_left_bottom);
		ImGui::DockBuilderDockWindow("Hierarchy", dock_id_left_top);
		ImGui::DockBuilderDockWindow("Properties", dock_id_left_bottom);
		ImGui::DockBuilderDockWindow("Viewport", dock_id_main);
		ImGui::DockBuilderDockWindow("Assets", dock_id_bottom);
		ImGui::DockBuilderDockWindow("Graph Editor", dock_id_bottom);
		ImGui::DockBuilderDockWindow("Texture Editor", dock_id_bottom);
		ImGui::DockBuilderDockWindow("Animation Editor", dock_id_bottom);
		ImGui::DockBuilderDockWindow("Tileset Editor", dock_id_bottom);
		ImGui::DockBuilderDockWindow("Code Editor", dock_id_bottom);
		ImGui::DockBuilderDockWindow("Data Table Editor", dock_id_bottom);
		ImGui::DockBuilderFinish(dockspace_id);
	}

	// Submit dockspace
	ImGui::DockSpaceOverViewport(dockspace_id, viewport, ImGuiDockNodeFlags_PassthruCentralNode);

	// Get current scene for asset browser
	auto& scene_manager = engine::scene::GetSceneManager();
	auto* scene = scene_manager.TryGetScene(editor_scene_id_);

	RenderMenuBar();
	hierarchy_panel_.Render(editor_scene_id_, selected_entity_);
	properties_panel_.Render(selected_entity_, engine.ecs, scene, selected_asset_, scene_active_camera_);
	viewport_panel_.Render(engine, scene, state_.is_running, editor_camera_, last_delta_time_, selected_entity_);
	asset_browser_panel_.Render(scene, selected_asset_);
	graph_editor_panel_.Render();
	shader_editor_panel_.Render();
	texture_editor_panel_.Render();
	animation_editor_panel_.Render();
	behavior_tree_editor_panel_.Render();
	tileset_editor_panel_.Render();
	data_table_editor_panel_.Render();
	sound_editor_panel_.Render();
	code_editor_panel_.Render();

	// Handle dialogs
	if (state_.show_new_scene_dialog) {
		ImGui::OpenPopup("New Scene");
		state_.show_new_scene_dialog = false;
	}

	if (ImGui::BeginPopupModal("New Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Create a new scene?");
		ImGui::Text("Any unsaved changes will be lost.");

		if (ImGui::Button("Create", ImVec2(120, 0))) {
			NewScene();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (state_.show_open_dialog) {
		ImGui::OpenPopup("Open Scene");
		state_.show_open_dialog = false;
	}

	if (ImGui::BeginPopupModal("Open Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Enter scene file path:");
		ImGui::InputText("##filepath", file_path_buffer_, sizeof(file_path_buffer_));

		if (ImGui::Button("Open", ImVec2(120, 0))) {
			OpenScene(file_path_buffer_);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (state_.show_save_as_dialog) {
		ImGui::OpenPopup("Save Scene As");
		state_.show_save_as_dialog = false;
	}

	if (ImGui::BeginPopupModal("Save Scene As", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Enter save file path:");
		ImGui::InputText("##savepath", file_path_buffer_, sizeof(file_path_buffer_));

		if (ImGui::Button("Save", ImVec2(120, 0))) {
			SaveSceneAs(file_path_buffer_);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (state_.show_rename_entity_dialog) {
		ImGui::OpenPopup("RenameEntityPopup");
		const auto& scene_entity = selected_entity_.get<engine::ecs::SceneEntity>();
		std::strncpy(rename_entity_buffer_, scene_entity.name.c_str(), sizeof(rename_entity_buffer_) - 1);
		state_.show_rename_entity_dialog = false;
	}

	if (selected_entity_ && ImGui::BeginPopupModal("RenameEntityPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::InputText("##rename", rename_entity_buffer_, sizeof(rename_entity_buffer_));
		if (ImGui::Button("Ok", ImVec2(120, 0)) && selected_entity_.set_name(rename_entity_buffer_)) {
			auto& scene_entity = selected_entity_.get_mut<engine::ecs::SceneEntity>();
			scene_entity.name = std::string(rename_entity_buffer_);
			rename_entity_buffer_[0] = '\0';
			// Note: Renaming doesn't go through command history yet (would need a RenameEntityCommand)
			// Mark scene as modified
			OnSceneModified();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void EditorScene::RenderMenuBar() {
	constexpr float MENU_PADDING = 6.0f;
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {MENU_PADDING, MENU_PADDING});
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New", "Ctrl+N")) {
				state_.show_new_scene_dialog = true;
			}
			if (ImGui::MenuItem("Open...", "Ctrl+O")) {
				file_path_buffer_[0] = '\0'; // Clear buffer for new input
				state_.show_open_dialog = true;
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Save", "Ctrl+S")) {
				if (state_.current_file_path.empty()) {
					file_path_buffer_[0] = '\0'; // Clear buffer for new input
					state_.show_save_as_dialog = true;
				}
				else {
					SaveScene();
				}
			}
			if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
				file_path_buffer_[0] = '\0'; // Clear buffer for new input
				state_.show_save_as_dialog = true;
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Exit", "Alt+F4")) {
				// TODO: Handle exit with unsaved changes check
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit")) {
			if (ImGui::MenuItem("Undo", "Ctrl+Z", false, command_history_.CanUndo())) {
				command_history_.Undo();
			}
			if (ImGui::MenuItem("Redo", "Ctrl+Y", false, command_history_.CanRedo())) {
				command_history_.Redo();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Cut", "Ctrl+X", false, selected_entity_.is_valid())) {
				CutEntity();
			}
			if (ImGui::MenuItem("Copy", "Ctrl+C", false, selected_entity_.is_valid())) {
				CopyEntity();
			}
			if (ImGui::MenuItem("Paste", "Ctrl+V", false, !clipboard_json_.empty())) {
				PasteEntity();
			}
			if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, selected_entity_.is_valid())) {
				DuplicateEntity();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View")) {
			ImGui::MenuItem("Hierarchy", nullptr, &hierarchy_panel_.VisibleRef());
			ImGui::MenuItem("Properties", nullptr, &properties_panel_.VisibleRef());
			ImGui::MenuItem("Viewport", nullptr, &viewport_panel_.VisibleRef());
			ImGui::MenuItem("Assets", nullptr, &asset_browser_panel_.VisibleRef());
			ImGui::MenuItem("Graph Editor", nullptr, &graph_editor_panel_.VisibleRef());
			ImGui::MenuItem("Shader Editor", nullptr, &shader_editor_panel_.VisibleRef());
			ImGui::MenuItem("Texture Editor", nullptr, &texture_editor_panel_.VisibleRef());
			ImGui::MenuItem("Animation Editor", nullptr, &animation_editor_panel_.VisibleRef());
			ImGui::MenuItem("Behavior Tree Editor", nullptr, &behavior_tree_editor_panel_.VisibleRef());
			ImGui::MenuItem("Tileset Editor", nullptr, &tileset_editor_panel_.VisibleRef());
			ImGui::MenuItem("Data Table Editor", nullptr, &data_table_editor_panel_.VisibleRef());
			ImGui::MenuItem("Code Editor", nullptr, &code_editor_panel_.VisibleRef());
			ImGui::MenuItem("Sound Editor", nullptr, &sound_editor_panel_.VisibleRef());
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Scene")) {
			if (ImGui::MenuItem("Add Entity")) {
				// Add a new entity to the scene using command
				auto& scene_manager = engine::scene::GetSceneManager();
				if (auto* scene = scene_manager.TryGetScene(editor_scene_id_)) {
					const std::string entity_name = MakeUniqueEntityName("NewEntity", scene);
					auto command = std::make_unique<CreateEntityCommand>(scene, entity_name, engine::ecs::Entity());
					command_history_.Execute(std::move(command));
				}
			}
			ImGui::EndMenu();
		}

		ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 30); // Centered approx
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {MENU_PADDING / 2.f, MENU_PADDING / 2.f});
		ImGui::SetCursorPosY(MENU_PADDING / 2.f);
		// Play/Stop buttons
		if (!state_.is_running) {
			if (ImGui::Button("Play")) {
				PlayScene();
			}
		}
		else {
			if (ImGui::Button("Stop")) {
				StopScene();
			}
		}
		ImGui::PopStyleVar();

		// Display current scene info on the right side of menu bar
		std::string title = state_.current_file_path.empty() ? "Untitled" : state_.current_file_path;
		if (command_history_.IsDirty()) {
			title += " *";
		}
		const float text_width = ImGui::CalcTextSize(title.c_str()).x;
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - text_width - 20);
		ImGui::TextDisabled("%s", title.c_str());

		ImGui::EndMainMenuBar();
	}
	ImGui::PopStyleVar();
}

// ========================================================================
// Callback Handlers
// ========================================================================

void EditorScene::OnEntitySelected(const engine::ecs::Entity entity) {
	selected_entity_ = entity;
	// Clear asset selection when entity is selected
	selected_asset_.Clear();
	selection_type_ = entity.is_valid() ? SelectionType::Entity : SelectionType::None;
}

void EditorScene::OnEntityDeleted(const engine::ecs::Entity entity) {
	if (selected_entity_ == entity) {
		selected_entity_ = {};
	}
}

void EditorScene::OnSceneModified() {
	// Scene modification now tracked through command history
	// No need to manually set dirty flag
}

void EditorScene::OnShowRenameDialog(const engine::ecs::Entity entity) {
	selected_entity_ = entity;
	state_.show_rename_entity_dialog = true;
}

void EditorScene::OnAddChildEntity(const engine::ecs::Entity parent) {
	auto& scene_manager = engine::scene::GetSceneManager();
	if (auto* scene = scene_manager.TryGetScene(editor_scene_id_)) {
		const std::string entity_name = MakeUniqueEntityName("NewEntity", scene);
		auto command = std::make_unique<CreateEntityCommand>(scene, entity_name, parent);
		// Store the command pointer to get the created entity
		auto* cmd_ptr = command.get();
		command_history_.Execute(std::move(command));
		// Select the newly created entity
		selected_entity_ = cmd_ptr->GetCreatedEntity();
	}
}

void EditorScene::OnAddComponent(const engine::ecs::Entity entity, const std::string& component_name) {
	const auto& registry = engine::ecs::ComponentRegistry::Instance();
	if (const auto* comp = registry.FindComponent(component_name)) {
		// Use flecs API directly to add component by ID
		entity.add(comp->id);
		// Note: Adding components doesn't go through command history yet (would need an AddComponentCommand)
		// Mark scene as modified by executing a no-op to maintain dirty state
		std::cout << "EditorScene: Added component '" << component_name << "' to entity" << std::endl;
	}
	else {
		std::cerr << "EditorScene: Component '" << component_name << "' not found in registry" << std::endl;
	}
}

void EditorScene::OnAssetSelected(const engine::scene::AssetType type, const std::string& name) {
	// Clear entity selection when asset is selected
	selected_entity_ = {};
	selection_type_ = SelectionType::Asset;
	selected_asset_.type = type;
	selected_asset_.name = name;
}

void EditorScene::OnAssetDeleted(const engine::scene::AssetType type, const std::string& name) {
	// Clear selection if the deleted asset was selected
	if (selected_asset_.type == type && selected_asset_.name == name) {
		selected_asset_.Clear();
		selection_type_ = SelectionType::None;
	}
}

// ========================================================================
// File Operations
// ========================================================================

void EditorScene::NewScene() {
	std::cout << "EditorScene: Creating new scene..." << std::endl;

	const auto& scene_manager = engine::scene::GetSceneManager();

	// Destroy the old scene
	if (editor_scene_id_ != engine::scene::INVALID_SCENE) {
		scene_manager.DestroyScene(editor_scene_id_);
	}

	// Create a new scene
	editor_scene_id_ = scene_manager.CreateScene("UntitledScene");
	scene_manager.SetActiveScene(editor_scene_id_);

	// Reset state
	state_.current_file_path = "";
	selected_entity_ = {};
	selected_asset_.Clear();
	selection_type_ = SelectionType::None;
	file_path_buffer_[0] = '\0';
	rename_entity_buffer_[0] = '\0';
	hierarchy_panel_.ClearNodeState();
	command_history_.Clear();

	std::cout << "EditorScene: New scene created" << std::endl;
}

void EditorScene::OpenScene(const std::string& path) {
	std::cout << "EditorScene: Opening scene from: " << path << std::endl;

	auto& scene_manager = engine::scene::GetSceneManager();

	// Destroy the old scene
	if (editor_scene_id_ != engine::scene::INVALID_SCENE) {
		scene_manager.DestroyScene(editor_scene_id_);
	}

	// Load scene from file using engine serializer
	const engine::platform::fs::Path file_path(path);
	editor_scene_id_ = scene_manager.LoadSceneFromFile(file_path);

	if (editor_scene_id_ == engine::scene::INVALID_SCENE) {
		std::cerr << "EditorScene: Failed to load scene from: " << path << std::endl;
		// Fall back to creating a new empty scene
		editor_scene_id_ = scene_manager.CreateScene("Untitled");
	}

	scene_manager.SetActiveScene(editor_scene_id_);

	// Store the scene's active camera (loaded from file) before switching to editor camera
	scene_active_camera_ = engine_->ecs.GetActiveCamera();
	// Filter out editor camera in case it was serialized (shouldn't happen but be safe)
	if (scene_active_camera_ == editor_camera_) {
		scene_active_camera_ = {};
	}

	// HACK: Reset to editor camera for viewport rendering
	engine_->ecs.SetActiveCamera(editor_camera_);

	// Update state
	state_.current_file_path = path;
	selected_entity_ = {};
	selected_asset_.Clear();
	selection_type_ = SelectionType::None;
	hierarchy_panel_.ClearNodeState();
	command_history_.Clear();

	std::cout << "EditorScene: Scene loaded from: " << path << std::endl;
}

void EditorScene::SaveScene() {
	if (state_.current_file_path.empty()) {
		std::cout << "EditorScene: No file path set, cannot save" << std::endl;
		return;
	}

	std::cout << "EditorScene: Saving scene to: " << state_.current_file_path << std::endl;

	auto& scene_manager = engine::scene::GetSceneManager();

	// HACK: Before saving, switch active camera from editor camera to scene's intended camera
	// This prevents the editor camera from being serialized as the active camera.
	// Use scene_active_camera_ if valid, otherwise set to invalid entity (no active camera)
	engine_->ecs.SetActiveCamera(scene_active_camera_);

	if (const engine::platform::fs::Path file_path(state_.current_file_path);
		scene_manager.SaveScene(editor_scene_id_, file_path)) {
		command_history_.SetSavePosition();
		std::cout << "EditorScene: Scene saved successfully" << std::endl;
	}
	else {
		std::cerr << "EditorScene: Failed to save scene" << std::endl;
	}

	// HACK: Restore editor camera as active after save
	engine_->ecs.SetActiveCamera(editor_camera_);
}

void EditorScene::SaveSceneAs(const std::string& path) {
	std::cout << "EditorScene: Saving scene as: " << path << std::endl;

	// Update the file path
	state_.current_file_path = path;

	// Update the scene file path
	auto& scene_manager = engine::scene::GetSceneManager();
	if (const auto* scene = scene_manager.TryGetScene(editor_scene_id_)) {
		scene->SetFilePath(path);
	}

	// Save
	SaveScene();
}

// ========================================================================
// Graph Editor Setup
// ========================================================================

void EditorScene::RegisterExampleGraphNodes() {
	using namespace engine::graph;
	auto& registry = NodeTypeRegistry::GetGlobal();

	// Math nodes
	{
		NodeTypeDefinition def("Add", "Math", "Add two values");
		def.default_inputs = {
				Pin(0, "A", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "B", PinType::Float, PinDirection::Input, 0.0f),
		};
		def.default_outputs = {Pin(0, "Result", PinType::Float, PinDirection::Output, 0.0f)};
		registry.Register(def);
	}
	{
		NodeTypeDefinition def("Multiply", "Math", "Multiply two values");
		def.default_inputs = {
				Pin(0, "A", PinType::Float, PinDirection::Input, 1.0f),
				Pin(0, "B", PinType::Float, PinDirection::Input, 1.0f),
		};
		def.default_outputs = {Pin(0, "Result", PinType::Float, PinDirection::Output, 0.0f)};
		registry.Register(def);
	}
	{
		NodeTypeDefinition def("Clamp", "Math", "Clamp value between min and max");
		def.default_inputs = {
				Pin(0, "Value", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "Min", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "Max", PinType::Float, PinDirection::Input, 1.0f),
		};
		def.default_outputs = {Pin(0, "Result", PinType::Float, PinDirection::Output, 0.0f)};
		registry.Register(def);
	}

	// Generator nodes
	{
		NodeTypeDefinition def("Constant", "Input", "A constant float value");
		def.default_outputs = {Pin(0, "Value", PinType::Float, PinDirection::Output, 0.0f)};
		registry.Register(def);
	}
	{
		NodeTypeDefinition def("Color", "Input", "A constant color value");
		def.default_outputs = {Pin(0, "Color", PinType::Color, PinDirection::Output, glm::vec4(1.0f))};
		registry.Register(def);
	}
	{
		NodeTypeDefinition def("Vec2", "Input", "A constant 2D vector");
		def.default_outputs = {Pin(0, "Vector", PinType::Vec2, PinDirection::Output, glm::vec2(0.0f))};
		registry.Register(def);
	}

	// Output nodes
	{
		NodeTypeDefinition def("Output", "Output", "Final output value");
		def.default_inputs = {Pin(0, "Value", PinType::Any, PinDirection::Input, 0.0f)};
		registry.Register(def);
	}
}

void EditorScene::CreateExampleGraph() {
	auto& graph = graph_editor_panel_.GetGraph();

	// Create a simple example: Constant(5) + Constant(3) → Output
	const int const_a = graph.AddNode("Constant", glm::vec2(50.0f, 50.0f));
	const int const_b = graph.AddNode("Constant", glm::vec2(50.0f, 200.0f));
	const int add_node = graph.AddNode("Add", glm::vec2(300.0f, 100.0f));
	const int output = graph.AddNode("Output", glm::vec2(550.0f, 100.0f));

	// Set up pins on nodes from registry definitions
	auto& registry = engine::graph::NodeTypeRegistry::GetGlobal();
	auto setup_pins = [&](int node_id, const std::string& type_name) {
		if (auto* node = graph.GetNode(node_id)) {
			if (const auto* def = registry.Get(type_name)) {
				node->inputs = def->default_inputs;
				node->outputs = def->default_outputs;
			}
		}
	};
	setup_pins(const_a, "Constant");
	setup_pins(const_b, "Constant");
	setup_pins(add_node, "Add");
	setup_pins(output, "Output");

	// Connect: Constant A output → Add input A
	graph.AddLink(const_a, 0, add_node, 0);
	// Connect: Constant B output → Add input B
	graph.AddLink(const_b, 0, add_node, 1);
	// Connect: Add result → Output value
	graph.AddLink(add_node, 0, output, 0);
}

// ========================================================================
// Scene Control
// ========================================================================

void EditorScene::PlayScene() {
	std::cout << "EditorScene: Playing scene..." << std::endl;

	// Snapshot the current scene state so we can restore it on Stop
	const auto& scene_manager = engine::scene::GetSceneManager();
	const auto& scene = scene_manager.GetScene(editor_scene_id_);
	play_mode_snapshot_ = engine::scene::SceneSerializer::SnapshotEntities(scene, engine_->ecs);

	// Deselect to avoid dangling entity references after restore
	selected_entity_ = {};
	selection_type_ = SelectionType::None;

	state_.is_running = true;
}

void EditorScene::StopScene() {
	std::cout << "EditorScene: Stopping scene..." << std::endl;
	state_.is_running = false;

	// Deselect before destroying entities
	selected_entity_ = {};
	selection_type_ = SelectionType::None;

	// Destroy all scene entities, then restore from snapshot
	auto& scene_manager = engine::scene::GetSceneManager();
	auto& scene = scene_manager.GetScene(editor_scene_id_);
	const auto entities = scene.GetAllEntities();
	for (auto entity : entities) {
		entity.destruct();
	}

	// Restore entities from snapshot
	if (!play_mode_snapshot_.empty()) {
		engine::scene::SceneSerializer::RestoreEntities(play_mode_snapshot_, scene, engine_->ecs);
		play_mode_snapshot_.clear();
	}

	// Ensure editor camera is active again
	engine_->ecs.SetActiveCamera(editor_camera_);
}

// ========================================================================
// Clipboard Operations
// ========================================================================

void EditorScene::CopyEntity() {
	if (!selected_entity_.is_valid()) {
		std::cout << "EditorScene: No entity selected to copy" << std::endl;
		return;
	}

	try {
		// Create clipboard JSON format
		json clipboard_data;
		clipboard_data["entities"] = json::array();

		// Serialize entity and all children
		std::function<void(engine::ecs::Entity, json&)> serialize_tree = [&](const engine::ecs::Entity entity,
																			 json& entities_array) {
			if (!entity.is_valid()) {
				return;
			}

			// Serialize this entity
			json entity_entry;
			entity_entry["path"] = entity.path().c_str();
			entity_entry["data"] = entity.to_json().c_str();
			entities_array.push_back(entity_entry);

			// Serialize children
			entity.children([&](const engine::ecs::Entity child) { serialize_tree(child, entities_array); });
		};

		serialize_tree(selected_entity_, clipboard_data["entities"]);

		// Store in clipboard
		clipboard_json_ = clipboard_data.dump();

		// Also copy to OS clipboard using ImGui
		ImGui::SetClipboardText(clipboard_json_.c_str());

		const std::string entity_name = selected_entity_.name().c_str();
		std::cout << "EditorScene: Copied entity '" << entity_name << "' to clipboard" << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "EditorScene: Error copying entity: " << e.what() << std::endl;
	}
}

void EditorScene::CutEntity() {
	if (!selected_entity_.is_valid()) {
		std::cout << "EditorScene: No entity selected to cut" << std::endl;
		return;
	}

	// Copy the entity first
	CopyEntity();

	// Then delete it using command for undo support
	auto& scene_manager = engine::scene::GetSceneManager();
	if (auto* scene = scene_manager.TryGetScene(editor_scene_id_)) {
		auto command = std::make_unique<CutEntityCommand>(scene, engine_->ecs, selected_entity_);
		command_history_.Execute(std::move(command));

		// Deselect the cut entity
		selected_entity_ = {};
		selection_type_ = SelectionType::None;

		std::cout << "EditorScene: Cut entity" << std::endl;
	}
}

void EditorScene::PasteEntity() {
	if (clipboard_json_.empty()) {
		// Try to get from OS clipboard
		const char* clipboard_text = ImGui::GetClipboardText();
		if (clipboard_text && clipboard_text[0] != '\0') {
			clipboard_json_ = clipboard_text;
		}
		else {
			std::cout << "EditorScene: Clipboard is empty" << std::endl;
			return;
		}
	}

	auto& scene_manager = engine::scene::GetSceneManager();
	if (auto* scene = scene_manager.TryGetScene(editor_scene_id_)) {
		// Paste under the selected entity if it's a valid parent (or use scene root)
		engine::ecs::Entity parent;
		if (selected_entity_.is_valid() && selected_entity_.has<engine::components::Group>()) {
			parent = selected_entity_;
		}

		auto command = std::make_unique<PasteEntityCommand>(scene, engine_->ecs, clipboard_json_, parent, true);
		auto* cmd_ptr = command.get();
		command_history_.Execute(std::move(command));

		// Select the pasted entity
		selected_entity_ = cmd_ptr->GetPastedEntity();

		std::cout << "EditorScene: Pasted entity from clipboard" << std::endl;
	}
}

void EditorScene::DuplicateEntity() {
	if (!selected_entity_.is_valid()) {
		std::cout << "EditorScene: No entity selected to duplicate" << std::endl;
		return;
	}

	auto& scene_manager = engine::scene::GetSceneManager();
	if (auto* scene = scene_manager.TryGetScene(editor_scene_id_)) {
		auto command = std::make_unique<DuplicateEntityCommand>(scene, engine_->ecs, selected_entity_);
		auto* cmd_ptr = command.get();
		command_history_.Execute(std::move(command));

		// Select the duplicated entity
		selected_entity_ = cmd_ptr->GetDuplicatedEntity();

		const std::string entity_name = selected_entity_.name().c_str();
		std::cout << "EditorScene: Duplicated entity '" << entity_name << "'" << std::endl;
	}
}

} // namespace editor
