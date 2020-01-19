#include "event_bus.hpp"

#include <iostream>

using namespace std;

bool EventBus::cancelCallback(uint64_t callback_id) {
  // Check if the callback with the given ID is present in the queue
  if (id_callback_map.find(callback_id) != id_callback_map.end()) {
    // Replace the callback function with a no-op
    id_callback_map[callback_id]->first = noopCallback;
    // Remove from the id->callback map
    id_callback_map.erase(callback_id);
    return true;
  }
  return false;
}

EventBus::eActionOnReturn EventBus::invokeCallbacks(
    string event_name, const string& event_argument) {
  preprocessEventName(event_name);

  auto& callback_queue = registered_callbacks_[event_name];
  while (!callback_queue.empty()) {
    const auto& [callback, id] = callback_queue.front();
    const auto action = callback(event_argument);

    id_callback_map.erase(id);
    callback_queue.pop_front();

    if (action == eActionOnReturn::kCancel) {
      // Remove remaining callbacks
      callback_queue.clear();
      return action;
    } else if (action == eActionOnReturn::kDefer) {
      // Leave the remaining callbacks in the queue
      return action;
    }
  }

  // Got to the end of the queue without special actions
  return eActionOnReturn::kNone;
}

uint64_t EventBus::addImpl(cbQueue& queue, callbackImpl&& new_callback) {
  // Insert the new callback into the queue
  queue.push_back({new_callback, next_unique_id});
  // Insert the iterator of the newly added callback into the id->callback map
  id_callback_map[next_unique_id] = --queue.end();
  // Increment the ID to keep assigning unique values to each callback
  return next_unique_id++;
}

// Converts a string to lowercase. Used when adding and invoking callbacks to
// simulate case-insensitive string comparisson.
void EventBus::preprocessEventName(std::string& event_name) {
  if (!case_sensitive_event_names_)
    transform(event_name.begin(), event_name.end(), event_name.begin(),
              ::tolower);
}