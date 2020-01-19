#pragma once

#include <algorithm>
#include <functional>
#include <list>
#include <unordered_map>

/**
 * Queues up callbacks that are executed on invoked events.
 *
 * Supports callback cancelation and special actions based on the return value
 * from each callback. Callbacks can be any callable type that takes a parameter
 * that can be constructed from a string. Callbacks with special actions on
 * return need to return eActionOnReturn enum. Others can return any type.
 * Also supports case-insensitive event name matching.
 *
 */
class EventBus {
 public:
  EventBus(bool case_sensitive = true) noexcept
      : case_sensitive_event_names_(case_sensitive) {}

  /**
   * Enum that modifies the execution callbacks in the queue.
   *
   * Callbacks that want to alter the control flow of the remaining elements in
   * the queue need to return an object ofk this type.
   *
   */
  enum class eActionOnReturn {
    kNone = 0,    // Continue with execution of queued callbacks
    kCancel = 1,  // Cancel the remaining callbacks in the queue
    kDefer = 2  // Defer the remaining callbacks in the queue to be executed on
                // the next event
    // Note: other options can be added here
  };

  /**
   * Adds a callback to the queue of the specified event.
   *
   * Return value of the callback is ingored; Use addCallbackWithAction to alter
   * the execution flow from the callback.
   *
   * @param event_name Name of the event for which this callback is being
   * registered.
   * @param new_callback Callable to the executed when the event is invoked.
   * @return Unique ID of the registered callback; can be used to cancel the
   * callback before the event.
   */
  template <typename T>
  uint64_t addCallback(std::string event_name, T&& new_callback);

  /**
   * Adds a callback to the queue of the specified event.
   *
   * When using this function, the callback needs to return an eActionOnReturn
   * enum. If this is not needed, use addCallback.
   *
   * @param event_name Name of the event for which this callback is being
   * registered.
   * @param new_callback Callable to the executed when the event is invoked.
   * @return Unique ID of the registered callback; can be used to cancel the
   * callback before the event.
   */
  template <typename T>
  uint64_t addCallbackWithAction(std::string event_name, T&& new_callback);

  /**
   * Cancels the callback with the specified ID.
   *
   * Has no effects if a callback with the given ID has already been called, or
   * if the ID is invalid.
   *
   * @param calback_id ID of the callback to cancel (use the IDs returned from
   * the add function calls).
   * @return Whether a callback was actually canceled.
   */
  bool cancelCallback(uint64_t calback_id);

  /**
   * Executed callbacks registered for the specified event.
   *
   * Executes all callbacks in the appropriate queue, unless one the the
   * callbacks requests a special action.
   *
   * @param event_name Name of the current event.
   * @param event_argument String parameter of the event; passed to each
   * callback.
   * @return The special action taken per callback's request. If no actions were
   * requested, returns kNone.
   */
  eActionOnReturn invokeCallbacks(std::string event_name,
                                  const std::string& event_argument);

 private:
  using callbackImpl = std::function<eActionOnReturn(const std::string&)>;
  using cbQueue = std::list<std::pair<callbackImpl, uint64_t>>;
  std::unordered_map<std::string, cbQueue> registered_callbacks_;
  std::unordered_map<uint64_t, cbQueue::iterator> id_callback_map;

  const bool case_sensitive_event_names_ = true;
  uint64_t next_unique_id = 0;

  // Callback functions are replaced with this one when they are canceled
  static eActionOnReturn noopCallback(const std::string&) {
    return eActionOnReturn::kNone;
  }

  uint64_t addImpl(cbQueue& queue, callbackImpl&& new_callback);
  void preprocessEventName(std::string& event_name);
};

template <typename T>
uint64_t EventBus::addCallback(std::string event_name, T&& new_callback) {
  preprocessEventName(event_name);

  // Using a lambda here instead the functor directly allows some
  // flexibility in parameter types: anything taking a parameter that can be
  // created from a const string& is supported
  return addImpl(
      registered_callbacks_[event_name],
      [cb = std::forward<T>(new_callback)](const std::string& str) mutable {
        cb(str);
        // Ignore return value, always return no special action.
        return eActionOnReturn::kNone;
      });
}

template <typename T>
uint64_t EventBus::addCallbackWithAction(std::string event_name,
                                         T&& new_callback) {
  preprocessEventName(event_name);

  return addImpl(registered_callbacks_[event_name],
                 [cb = std::forward<T>(new_callback)](
                     const std::string& str) mutable -> eActionOnReturn {
                   // Propagate the special action request
                   return cb(str);
                 });
}