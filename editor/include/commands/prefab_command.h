#pragma once

#include "../command.h"

#include <flecs.h>
#include <memory>

import engine;

namespace editor {

/**
 * @brief Command wrapper that saves the prefab file after executing the inner command.
 *
 * Wraps any ICommand and calls SavePrefabTemplate after Execute/Undo/Redo,
 * so the prefab asset stays in sync with the in-memory entity.
 */
class PrefabUpdateCommand : public ICommand {
public:
	PrefabUpdateCommand(std::unique_ptr<ICommand> inner, engine::ecs::Entity prefab_entity) :
			inner_(std::move(inner)), prefab_entity_(prefab_entity) {}

	void Execute() override {
		inner_->Execute();
		engine::scene::PrefabUtility::SavePrefabTemplate(prefab_entity_);
	}

	void Undo() override {
		inner_->Undo();
		engine::scene::PrefabUtility::SavePrefabTemplate(prefab_entity_);
	}

	void Redo() override {
		inner_->Redo();
		engine::scene::PrefabUtility::SavePrefabTemplate(prefab_entity_);
	}

	std::string GetDescription() const override { return inner_->GetDescription() + " (Prefab)"; }

private:
	std::unique_ptr<ICommand> inner_;
	engine::ecs::Entity prefab_entity_;
};

} // namespace editor
