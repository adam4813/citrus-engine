module;

#include <unordered_map>
#include <vector>
#include <functional>
#include <algorithm>

export module engine.ui:mouse_event_manager;

import :mouse_event;
import engine.ui.batch_renderer;

export namespace engine::ui {
    using namespace batch_renderer;
    
    /**
     * @brief Priority-based mouse event handler registration
     * 
     * Allows flexible registration of UI components for mouse events with
     * explicit priorities. Handlers are sorted by priority (higher = first).
     * 
     * Inspired by towerforge's MouseEventManager architecture.
     * 
     * Use cases:
     * - Modal dialogs (high priority to block lower layers)
     * - Context menus (medium priority)
     * - Background panels (low priority)
     * 
     * @example
     * ```cpp
     * MouseEventManager manager;
     * 
     * // Register modal dialog (highest priority)
     * auto modal_handle = manager.RegisterRegion(
     *     Rectangle{100, 100, 300, 200},
     *     [](const MouseEvent& event) {
     *         // Handle modal events
     *         return true;  // Block all events when visible
     *     },
     *     100  // High priority
     * );
     * 
     * // Register button (normal priority)
     * auto button_handle = manager.RegisterRegion(
     *     Rectangle{150, 150, 100, 40},
     *     [](const MouseEvent& event) {
     *         if (event.left_pressed) {
     *             HandleClick();
     *             return true;
     *         }
     *         return false;
     *     },
     *     50  // Normal priority
     * );
     * 
     * // Handles remain valid across registrations and dispatches
     * manager.UpdateRegionBounds(button_handle, Rectangle{150, 200, 100, 40});
     * 
     * // Dispatch event (calls handlers in priority order)
     * MouseEvent event{200, 170, false, false, true, false};
     * manager.DispatchEvent(event);
     * ```
     */
    class MouseEventManager {
    public:
        /**
         * @brief Mouse event handler callback
         * 
         * @param event Mouse event to handle
         * @return true if event was consumed (stops propagation to lower priority handlers)
         */
        using EventHandler = std::function<bool(const MouseEvent&)>;
        
        /**
         * @brief Registered event region with handler and priority
         */
        struct EventRegion {
            size_t id{0};               ///< Unique stable identifier
            Rectangle bounds;           ///< Hit-test bounds (screen space)
            EventHandler handler;       ///< Callback to invoke
            int priority{0};            ///< Higher priority = called first
            bool enabled{true};         ///< Can be disabled without unregistering
            void* user_data{nullptr};   ///< Optional user data for identification
            
            EventRegion() = default;
            
            EventRegion(size_t id, const Rectangle& bounds, EventHandler handler, int priority = 0, void* user_data = nullptr)
                : id(id), bounds(bounds), handler(std::move(handler)), priority(priority), user_data(user_data) {}
        };
        
        /**
         * @brief Handle type for managing registered regions (stable across operations)
         */
        using RegionHandle = size_t;
        static constexpr RegionHandle INVALID_HANDLE = static_cast<size_t>(-1);
        
        /**
         * @brief Register a mouse event region with priority
         * 
         * @param bounds Hit-test rectangle (screen space)
         * @param handler Callback to invoke when event occurs in region
         * @param priority Higher priority handlers are called first (default: 0)
         * @param user_data Optional user data for identification
         * @return Stable handle for later unregistration or modification
         */
        RegionHandle RegisterRegion(const Rectangle& bounds, EventHandler handler, 
                                    int priority = 0, void* user_data = nullptr) {
            // Generate unique ID
            RegionHandle handle = next_id_++;
            
            // Create region with stable ID
            EventRegion region{handle, bounds, std::move(handler), priority, user_data};
            
            // Store in map for stable access
            regions_[handle] = std::move(region);
            
            // Mark as needing resort
            needs_sort_ = true;
            
            return handle;
        }
        
        /**
         * @brief Unregister a region by handle
         * 
         * @param handle Handle returned from RegisterRegion()
         * @return true if region was found and removed
         */
        bool UnregisterRegion(RegionHandle handle) {
            auto it = regions_.find(handle);
            if (it != regions_.end()) {
                regions_.erase(it);
                needs_sort_ = true;
                return true;
            }
            return false;
        }
        
        /**
         * @brief Unregister all regions with matching user_data
         * 
         * @param user_data User data to match
         * @return Number of regions removed
         */
        size_t UnregisterByUserData(void* user_data) {
            size_t count = 0;
            for (auto it = regions_.begin(); it != regions_.end(); ) {
                if (it->second.user_data == user_data) {
                    it = regions_.erase(it);
                    ++count;
                } else {
                    ++it;
                }
            }
            if (count > 0) {
                needs_sort_ = true;
            }
            return count;
        }
        
        /**
         * @brief Update region bounds (e.g., for animated/repositioned elements)
         * 
         * @param handle Handle of region to update
         * @param new_bounds New bounds rectangle
         * @return true if region was found and updated
         */
        bool UpdateRegionBounds(RegionHandle handle, const Rectangle& new_bounds) {
            auto it = regions_.find(handle);
            if (it != regions_.end()) {
                it->second.bounds = new_bounds;
                return true;
            }
            return false;
        }
        
        /**
         * @brief Enable or disable a region without unregistering
         * 
         * @param handle Handle of region to update
         * @param enabled Enable state
         * @return true if region was found and updated
         */
        bool SetRegionEnabled(RegionHandle handle, bool enabled) {
            auto it = regions_.find(handle);
            if (it != regions_.end()) {
                it->second.enabled = enabled;
                return true;
            }
            return false;
        }
        
        /**
         * @brief Dispatch mouse event to registered regions
         * 
         * Algorithm:
         * 1. Build sorted list of regions by priority (if needed)
         * 2. For each region (in priority order):
         *    - Check if event position is within bounds
         *    - If yes, call handler
         *    - If handler returns true, stop propagation
         * 
         * @param event Mouse event to dispatch
         * @return true if any handler consumed the event
         */
        bool DispatchEvent(const MouseEvent& event) {
            // Build sorted list if needed
            if (needs_sort_) {
                BuildSortedList();
            }
            
            // Dispatch to regions in priority order
            for (size_t handle : sorted_handles_) {
                auto it = regions_.find(handle);
                if (it == regions_.end()) {
                    continue;  // Region was removed
                }
                
                const auto& region = it->second;
                if (!region.enabled) {
                    continue;
                }
                
                // Hit test
                if (Contains(region.bounds, event.x, event.y)) {
                    // Invoke handler
                    if (region.handler && region.handler(event)) {
                        return true;  // Event consumed
                    }
                }
            }
            
            return false;  // No handler consumed event
        }
        
        /**
         * @brief Clear all registered regions
         */
        void Clear() {
            regions_.clear();
            sorted_handles_.clear();
            needs_sort_ = false;
        }
        
        /**
         * @brief Get number of registered regions
         */
        size_t GetRegionCount() const {
            return regions_.size();
        }
        
        /**
         * @brief Get region by handle (for inspection/debugging)
         * @return Pointer to region, or nullptr if handle is invalid
         */
        const EventRegion* GetRegion(RegionHandle handle) const {
            auto it = regions_.find(handle);
            if (it != regions_.end()) {
                return &it->second;
            }
            return nullptr;
        }
        
    private:
        /**
         * @brief Build sorted list of region handles by priority
         */
        void BuildSortedList() {
            sorted_handles_.clear();
            sorted_handles_.reserve(regions_.size());
            
            // Collect handles
            for (const auto& pair : regions_) {
                sorted_handles_.push_back(pair.first);
            }
            
            // Sort by priority (descending)
            std::sort(sorted_handles_.begin(), sorted_handles_.end(),
                [this](RegionHandle a, RegionHandle b) {
                    const auto& region_a = regions_.at(a);
                    const auto& region_b = regions_.at(b);
                    return region_a.priority > region_b.priority;  // Descending
                });
            
            needs_sort_ = false;
        }
        
        /**
         * @brief Hit test helper
         */
        static bool Contains(const Rectangle& rect, float x, float y) {
            return x >= rect.x && x <= rect.x + rect.width &&
                   y >= rect.y && y <= rect.y + rect.height;
        }
        
        std::unordered_map<RegionHandle, EventRegion> regions_;  ///< Map of stable ID to region
        std::vector<RegionHandle> sorted_handles_;               ///< Cached sorted list of handles
        RegionHandle next_id_{0};                                ///< Next unique ID to assign
        bool needs_sort_{false};                                 ///< Flag indicating sort is needed
    };
}
