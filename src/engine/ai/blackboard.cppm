module;

#include <any>
#include <optional>
#include <string>
#include <unordered_map>

export module engine.ai.blackboard;

export namespace engine::ai {

/**
 * @brief Shared data storage for behavior trees
 *
 * The Blackboard is a key-value store that allows nodes in a behavior tree
 * to share data. Uses std::any for type-erased storage.
 */
class Blackboard {
public:
	Blackboard() = default;
	~Blackboard() = default;

	// Disable copying (use shared_ptr if you need shared blackboards)
	Blackboard(const Blackboard&) = delete;
	Blackboard& operator=(const Blackboard&) = delete;

	// Allow moving
	Blackboard(Blackboard&&) noexcept = default;
	Blackboard& operator=(Blackboard&&) noexcept = default;

	/**
	 * @brief Set a value in the blackboard
	 * @tparam T Type of the value
	 * @param key The key to store the value under
	 * @param value The value to store
	 */
	template <typename T> void Set(const std::string& key, T value) { data_[key] = std::make_any<T>(std::move(value)); }

	/**
	 * @brief Get a value from the blackboard
	 * @tparam T Expected type of the value
	 * @param key The key to retrieve
	 * @return Optional containing the value if found and type matches, empty otherwise
	 */
	template <typename T> [[nodiscard]] std::optional<T> Get(const std::string& key) const {
		auto it = data_.find(key);
		if (it == data_.end()) {
			return std::nullopt;
		}

		try {
			return std::any_cast<T>(it->second);
		}
		catch (const std::bad_any_cast&) {
			return std::nullopt;
		}
	}

	/**
	 * @brief Check if a key exists in the blackboard
	 * @param key The key to check
	 * @return true if the key exists, false otherwise
	 */
	[[nodiscard]] bool Has(const std::string& key) const { return data_.find(key) != data_.end(); }

	/**
	 * @brief Remove a key from the blackboard
	 * @param key The key to remove
	 * @return true if the key was removed, false if it didn't exist
	 */
	bool Remove(const std::string& key) { return data_.erase(key) > 0; }

	/**
	 * @brief Clear all data from the blackboard
	 */
	void Clear() { data_.clear(); }

	/**
	 * @brief Get the number of entries in the blackboard
	 */
	[[nodiscard]] size_t Size() const { return data_.size(); }

	/**
	 * @brief Check if the blackboard is empty
	 */
	[[nodiscard]] bool IsEmpty() const { return data_.empty(); }

private:
	std::unordered_map<std::string, std::any> data_;
};

} // namespace engine::ai
