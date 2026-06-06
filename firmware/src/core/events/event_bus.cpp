#include "core/events/event_bus.h"

namespace reeflow::core::events {
namespace {

EventBus eventBus;

}  // namespace

uint8_t EventBus::subscribe(EventType type, EventCallback callback,
                            void* context) {
  if (callback == nullptr) {
    return kInvalidSubscriptionId;
  }

  for (size_t index = 0; index < kMaxEventSubscriptions; ++index) {
    Subscription& subscription = subscriptions_[index];
    if (subscription.active) {
      continue;
    }

    subscription.active = true;
    subscription.id = nextSubscriptionId_++;
    if (nextSubscriptionId_ == kInvalidSubscriptionId) {
      nextSubscriptionId_ = 1;
    }
    subscription.type = type;
    subscription.callback = callback;
    subscription.context = context;
    return subscription.id;
  }

  return kInvalidSubscriptionId;
}

bool EventBus::unsubscribe(uint8_t subscriptionId) {
  if (subscriptionId == kInvalidSubscriptionId) {
    return false;
  }

  for (size_t index = 0; index < kMaxEventSubscriptions; ++index) {
    Subscription& subscription = subscriptions_[index];
    if (!subscription.active || subscription.id != subscriptionId) {
      continue;
    }

    subscription = {};
    return true;
  }

  return false;
}

PublishResult EventBus::publish(Event event) {
  PublishResult result = {};

  if (event.sequence == 0) {
    event.sequence = nextEventSequence_++;
  }

  uint8_t lastDeliveredId = kInvalidSubscriptionId;
  while (true) {
    Subscription* nextSubscription = nullptr;

    for (size_t index = 0; index < kMaxEventSubscriptions; ++index) {
      Subscription& subscription = subscriptions_[index];
      if (!subscription.active || subscription.type != event.type ||
          subscription.id <= lastDeliveredId) {
        continue;
      }

      if (nextSubscription == nullptr ||
          subscription.id < nextSubscription->id) {
        nextSubscription = &subscription;
      }
    }

    if (nextSubscription == nullptr) {
      break;
    }

    lastDeliveredId = nextSubscription->id;
    if (nextSubscription->callback(event, nextSubscription->context)) {
      result.deliveredCount += 1;
    } else {
      result.failedCount += 1;
    }
  }

  return result;
}

void EventBus::reset() {
  nextSubscriptionId_ = 1;
  nextEventSequence_ = 1;
  for (size_t index = 0; index < kMaxEventSubscriptions; ++index) {
    subscriptions_[index] = {};
  }
}

EventBus& defaultEventBus() {
  return eventBus;
}

}  // namespace reeflow::core::events
