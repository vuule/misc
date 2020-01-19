#define CATCH_CONFIG_MAIN

#include "external/catch2/catch.hpp"

#include <vector>

#include "event_bus.hpp"

using namespace std::placeholders;

using cbAction = EventBus::eActionOnReturn;

TEST_CASE("Test basic add->invoke", "[single-file]") {
  EventBus ebus;
  const std::string event_name = "EventName";
  int val = 0;
  ebus.addCallback(event_name, [&](const std::string& s) { val += s.size(); });

  const std::string arg = "sample arg";
  REQUIRE(ebus.invokeCallbacks(event_name, arg) == cbAction::kNone);
  REQUIRE(val == arg.size());
}

TEST_CASE("Test add->invoke with mismatched event names", "[single-file]") {
  EventBus ebus;
  const std::string event_name = "EventName";
  int val = 0;
  ebus.addCallback(event_name, [&](const std::string& s) { val += s.size(); });

  const std::string arg = "sample arg";
  REQUIRE(ebus.invokeCallbacks("OtherName", arg) == cbAction::kNone);
  REQUIRE(val == 0);
}

TEST_CASE("Test multiple event names", "[single-file]") {
  EventBus ebus;
  const std::vector<std::string> event_names = {"RecEmail", "SendEmail"};
  int val = 0;
  ebus.addCallback(event_names[0],
                   [&](const std::string& s) { val += s.size(); });
  ebus.addCallback(event_names[1],
                   [&](const std::string& s) { val *= s.size(); });

  const std::string arg = "word";
  REQUIRE(ebus.invokeCallbacks(event_names[0], arg) == cbAction::kNone);
  // Expect only the first callback to execute
  REQUIRE(val == arg.size());

  REQUIRE(ebus.invokeCallbacks(event_names[1], arg) == cbAction::kNone);
  // Expect only the second callback to execute
  REQUIRE(val == arg.size() * arg.size());
}

TEST_CASE("Test double invoke", "[single-file]") {
  EventBus ebus;
  const std::string event_name = "EventName";
  int val = 0;
  ebus.addCallback(event_name, [&](const std::string& s) { val += s.size(); });
  ebus.addCallback(event_name, [&](const std::string& s) { val *= s.size(); });

  const std::string arg = "word";
  REQUIRE(ebus.invokeCallbacks(event_name, arg) == cbAction::kNone);
  REQUIRE(ebus.invokeCallbacks(event_name, arg) == cbAction::kNone);
  REQUIRE(val == arg.size() * arg.size());
}

int testFreeFunction(std::string s) { return 0; }

float testStringView(std::string_view s) { return 1.f; }

void noret(std::string s) {}

int testFunctionExtraParameters(const std::string& s, int cnt) { return cnt; }

struct TestCallableClass {
  int operator()(const std::string& s) { return 0; }
};

class TestMemberFunc {
 public:
  char onEvent(std::string s) { return s[0]; }
  static char staticOnEvent(std::string s) { return 's'; }
};

TEST_CASE("Test different callable types", "[single-file]") {
  EventBus ebus;
  const std::string event_name = "EventName";
  // Lambda that takes a const string&
  ebus.addCallback(event_name, [](const std::string& s) { return s; });
  // Free function that takes a string
  ebus.addCallback(event_name, testFreeFunction);
  // Function that returns void
  ebus.addCallback(event_name, noret);
  // Free function that takes a string_view
  ebus.addCallback(event_name, testStringView);
  // Callable object
  ebus.addCallback(event_name, TestCallableClass());
  // Function which takes additional parameters; those are bound at this point
  ebus.addCallback(event_name, std::bind(testFunctionExtraParameters, _1, 2));

  TestMemberFunc obj;
  // Static member fuunction
  ebus.addCallback(event_name, TestMemberFunc::staticOnEvent);
  // Member fuunction
  ebus.addCallback(event_name, std::bind(&TestMemberFunc::onEvent, &obj, _1));

  // with addCallback, function return type is ignored; functions above return
  // different types
  REQUIRE(ebus.invokeCallbacks(event_name, "unused") == cbAction::kNone);
}

TEST_CASE("Test special ations - cancelation", "[single-file]") {
  EventBus ebus;
  const std::string event_name = "EventName";
  int val = 0;

  // Increase the val and send the cancel request if val value was odd
  auto cancelOdd = [&](const std::string& s) {
    return ++val % 2 ? cbAction::kNone : cbAction::kCancel;
  };
  for (int i = 0; i < 3; ++i) ebus.addCallbackWithAction(event_name, cancelOdd);

  const std::string arg = "unused";
  REQUIRE(ebus.invokeCallbacks(event_name, arg) == cbAction::kCancel);
  // Expect to cancel after the second callback
  REQUIRE(val == 2);

  // Expect the queue to be empty
  REQUIRE(ebus.invokeCallbacks(event_name, arg) == cbAction::kNone);
  REQUIRE(val == 2);
}

TEST_CASE("Test special ations - deferring", "[single-file]") {
  EventBus ebus;
  const std::string event_name = "EventName";
  int val = 0;

  auto cancelOdd = [&](const std::string& s) {
    return ++val % 2 ? cbAction::kNone : cbAction::kDefer;
  };
  for (int i = 0; i < 3; ++i) ebus.addCallbackWithAction(event_name, cancelOdd);

  const std::string arg = "unused";
  REQUIRE(ebus.invokeCallbacks(event_name, arg) == cbAction::kDefer);
  // Expect to cancel after the second callback
  REQUIRE(val == 2);

  // Expect the remaining callback to execute (without deferring - val is even)
  REQUIRE(ebus.invokeCallbacks(event_name, arg) == cbAction::kNone);
  REQUIRE(val == 3);
}

TEST_CASE("Test callback cancelation", "[single-file]") {
  EventBus ebus;
  const std::string event_name = "EventName";
  int val = 0;
  std::vector<uint64_t> ids;
  for (int i = 0; i < 3; ++i)
    ids.push_back(ebus.addCallback(
        event_name, [&val, i](const std::string& s) { val += i + 1; }));

  REQUIRE(ebus.cancelCallback(ids[1]));

  REQUIRE(ebus.invokeCallbacks(event_name, "unused") == cbAction::kNone);
  // Expect only the first and last callback to execute (val = 1 + 3)
  REQUIRE(val == 4);

  // Double cancelation should fail
  REQUIRE(!ebus.cancelCallback(ids[1]));

  // Cancelation with invalid ID should fail
  REQUIRE(!ebus.cancelCallback(100));
}

TEST_CASE("Test case insensitive event names", "[single-file]") {
  EventBus ebus(false);
  int val = 0;
  auto addLen = [&](const std::string& s) { val += s.size(); };
  ebus.addCallback("SendEmail", addLen);
  ebus.addCallback("SENDEMAIL", addLen);
  ebus.addCallback("sendemail", addLen);
  ebus.addCallback("Send email", addLen);

  const std::string arg = "test";
  REQUIRE(ebus.invokeCallbacks("SendEmail", arg) == cbAction::kNone);
  REQUIRE(val == 3 * arg.size());
}

#ifndef _DEBUG
TEST_CASE("Stress test", "[single-file]") {
  EventBus ebus;
  // Million callbacks
  const int num_callbacks = 1 << 20;
  int val = 0;
  const std::string event_name = "EventName";

  for (int i = 0; i < num_callbacks; ++i)
    ebus.addCallback(event_name,
                     [&](const std::string& s) { val += s.size(); });

  const std::string arg = "word";
  REQUIRE(ebus.invokeCallbacks(event_name, arg) == cbAction::kNone);
  REQUIRE(val == num_callbacks * arg.size());
}
#endif
