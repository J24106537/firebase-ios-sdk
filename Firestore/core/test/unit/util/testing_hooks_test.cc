/*
 * Copyright 2023 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Firestore/core/src/util/testing_hooks.h"

#include <chrono>  // NOLINT(build/c++11)
#include <future>  // NOLINT(build/c++11)
#include <memory>
#include <thread>  // NOLINT(build/c++11)
#include <vector>  // NOLINT(build/c++11)

#include "Firestore/core/src/api/listener_registration.h"
#include "Firestore/core/src/util/defer.h"
#include "Firestore/core/test/unit/testutil/async_testing.h"

#include "gtest/gtest.h"

namespace {

using namespace std::chrono_literals;  // NOLINT(build/namespaces)

using firebase::firestore::api::ListenerRegistration;
using firebase::firestore::testutil::AsyncTest;
using firebase::firestore::util::Defer;
using firebase::firestore::util::TestingHooks;

using ExistenceFilterMismatchInfoAccumulator =
    firebase::firestore::testutil::AsyncAccumulator<
        TestingHooks::ExistenceFilterMismatchInfo>;

class TestingHooksTest : public ::testing::Test, public AsyncTest {
 public:
  void AssertAccumulatedObject(
      const std::shared_ptr<ExistenceFilterMismatchInfoAccumulator>&
          accumulator,
      TestingHooks::ExistenceFilterMismatchInfo expected) {
    Await(accumulator->WaitForObject());
    ASSERT_FALSE(accumulator->IsEmpty());
    TestingHooks::ExistenceFilterMismatchInfo info = accumulator->Shift();
    EXPECT_EQ(info.local_cache_count, expected.local_cache_count);
    EXPECT_EQ(info.existence_filter_count, expected.existence_filter_count);
    EXPECT_EQ(info.bloom_filter.has_value(), expected.bloom_filter.has_value());
    if (info.bloom_filter.has_value() && expected.bloom_filter.has_value()) {
      const TestingHooks::BloomFilterInfo& info_bloom_filter =
          info.bloom_filter.value();
      const TestingHooks::BloomFilterInfo& expected_bloom_filter =
          expected.bloom_filter.value();
      EXPECT_EQ(info_bloom_filter.applied, expected_bloom_filter.applied);
      EXPECT_EQ(info_bloom_filter.hash_count, expected_bloom_filter.hash_count);
      EXPECT_EQ(info_bloom_filter.bitmap_length,
                expected_bloom_filter.bitmap_length);
      EXPECT_EQ(info_bloom_filter.padding, expected_bloom_filter.padding);
    }
  }

  std::future<void> NotifyOnExistenceFilterMismatchAsync(
      TestingHooks::ExistenceFilterMismatchInfo info) {
    return Async([info]() {
      TestingHooks::GetInstance().NotifyOnExistenceFilterMismatch(info);
    });
  }
};

TEST_F(TestingHooksTest, GetInstanceShouldAlwaysReturnTheSameObject) {
  TestingHooks& testing_hooks1 = TestingHooks::GetInstance();
  TestingHooks& testing_hooks2 = TestingHooks::GetInstance();
  EXPECT_EQ(&testing_hooks1, &testing_hooks2);
}

TEST_F(TestingHooksTest, OnExistenceFilterMismatchCallbackShouldGetNotified) {
  auto accumulator = ExistenceFilterMismatchInfoAccumulator::NewInstance();
  std::shared_ptr<ListenerRegistration> listener_registration =
      TestingHooks::GetInstance().OnExistenceFilterMismatch(
          accumulator->AsCallback());
  Defer unregister_listener([=] { listener_registration->Remove(); });
  TestingHooks::BloomFilterInfo bloom_filter_info{true, 10, 11, 12};

  NotifyOnExistenceFilterMismatchAsync({123, 456, bloom_filter_info});

  AssertAccumulatedObject(accumulator, {123, 456, bloom_filter_info});
}

TEST_F(TestingHooksTest,
       OnExistenceFilterMismatchCallbackShouldGetNotifiedMultipleTimes) {
  auto accumulator = ExistenceFilterMismatchInfoAccumulator::NewInstance();
  std::shared_ptr<ListenerRegistration> listener_registration =
      TestingHooks::GetInstance().OnExistenceFilterMismatch(
          accumulator->AsCallback());
  Defer unregister_listener([=] { listener_registration->Remove(); });
  TestingHooks::BloomFilterInfo bloom_filter_info{true, 10, 11, 12};

  NotifyOnExistenceFilterMismatchAsync({111, 222, bloom_filter_info});
  AssertAccumulatedObject(accumulator, {111, 222, bloom_filter_info});
  NotifyOnExistenceFilterMismatchAsync({333, 444, bloom_filter_info});
  AssertAccumulatedObject(accumulator, {333, 444, bloom_filter_info});
  NotifyOnExistenceFilterMismatchAsync({555, 666, bloom_filter_info});
  AssertAccumulatedObject(accumulator, {555, 666, bloom_filter_info});
}

TEST_F(TestingHooksTest,
       OnExistenceFilterMismatchAllCallbacksShouldGetNotified) {
  auto accumulator1 = ExistenceFilterMismatchInfoAccumulator::NewInstance();
  auto accumulator2 = ExistenceFilterMismatchInfoAccumulator::NewInstance();
  std::shared_ptr<ListenerRegistration> listener_registration1 =
      TestingHooks::GetInstance().OnExistenceFilterMismatch(
          accumulator1->AsCallback());
  Defer unregister_listener1([=] { listener_registration1->Remove(); });
  std::shared_ptr<ListenerRegistration> listener_registration2 =
      TestingHooks::GetInstance().OnExistenceFilterMismatch(
          accumulator2->AsCallback());
  Defer unregister_listener2([=] { listener_registration2->Remove(); });
  TestingHooks::BloomFilterInfo bloom_filter_info{true, 10, 11, 12};

  NotifyOnExistenceFilterMismatchAsync({123, 456, bloom_filter_info});

  AssertAccumulatedObject(accumulator1, {123, 456, bloom_filter_info});
  AssertAccumulatedObject(accumulator2, {123, 456, bloom_filter_info});
}

TEST_F(TestingHooksTest,
       OnExistenceFilterMismatchCallbackShouldGetNotifiedOncePerRegistration) {
  auto accumulator = ExistenceFilterMismatchInfoAccumulator::NewInstance();
  std::shared_ptr<ListenerRegistration> listener_registration1 =
      TestingHooks::GetInstance().OnExistenceFilterMismatch(
          accumulator->AsCallback());
  Defer unregister_listener1([=] { listener_registration1->Remove(); });
  std::shared_ptr<ListenerRegistration> listener_registration2 =
      TestingHooks::GetInstance().OnExistenceFilterMismatch(
          accumulator->AsCallback());
  Defer unregister_listener2([=] { listener_registration1->Remove(); });
  TestingHooks::BloomFilterInfo bloom_filter_info{true, 10, 11, 12};

  NotifyOnExistenceFilterMismatchAsync({123, 456, bloom_filter_info});

  AssertAccumulatedObject(accumulator, {123, 456, bloom_filter_info});
  AssertAccumulatedObject(accumulator, {123, 456, bloom_filter_info});
  std::this_thread::sleep_for(250ms);
  EXPECT_TRUE(accumulator->IsEmpty());
}

TEST_F(TestingHooksTest,
       OnExistenceFilterMismatchShouldNotBeNotifiedAfterRemove) {
  auto accumulator = ExistenceFilterMismatchInfoAccumulator::NewInstance();
  std::shared_ptr<ListenerRegistration> registration =
      TestingHooks::GetInstance().OnExistenceFilterMismatch(
          accumulator->AsCallback());
  registration->Remove();
  TestingHooks::BloomFilterInfo bloom_filter_info{true, 10, 11, 12};

  NotifyOnExistenceFilterMismatchAsync({123, 456, bloom_filter_info});

  std::this_thread::sleep_for(250ms);
  EXPECT_TRUE(accumulator->IsEmpty());
}

TEST_F(TestingHooksTest, OnExistenceFilterMismatchRemoveShouldOnlyRemoveOne) {
  auto accumulator1 = ExistenceFilterMismatchInfoAccumulator::NewInstance();
  auto accumulator2 = ExistenceFilterMismatchInfoAccumulator::NewInstance();
  auto accumulator3 = ExistenceFilterMismatchInfoAccumulator::NewInstance();
  std::shared_ptr<ListenerRegistration> listener_registration1 =
      TestingHooks::GetInstance().OnExistenceFilterMismatch(
          accumulator1->AsCallback());
  Defer unregister_listener1([=] { listener_registration1->Remove(); });
  std::shared_ptr<ListenerRegistration> listener_registration2 =
      TestingHooks::GetInstance().OnExistenceFilterMismatch(
          accumulator2->AsCallback());
  Defer unregister_listener2([=] { listener_registration1->Remove(); });
  std::shared_ptr<ListenerRegistration> listener_registration3 =
      TestingHooks::GetInstance().OnExistenceFilterMismatch(
          accumulator3->AsCallback());
  Defer unregister_listener3([=] { listener_registration3->Remove(); });
  TestingHooks::BloomFilterInfo bloom_filter_info{true, 10, 11, 12};

  listener_registration2->Remove();

  NotifyOnExistenceFilterMismatchAsync({123, 456, bloom_filter_info});

  AssertAccumulatedObject(accumulator1, {123, 456, bloom_filter_info});
  AssertAccumulatedObject(accumulator3, {123, 456, bloom_filter_info});
  std::this_thread::sleep_for(250ms);
  EXPECT_TRUE(accumulator2->IsEmpty());
}

TEST_F(TestingHooksTest, OnExistenceFilterMismatchMultipleRemovesHaveNoEffect) {
  auto accumulator = ExistenceFilterMismatchInfoAccumulator::NewInstance();
  std::shared_ptr<ListenerRegistration> listener_registration =
      TestingHooks::GetInstance().OnExistenceFilterMismatch(
          accumulator->AsCallback());
  Defer unregister_listener([=] { listener_registration->Remove(); });
  listener_registration->Remove();
  listener_registration->Remove();
  listener_registration->Remove();
  TestingHooks::BloomFilterInfo bloom_filter_info{true, 10, 11, 12};

  NotifyOnExistenceFilterMismatchAsync({123, 456, bloom_filter_info});

  std::this_thread::sleep_for(250ms);
  EXPECT_TRUE(accumulator->IsEmpty());
}

}  // namespace
