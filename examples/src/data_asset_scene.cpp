// Data Asset System Example Scene
// Demonstrates DataAsset, DataTable, Schema, and serialization

#include "example_scene.h"
#include "scene_registry.h"

import engine;
import glm;

#include <iostream>
#include <memory>

using namespace engine;
using namespace engine::data;

namespace examples {

class DataAssetScene : public ExampleScene {
public:
	const char* GetName() const override { return "Data Assets"; }

	const char* GetDescription() const override {
		return "Demonstrates DataAsset, DataTable, Schema, and JSON/CSV serialization";
	}

	void Initialize(engine::Engine& engine) override {
		std::cout << "=== Data Asset System Example ===" << std::endl;

		// 1. Register a schema for item data
		CreateItemSchema();

		// 2. Create some item assets
		CreateItemAssets();

		// 3. Create and populate a loot table
		CreateLootTable();

		// 4. Demonstrate serialization
		DemonstrateSerialization();

		std::cout << "\n✓ Data Asset System demonstration complete!" << std::endl;
		std::cout << "Check the console output to see the data." << std::endl;
	}

	void Shutdown(engine::Engine& engine) override {
		// Clean up registry
		auto& registry = DataAssetRegistry::Instance();
		registry.Clear();
	}

	void Update(engine::Engine& engine, float delta_time) override {
		// Nothing to update in this example
	}

	void Render(engine::Engine& engine) override {
		// Nothing to render in this example
	}

	void RenderUI(engine::Engine& engine) override {
		// Could add ImGui UI here to visualize the data
	}

private:
	void CreateItemSchema() {
		std::cout << "\n1. Creating Item Schema..." << std::endl;

		auto& registry = DataAssetRegistry::Instance();

		Schema item_schema("ItemData");
		item_schema.category = "Game/Items";
		item_schema.description = "Base schema for all game items";

		// Define fields with defaults
		item_schema.AddField({"name", "string", std::string("Unnamed Item")});
		item_schema.AddField({"description", "string", std::string("")});
		item_schema.AddField({"stack_size", "int", 1});
		item_schema.AddField({"sell_price", "float", 0.0f});
		item_schema.AddField({"rarity", "int", 0}); // 0=common, 1=rare, 2=epic, 3=legendary
		item_schema.AddField({"consumable", "bool", false});

		registry.RegisterSchema(item_schema);

		std::cout << "  Schema 'ItemData' registered with " << item_schema.fields.size()
				  << " fields" << std::endl;
	}

	void CreateItemAssets() {
		std::cout << "\n2. Creating Item Assets..." << std::endl;

		auto& registry = DataAssetRegistry::Instance();

		// Create Health Potion
		auto health_potion_opt = registry.CreateAssetFromSchema("ItemData", "item_health_potion");
		if (health_potion_opt) {
			auto& health_potion = health_potion_opt.value();
			health_potion.SetProperty("name", std::string("Health Potion"));
			health_potion.SetProperty("description",
									  std::string("Restores 50 HP when consumed"));
			health_potion.SetProperty("stack_size", 10);
			health_potion.SetProperty("sell_price", 15.0f);
			health_potion.SetProperty("rarity", 0); // common
			health_potion.SetProperty("consumable", true);

			std::cout << "  Created: "
					  << std::get<std::string>(health_potion.GetProperty("name")) << std::endl;
		}

		// Create Magic Sword
		auto sword_opt = registry.CreateAssetFromSchema("ItemData", "item_magic_sword");
		if (sword_opt) {
			auto& sword = sword_opt.value();
			sword.SetProperty("name", std::string("Flaming Sword"));
			sword.SetProperty("description", std::string("A sword wreathed in eternal flames"));
			sword.SetProperty("stack_size", 1);
			sword.SetProperty("sell_price", 500.0f);
			sword.SetProperty("rarity", 2); // epic
			sword.SetProperty("consumable", false);

			std::cout << "  Created: " << std::get<std::string>(sword.GetProperty("name"))
					  << std::endl;
		}

		// Create Gold Coins
		auto gold_opt = registry.CreateAssetFromSchema("ItemData", "item_gold");
		if (gold_opt) {
			auto& gold = gold_opt.value();
			gold.SetProperty("name", std::string("Gold Coins"));
			gold.SetProperty("description", std::string("Standard currency"));
			gold.SetProperty("stack_size", 9999);
			gold.SetProperty("sell_price", 1.0f);
			gold.SetProperty("rarity", 0);
			gold.SetProperty("consumable", false);

			std::cout << "  Created: " << std::get<std::string>(gold.GetProperty("name"))
					  << std::endl;
		}
	}

	void CreateLootTable() {
		std::cout << "\n3. Creating Loot Table..." << std::endl;

		DataTable loot_table("enemy_goblin_loot");

		// Define columns
		loot_table.AddColumn({"item_id"});
		loot_table.AddColumn({"drop_chance"});
		loot_table.AddColumn({"min_quantity"});
		loot_table.AddColumn({"max_quantity"});

		// Add loot entries
		DataRow gold_row("gold");
		gold_row.SetValue("item_id", std::string("item_gold"));
		gold_row.SetValue("drop_chance", 0.75f);
		gold_row.SetValue("min_quantity", 5);
		gold_row.SetValue("max_quantity", 20);
		loot_table.AddRow(gold_row);

		DataRow potion_row("potion");
		potion_row.SetValue("item_id", std::string("item_health_potion"));
		potion_row.SetValue("drop_chance", 0.30f);
		potion_row.SetValue("min_quantity", 1);
		potion_row.SetValue("max_quantity", 3);
		loot_table.AddRow(potion_row);

		DataRow sword_row("sword");
		sword_row.SetValue("item_id", std::string("item_magic_sword"));
		sword_row.SetValue("drop_chance", 0.01f); // rare drop
		sword_row.SetValue("min_quantity", 1);
		sword_row.SetValue("max_quantity", 1);
		loot_table.AddRow(sword_row);

		std::cout << "  Loot table created with " << loot_table.GetRowCount() << " entries:"
				  << std::endl;

		// Display the loot table
		for (const auto& row : loot_table.GetAllRows()) {
			std::string item_id = std::get<std::string>(row.GetValue("item_id"));
			float chance = std::get<float>(row.GetValue("drop_chance"));
			std::cout << "    - " << item_id << " (" << (chance * 100.0f) << "% drop chance)"
					  << std::endl;
		}
	}

	void DemonstrateSerialization() {
		std::cout << "\n4. Testing Serialization..." << std::endl;

		// Create a simple data asset
		DataAsset test_asset("test_config", "ConfigData");
		test_asset.SetProperty("max_players", 4);
		test_asset.SetProperty("difficulty", 1.5f);
		test_asset.SetProperty("pvp_enabled", true);

		// Serialize to JSON
		std::string json = DataSerializer::SerializeAsset(test_asset);
		std::cout << "  Serialized DataAsset to JSON:" << std::endl;
		std::cout << "  " << json.substr(0, 100) << "..." << std::endl;

		// Deserialize back
		DataAsset loaded = DataSerializer::DeserializeAsset(json);
		int max_players = std::get<int>(loaded.GetProperty("max_players"));
		std::cout << "  Deserialized: max_players = " << max_players << std::endl;

		// Test CSV export
		DataTable csv_table("test_table");
		csv_table.AddColumn({"name"});
		csv_table.AddColumn({"value"});

		DataRow row1("r1");
		row1.SetValue("name", std::string("Setting A"));
		row1.SetValue("value", std::string("100"));
		csv_table.AddRow(row1);

		std::string csv = DataSerializer::ExportTableToCSV(csv_table);
		std::cout << "  Exported DataTable to CSV:" << std::endl;
		std::cout << "  " << csv << std::endl;
	}
};

// Register the scene with the registry
REGISTER_EXAMPLE_SCENE(DataAssetScene, "Data Assets",
						"Demonstrates the data asset system with schemas, assets, and tables");

} // namespace examples
