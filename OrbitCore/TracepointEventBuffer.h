// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ORBIT_CORE_TRACEPOINT_EVENT_BUFFER_H_
#define ORBIT_CORE_TRACEPOINT_EVENT_BUFFER_H_

#include <atomic>
#include <limits>
#include <map>
#include <vector>

#include "SamplingProfiler.h"
#include "Threading.h"
#include "capture_data.pb.h"

class TracepointEventBuffer {
 public:
  void AddTracepointEventAndMapToThreads(uint64_t time, uint64_t tracepoint_hash,
                                         int32_t process_id, int32_t thread_id, int32_t cpu,
                                         bool is_same_pid_as_target);

  [[nodiscard]] const std::map<uint64_t, orbit_client_protos::TracepointEventInfo>&
  GetTracepointsOfThread(int32_t thread_id) const;

  void ForEachTracepointEventOfThreadInTimeRange(
      int32_t thread_id, uint64_t min_tick, uint64_t max_tick,
      const std::function<void(const orbit_client_protos::TracepointEventInfo&)>& action) const;

  void ForEachTracepointEvent(
      const std::function<void(const orbit_client_protos::TracepointEventInfo&)>& action) const {
    ScopeLock lock(mutex_);
    for (auto const& entry : tracepoint_events_) {
      for (auto const& time_to_tracepoint_event : entry.second) {
        action(time_to_tracepoint_event.second);
      }
    }
  }

  uint32_t GetNumTracepointsForThreadId(int32_t thread_id);

  static const int32_t kAllTracepointsFakeTid;
  static const int32_t kNotTargetProcessThreadId;

 private:
  int32_t num_total_tracepoints_ = 0;

  mutable Mutex mutex_;
  std::map<int32_t, std::map<uint64_t, orbit_client_protos::TracepointEventInfo> >
      tracepoint_events_;
};

#endif  // ORBIT_CORE_TRACEPOINT_EVENT_BUFFER_H_
