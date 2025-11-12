module;

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
     * manager.RegisterRegion(
     *     Rectangle{100, 100, 300, 200},
     *     [](const MouseEvent& event) {
     *         // Handle modal events
     *         return true;  // Block all events when visible
     *     },
     *     100  // High priority
     * );
     * 
     * // Register button (normal priority)
     * manager.RegisterRegion(
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
            Rectangle bounds;           ///< Hit-test bounds (screen space)
            EventHandler handler;       ///< Callback to invoke
            int priority{0};            ///< Higher priority = called first
            bool enabled{true};         ///< Can be disabled without unregistering
            void* user_data{nullptr};   ///< Optional user data for identification
            
            EventRegion() = default;
            
            EventRegion(const Rectangle& bounds, EventHandler handler, int priority = 0, void* user_data = nullptr)
                : bounds(bounds), handler(std::move(handler)), priority(priority), user_data(user_data) {}
        };
        
        /**
         * @brief Handle type for managing registered regions
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
         * @return Handle for later unregistration or modification
         */
        RegionHandle RegisterRegion(const Rectangle& bounds, EventHandler handler, 
                                    int priority = 0, void* user_data = nullptr) {
            EventRegion region{bounds, std::move(handler), priority, user_data};
            regions_.push_back(std::move(region));
            
            // Sort by priority (descending)
            SortRegions();
            
            // Return handle (index after sorting)
            return regions_.size() - 1;
        }
        
        /**
         * @brief Unregister a region by handle
         * 
         * @param handle Handle returned from RegisterRegion()
         */
        void UnregisterRegion(RegionHandle handle) {
            if (handle < regions_.size()) {
                regions_.erase(regions_.begin() + handle);
            }
        }
        
        /**
         * @brief Unregister all regions with matching user_data
         * 
         * @param user_data User data to match
         * @return Number of regions removed
         */
        size_t UnregisterByUserData(void* user_data) {
            size_t count = 0;
            regions_.erase(
                std::remove_if(regions_.begin(), regions_.end(),
                    [user_data, &count](const EventRegion& region) {
                        if (region.user_data == user_data) {
                            ++count;
                            return true;
                        }
                        return false;
                    }),
                regions_.end()
            );
            return count;
        }
        
        /**
         * @brief Update region bounds (e.g., for animated/repositioned elements)
         * 
         * @param handle Handle of region to update
         * @param new_bounds New bounds rectangle
         */
        void UpdateRegionBounds(RegionHandle handle, const Rectangle& new_bounds) {
            if (handle < regions_.size()) {
                regions_[handle].bounds = new_bounds;
            }
        }
        
        /**
         * @brief Enable or disable a region without unregistering
         * 
         * @param handle Handle of region to update
         * @param enabled Enable state
         */
        void SetRegionEnabled(RegionHandle handle, bool enabled) {
            if (handle < regions_.size()) {
                regions_[handle].enabled = enabled;
            }
        }
        
        /**
         * @brief Dispatch mouse event to registered regions
         * 
         * Algorithm:
         * 1. Sort regions by priority (highest first)
         * 2. For each region (in priority order):
         *    - Check if event position is within bounds
         *    - If yes, call handler
         *    - If handler returns true, stop propagation
         * 
         * @param event Mouse event to dispatch
         * @return true if any handler consumed the event
         */
        bool DispatchEvent(const MouseEvent& event) {
            // Sort by priority before dispatch (in case priorities changed)
            SortRegions();
            
            // Dispatch to regions in priority order
            for (auto& region : regions_) {
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
        }
        
        /**
         * @brief Get number of registered regions
         */
        size_t GetRegionCount() const {
            return regions_.size();
        }
        
        /**
         * @brief Get region by handle (for inspection/debugging)
         */
        const EventRegion* GetRegion(RegionHandle handle) const {
            if (handle < regions_.size()) {
                return &regions_[handle];
            }
            return nullptr;
        }
        
    private:
        /**
         * @brief Sort regions by priority (descending)
         */
        void SortRegions() {
            std::sort(regions_.begin(), regions_.end(),
                [](const EventRegion& a, const EventRegion& b) {
                    return a.priority > b.priority;  // Descending
                });
        }
        
        /**
         * @brief Hit test helper
         */
        static bool Contains(const Rectangle& rect, float x, float y) {
            return x >= rect.x && x <= rect.x + rect.width &&
                   y >= rect.y && y <= rect.y + rect.height;
        }
        
        std::vector<EventRegion> regions_;
    };
}
