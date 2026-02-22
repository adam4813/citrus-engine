#include "editor_scene.h"
#include "commands/entity_commands.h"
#include "commands/prefab_command.h"
#include "commands/transform_command.h"
#include "editor_utils.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>

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

	// Set up scene file dialogs
	open_scene_dialog_.SetCallback([this](const std::string& path) { OpenScene(path); });
	save_scene_dialog_.SetCallback([this](const std::string& path) { SaveSceneAs(path); });

	// Create editor camera (not part of the scene, used for viewport navigation)
	// Manually created in Flecs ECS world, so it isn't under the scene root entity.
	// NOTE: Editor camera is automatically excluded from serialization because it's not
	// a child of the scene root. Only the active camera *reference* needs to be swapped
	// during save/load (see SaveScene/OpenScene camera hack).
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
	callbacks.on_scene_modified = [this]() {
		if (selected_prefab_entity_.is_valid() && selected_prefab_entity_.has(flecs::Prefab)) {
			engine::scene::PrefabUtility::SavePrefabTemplate(selected_prefab_entity_);
		}
		else {
			OnSceneModified();
		}
	};
	callbacks.on_show_rename_dialog = [this](const engine::ecs::Entity entity) { OnShowRenameDialog(entity); };
	callbacks.on_asset_selected = [this](const engine::assets::AssetType type, const std::string& name) {
		OnAssetSelected(type, name);
	};
	callbacks.on_asset_deleted = [this](const engine::assets::AssetType type, const std::string& name) {
		OnAssetDeleted(type, name);
	};
	callbacks.on_scene_camera_changed = [this](const engine::ecs::Entity camera) { scene_active_camera_ = camera; };
	callbacks.on_execute_command = [this](std::unique_ptr<ICommand> command) {
		if (selected_prefab_entity_.is_valid() && selected_prefab_entity_.has(flecs::Prefab)) {
			auto wrapped = std::make_unique<PrefabUpdateCommand>(std::move(command), selected_prefab_entity_);
			command_history_.Execute(std::move(wrapped));
		}
		else {
			command_history_.Execute(std::move(command));
		}
	};
	callbacks.on_instantiate_prefab = [this](const std::string& prefab_path) {
		auto& scene_manager = engine::scene::GetSceneManager();
		if (auto* scene = scene_manager.TryGetScene(editor_scene_id_)) {
			auto command =
					std::make_unique<InstantiatePrefabCommand>(prefab_path, scene, engine_->ecs, selected_entity_);
			const auto* cmd_ptr = command.get();
			command_history_.Execute(std::move(command));
			if (cmd_ptr->GetInstance().is_valid()) {
				OnEntitySelected(cmd_ptr->GetInstance());
			}
		}
	};
	callbacks.on_copy_entity = [this]() { CopyEntity(); };
	callbacks.on_paste_entity = [this]() { PasteEntity(); };
	callbacks.on_duplicate_entity = [this]() { DuplicateEntity(); };

	// Collect all panels for iteration (View menu, asset registration)
	panels_ = {
			&hierarchy_panel_,
			&properties_panel_,
			&viewport_panel_,
			&asset_browser_panel_,
			&graph_editor_panel_,
			&shader_editor_panel_,
			&texture_editor_panel_,
			&animation_editor_panel_,
			&behavior_tree_editor_panel_,
			&tileset_editor_panel_,
			&sprite_editor_panel_,
			&data_table_editor_panel_,
			&code_editor_panel_,
			&sound_editor_panel_,
			&material_editor_panel_,
	};

	// Set default visibility for panels that should be visible on startup
	hierarchy_panel_.SetVisible(true);
	properties_panel_.SetVisible(true);
	viewport_panel_.SetVisible(true);
	asset_browser_panel_.SetVisible(true);

	// Each panel self-registers its asset type handlers
	for (auto* panel : panels_) {
		panel->RegisterAssetHandlers(asset_editor_registry_);
	}

	// Register prefab handler (needs EditorScene state, not panel-owned)
	asset_editor_registry_.Register("prefab", [this](const std::string& path) {
		auto& scene_manager = engine::scene::GetSceneManager();
		if (auto* scene = scene_manager.TryGetScene(editor_scene_id_)) {
			auto command = std::make_unique<InstantiatePrefabCommand>(path, scene, engine_->ecs, selected_entity_);
			const auto* cmd_ptr = command.get();
			command_history_.Execute(std::move(command));
			if (cmd_ptr->GetInstance().is_valid()) {
				OnEntitySelected(cmd_ptr->GetInstance());
			}
		}
	});

	// Generic asset file opener — dispatches via registry
	callbacks.on_open_asset_file = [this](const std::string& path) { asset_editor_registry_.TryOpen(path); };

	callbacks.on_open_file = [this](const std::string& path) {
		code_editor_panel_.OpenFile(path);
		code_editor_panel_.SetVisible(true);
	};
	callbacks.on_file_selected = [this](const std::string& path) {
		// When a prefab file is selected, load it and display its properties
		if (path.ends_with(".prefab.json")) {
			if (const auto prefab_entity = engine::scene::PrefabUtility::LoadPrefab(path, engine_->ecs);
				prefab_entity.is_valid()) {
				selected_entity_ = prefab_entity;
				selected_asset_.Clear();
				selection_type_ = SelectionType::Entity;
				selected_prefab_entity_ = prefab_entity;
				return;
			}
		}
		// Non-prefab file selected: clear prefab tracking
		selected_prefab_entity_ = {};
	};

	hierarchy_panel_.SetCallbacks(callbacks);
	hierarchy_panel_.SetWorld(&engine.ecs);
	properties_panel_.SetCallbacks(callbacks);
	asset_browser_panel_.SetCallbacks(callbacks);
	viewport_panel_.SetCallbacks(callbacks);

	// Register example node types for the graph editor
	RegisterExampleGraphNodes();

	// Register shader-specific node types for the shader editor
	RegisterShaderGraphNodes(shader_editor_panel_.GetRegistry());

	// Create a demo graph so the panel isn't empty
	CreateExampleGraph();

	// Notify all panels that engine + GL context are fully ready
	for (auto* panel : panels_) {
		panel->OnInitialized();
	}

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
		ImGui::DockBuilderDockWindow("Sprite Editor", dock_id_bottom);
		ImGui::DockBuilderDockWindow("Code Editor", dock_id_bottom);
		ImGui::DockBuilderDockWindow("Data Table Editor", dock_id_bottom);
		ImGui::DockBuilderDockWindow("Build Output", dock_id_bottom);
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
	shader_editor_panel_.Render(scene);
	texture_editor_panel_.Render();
	animation_editor_panel_.Render();
	behavior_tree_editor_panel_.Render();
	tileset_editor_panel_.Render(engine);
	sprite_editor_panel_.Render(engine);
	data_table_editor_panel_.Render();
	sound_editor_panel_.Render();
	code_editor_panel_.Render();
	material_editor_panel_.Render();
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

	open_scene_dialog_.Render();
	save_scene_dialog_.Render();

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

// ========================================================================
// Callback Handlers
// ========================================================================

void EditorScene::OnEntitySelected(const engine::ecs::Entity entity) {
	selected_entity_ = entity;
	// Clear asset selection when entity is selected
	selected_asset_.Clear();
	selection_type_ = entity.is_valid() ? SelectionType::Entity : SelectionType::None;
	selected_prefab_entity_ = {};
}

void EditorScene::OnEntityDeleted(const engine::ecs::Entity entity) {
	if (selected_entity_ == entity) {
		selected_entity_ = {};
	}
}

void EditorScene::OnSceneModified() {
	// Scene modification now tracked through command history
}

void EditorScene::OnShowRenameDialog(const engine::ecs::Entity entity) {
	selected_entity_ = entity;
	state_.show_rename_entity_dialog = true;
}

void EditorScene::OnAssetSelected(const engine::assets::AssetType type, const std::string& name) {
	// Clear entity selection when asset is selected
	selected_entity_ = {};
	selection_type_ = SelectionType::Asset;
	selected_asset_.type = type;
	selected_asset_.name = name;
}

void EditorScene::OnAssetDeleted(const engine::assets::AssetType type, const std::string& name) {
	// Clear selection if the deleted asset was selected
	if (selected_asset_.type == type && selected_asset_.name == name) {
		selected_asset_.Clear();
		selection_type_ = SelectionType::None;
	}
}

// ========================================================================
// Graph Editor Setup
// ========================================================================

void EditorScene::RegisterExampleGraphNodes() {
	using namespace engine::graph;
	auto& registry = graph_editor_panel_.GetRegistry();

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
	auto& registry = graph_editor_panel_.GetRegistry();
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

	// Stop all playing sounds before restoring snapshot
	auto& audio_sys = engine::audio::AudioSystem::Get();
	if (audio_sys.IsInitialized()) {
		audio_sys.StopAllSounds();
	}

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

} // namespace editor
