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

#import "Firestore/Example/Tests/Util/FSTTestingHooks.h"

#include <functional>
#include <memory>
#include <vector>

#include "Firestore/core/src/util/testing_hooks.h"

#include "Firestore/core/src/api/listener_registration.h"
#include "Firestore/core/src/util/defer.h"
#include "Firestore/core/src/util/testing_hooks.h"
#include "Firestore/core/test/unit/testutil/async_testing.h"

using firebase::firestore::api::ListenerRegistration;
using firebase::firestore::testutil::AsyncAccumulator;
using firebase::firestore::util::Defer;
using firebase::firestore::util::TestingHooks;

// Add private "init" methods to FSTTestingHooksBloomFilter.
@interface FSTTestingHooksBloomFilter ()

- (instancetype)initWithApplied:(BOOL)applied
    hashCount:(int)hashCount
    bitmapLength:(int)bitmapLength
    padding:(int)padding
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithBloomFilterInfo:(const TestingHooks::BloomFilterInfo&)bloomFilterInfo;

@end

// Add private "init" methods to FSTTestingHooksExistenceFilterMismatchInfo.
@interface FSTTestingHooksExistenceFilterMismatchInfo ()

- (instancetype)initWithLocalCacheCount:(BOOL)localCacheCount
    existenceFilterCount:(int)existenceFilterCount
    bloomFilter:(FSTTestingHooksBloomFilter*)bloomFilter
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithExistenceFilterMismatchInfo:(const TestingHooks::ExistenceFilterMismatchInfo)existenceFilterMismatchInfo;

@end

@implementation FSTTestingHooks

+(NSArray<FSTTestingHooksExistenceFilterMismatchInfo*>*)captureExistenceFilterMismatchesDuringBlock:(void(^)())block {
  auto accumulator = AsyncAccumulator<TestingHooks::ExistenceFilterMismatchInfo>::NewInstance();

  TestingHooks& testing_hooks = TestingHooks::GetInstance();
  std::shared_ptr<ListenerRegistration> registration = testing_hooks.OnExistenceFilterMismatch(accumulator->AsCallback());
  Defer unregister_callback([registration]() { registration->Remove(); });

  block();

  std::vector<TestingHooks::ExistenceFilterMismatchInfo> mismatches;
  while (!accumulator->IsEmpty()) {
    mismatches.push_back(accumulator->Shift());
  }

  //return mismatches;
  return @[];
}

@end
