#ifndef WATCHER_H
#define WATCHER_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

#include "DirTree.hh"
#include "Glob.hh"
#include "Event.hh"

struct Watcher;
using WatcherRef = std::shared_ptr<Watcher>;

class WatcherState {
public:
  virtual ~WatcherState() = default;
};

struct Watcher {
  std::string mDir;
  std::unordered_set<std::string> mIgnorePaths;
  std::unordered_set<Glob> mIgnoreGlobs;
  EventList mEvents;
  std::shared_ptr<WatcherState> state;

  Watcher(std::string dir, std::unordered_set<std::string> ignorePaths,
          std::unordered_set<Glob> ignoreGlobs)
      : mDir(std::move(dir)),
        mIgnorePaths(std::move(ignorePaths)),
        mIgnoreGlobs(std::move(ignoreGlobs)) {}

  ~Watcher() = default;

  bool operator==(const Watcher &other) const {
    return mDir == other.mDir && mIgnorePaths == other.mIgnorePaths &&
           mIgnoreGlobs == other.mIgnoreGlobs;
  }

  void wait() {
    std::unique_lock<std::mutex> lk(mMutex);
    mCond.wait(lk);
  }

  void notify() {
    std::unique_lock<std::mutex> lk(mMutex);
    mCond.notify_all();
  }

  void notifyError(std::exception &err) { mEvents.error(err.what()); }

  bool isIgnored(std::string path) {
    for (auto it = mIgnorePaths.begin(); it != mIgnorePaths.end(); it++) {
      auto dir = *it + DIR_SEP;
      if (*it == path || path.compare(0, dir.size(), dir) == 0) {
        return true;
      }
    }

    auto basePath = mDir + DIR_SEP;
    if (path.rfind(basePath, 0) != 0) {
      return false;
    }

    auto relativePath = path.substr(basePath.size());
    for (auto it = mIgnoreGlobs.begin(); it != mIgnoreGlobs.end(); it++) {
      if (it->isIgnored(relativePath)) {
        return true;
      }
    }

    return false;
  }

  static WatcherRef getShared(std::string dir,
                              std::unordered_set<std::string> ignorePaths,
                              std::unordered_set<Glob> ignoreGlobs) {
    return std::make_shared<Watcher>(std::move(dir), std::move(ignorePaths),
                                     std::move(ignoreGlobs));
  }

private:
  std::mutex mMutex;
  std::condition_variable mCond;
};

class WatcherError : public std::runtime_error {
public:
  WatcherRef mWatcher;
  WatcherError(std::string msg, WatcherRef watcher)
      : std::runtime_error(std::move(msg)), mWatcher(std::move(watcher)) {}
  WatcherError(const char *msg, WatcherRef watcher)
      : std::runtime_error(msg), mWatcher(std::move(watcher)) {}
};

#endif
