// Data module test - tests DataAsset, DataTable, DataAssetRegistry, and DataSerializer

import engine.data;
import glm;

#include <cassert>
#include <iostream>
#include <string>

using namespace engine::data;

void test_data_asset() {
	std::cout << "Testing DataAsset..." << std::endl;

	// Create a data asset
	DataAsset asset("item_001", "ItemData");

	// Set properties
	asset.SetProperty("name", std::string("Health Potion"));
	asset.SetProperty("health", 50);
	asset.SetProperty("cost", 10.5f);
	asset.SetProperty("consumable", true);

	// Get properties
	auto name = std::get<std::string>(asset.GetProperty("name"));
	auto health = std::get<int>(asset.GetProperty("health"));
	auto cost = std::get<float>(asset.GetProperty("cost"));
	auto consumable = std::get<bool>(asset.GetProperty("consumable"));

	assert(name == "Health Potion");
	assert(health == 50);
	assert(cost == 10.5f);
	assert(consumable == true);

	// Check property existence
	assert(asset.HasProperty("name"));
	assert(!asset.HasProperty("nonexistent"));

	// Remove property
	asset.RemoveProperty("consumable");
	assert(!asset.HasProperty("consumable"));

	std::cout << "  ✓ DataAsset tests passed" << std::endl;
}

void test_data_table() {
	std::cout << "Testing DataTable..." << std::endl;

	// Create a data table
	DataTable table("LootTable");

	// Add columns
	table.AddColumn({"item_id"});
	table.AddColumn({"drop_chance"});
	table.AddColumn({"min_quantity"});
	table.AddColumn({"max_quantity"});

	// Add rows
	DataRow row1("row_1");
	row1.SetValue("item_id", std::string("sword_001"));
	row1.SetValue("drop_chance", 0.1f);
	row1.SetValue("min_quantity", 1);
	row1.SetValue("max_quantity", 1);
	table.AddRow(row1);

	DataRow row2("row_2");
	row2.SetValue("item_id", std::string("gold"));
	row2.SetValue("drop_chance", 0.8f);
	row2.SetValue("min_quantity", 10);
	row2.SetValue("max_quantity", 50);
	table.AddRow(row2);

	// Query rows
	assert(table.GetRowCount() == 2);
	assert(!table.IsEmpty());

	auto row_opt = table.GetRow("row_1");
	assert(row_opt.has_value());
	auto row = row_opt.value();
	assert(std::get<std::string>(row.GetValue("item_id")) == "sword_001");

	// Find by column value
	auto results = table.FindByColumn("item_id", std::string("gold"));
	assert(results.size() == 1);
	assert(std::get<float>(results[0].GetValue("drop_chance")) == 0.8f);

	// Remove row
	assert(table.RemoveRow("row_1"));
	assert(table.GetRowCount() == 1);

	std::cout << "  ✓ DataTable tests passed" << std::endl;
}

void test_data_asset_registry() {
	std::cout << "Testing DataAssetRegistry..." << std::endl;

	auto& registry = DataAssetRegistry::Instance();
	registry.Clear(); // Start fresh

	// Create a schema
	Schema item_schema("ItemData");
	item_schema.category = "Game/Items";
	item_schema.description = "Schema for item data";

	item_schema.AddField({"name", "string", std::string("Unnamed Item")});
	item_schema.AddField({"health", "int", 0});
	item_schema.AddField({"cost", "float", 0.0f});
	item_schema.AddField({"consumable", "bool", false});

	// Register schema
	registry.RegisterSchema(item_schema);

	// Verify registration
	assert(registry.HasSchema("ItemData"));
	auto schema_opt = registry.GetSchema("ItemData");
	assert(schema_opt.has_value());
	assert(schema_opt->name == "ItemData");
	assert(schema_opt->fields.size() == 4);

	// Create asset from schema
	auto asset_opt = registry.CreateAssetFromSchema("ItemData", "potion_001");
	assert(asset_opt.has_value());
	auto asset = asset_opt.value();
	assert(asset.id == "potion_001");
	assert(asset.type_name == "ItemData");

	// Check default values
	assert(std::get<std::string>(asset.GetProperty("name")) == "Unnamed Item");
	assert(std::get<int>(asset.GetProperty("health")) == 0);
	assert(std::get<float>(asset.GetProperty("cost")) == 0.0f);
	assert(std::get<bool>(asset.GetProperty("consumable")) == false);

	// Get all schema names
	auto names = registry.GetAllSchemaNames();
	assert(names.size() == 1);
	assert(names[0] == "ItemData");

	std::cout << "  ✓ DataAssetRegistry tests passed" << std::endl;
}

void test_data_serializer() {
	std::cout << "Testing DataSerializer..." << std::endl;

	// Test DataAsset serialization
	{
		DataAsset asset("test_001", "TestType");
		asset.SetProperty("name", std::string("Test Asset"));
		asset.SetProperty("value", 42);
		asset.SetProperty("ratio", 3.14f);
		asset.SetProperty("enabled", true);
		asset.SetProperty("position", glm::vec3(1.0f, 2.0f, 3.0f));

		// Serialize
		std::string json = DataSerializer::SerializeAsset(asset);
		assert(!json.empty());

		// Deserialize
		DataAsset loaded = DataSerializer::DeserializeAsset(json);
		assert(loaded.id == "test_001");
		assert(loaded.type_name == "TestType");
		assert(std::get<std::string>(loaded.GetProperty("name")) == "Test Asset");
		assert(std::get<int>(loaded.GetProperty("value")) == 42);
		assert(std::get<float>(loaded.GetProperty("ratio")) == 3.14f);
		assert(std::get<bool>(loaded.GetProperty("enabled")) == true);

		auto pos = std::get<glm::vec3>(loaded.GetProperty("position"));
		assert(pos.x == 1.0f && pos.y == 2.0f && pos.z == 3.0f);
	}

	// Test DataTable serialization
	{
		DataTable table("TestTable");
		table.AddColumn({"name"});
		table.AddColumn({"count"});

		DataRow row1("r1");
		row1.SetValue("name", std::string("Item A"));
		row1.SetValue("count", 10);
		table.AddRow(row1);

		DataRow row2("r2");
		row2.SetValue("name", std::string("Item B"));
		row2.SetValue("count", 20);
		table.AddRow(row2);

		// Serialize
		std::string json = DataSerializer::SerializeTable(table);
		assert(!json.empty());

		// Deserialize
		DataTable loaded = DataSerializer::DeserializeTable(json);
		assert(loaded.GetName() == "TestTable");
		assert(loaded.GetRowCount() == 2);
		assert(loaded.GetColumns().size() == 2);

		auto row_opt = loaded.GetRow("r1");
		assert(row_opt.has_value());
		assert(std::get<std::string>(row_opt->GetValue("name")) == "Item A");
		assert(std::get<int>(row_opt->GetValue("count")) == 10);
	}

	// Test Schema serialization
	{
		Schema schema("TestSchema");
		schema.category = "Test";
		schema.description = "A test schema";
		schema.AddField({"field1", "int", 42});
		schema.AddField({"field2", "string", std::string("default")});

		// Serialize
		std::string json = DataSerializer::SerializeSchema(schema);
		assert(!json.empty());

		// Deserialize
		Schema loaded = DataSerializer::DeserializeSchema(json);
		assert(loaded.name == "TestSchema");
		assert(loaded.category == "Test");
		assert(loaded.description == "A test schema");
		assert(loaded.fields.size() == 2);
		assert(loaded.fields[0].name == "field1");
		assert(std::get<int>(loaded.fields[0].default_value) == 42);
	}

	// Test CSV export/import
	{
		DataTable table("CSVTest");
		table.AddColumn({"name"});
		table.AddColumn({"value"});

		DataRow row1("row1");
		row1.SetValue("name", std::string("Alpha"));
		row1.SetValue("value", std::string("100"));
		table.AddRow(row1);

		DataRow row2("row2");
		row2.SetValue("name", std::string("Beta"));
		row2.SetValue("value", std::string("200"));
		table.AddRow(row2);

		// Export to CSV
		std::string csv = DataSerializer::ExportTableToCSV(table);
		assert(!csv.empty());
		assert(csv.find("key,name,value") != std::string::npos);

		// Import from CSV
		DataTable loaded = DataSerializer::ImportTableFromCSV(csv, "ImportedTable");
		assert(loaded.GetName() == "ImportedTable");
		assert(loaded.GetRowCount() == 2);
	}

	std::cout << "  ✓ DataSerializer tests passed" << std::endl;
}

int main() {
	std::cout << "Running Data Module Tests..." << std::endl;
	std::cout << "========================================" << std::endl;

	try {
		test_data_asset();
		test_data_table();
		test_data_asset_registry();
		test_data_serializer();

		std::cout << "========================================" << std::endl;
		std::cout << "All tests passed! ✓" << std::endl;
		return 0;
	}
	catch (const std::exception& e) {
		std::cerr << "Test failed with exception: " << e.what() << std::endl;
		return 1;
	}
}
