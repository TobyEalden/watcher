#ifndef EVENT_H
#define EVENT_H

#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

struct Event {
  std::string path;
  bool isCreated;
  bool isDeleted;
  explicit Event(std::string path) : path(std::move(path)), isCreated(false), isDeleted(false) {}
};

class EventList {
 public:
  void create(std::string path) {
    std::lock_guard<std::mutex> l(mMutex);
    Event *event = internalUpdate(path);
    if (event->isDeleted) {
      event->isDeleted = false;
    } else {
      event->isCreated = true;
    }
  }

  Event *update(std::string path) {
    std::lock_guard<std::mutex> l(mMutex);
    return internalUpdate(path);
  }

  void remove(std::string path) {
    std::lock_guard<std::mutex> l(mMutex);
    Event *event = internalUpdate(path);
    event->isDeleted = true;
  }

  size_t size() {
    std::lock_guard<std::mutex> l(mMutex);
    return mEvents.size();
  }

  std::vector<Event> getEvents() {
    std::lock_guard<std::mutex> l(mMutex);
    std::vector<Event> eventsCloneVector;
    for (auto it = mEvents.begin(); it != mEvents.end(); ++it) {
      if (!(it->second.isCreated && it->second.isDeleted)) {
        eventsCloneVector.push_back(it->second);
      }
    }
    return eventsCloneVector;
  }

  void clear() {
    std::lock_guard<std::mutex> l(mMutex);
    mEvents.clear();
    mError.reset();
  }

  void error(std::string err) {
    std::lock_guard<std::mutex> l(mMutex);
    if (!mError.has_value()) {
      mError.emplace(std::move(err));
    }
  }

  bool hasError() {
    std::lock_guard<std::mutex> l(mMutex);
    return mError.has_value();
  }

  std::string getError() {
    std::lock_guard<std::mutex> l(mMutex);
    return mError.value_or("");
  }

 private:
  mutable std::mutex mMutex;
  std::map<std::string, Event> mEvents;
  std::optional<std::string> mError;
  Event *internalUpdate(std::string const &path) {
    auto found = mEvents.find(path);
    if (found == mEvents.end()) {
      auto it = mEvents.emplace(path, Event(path));
      return &it.first->second;
    }

    return &found->second;
  }
};

#endif
