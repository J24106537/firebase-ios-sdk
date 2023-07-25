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

// NOTE: For Swift compatibility, please keep this header Objective-C only.
//       Swift cannot interact with any C++ definitions.
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface FSTTestingHooksExistenceFilterMismatchInfo : NSObject

@end

@interface FSTTestingHooks : NSObject

/**
 * Captures all existence filter mismatches in the Watch 'Listen' stream that occur during the
 * execution of the given block.
 *
 * @param block The block to execute; during the execution of this block all existence filter
 * mismatches will be captured.
 *
 * @return the captured existence filter mismatches.
 */
+(NSArray<FSTTestingHooksExistenceFilterMismatchInfo*>*)captureExistenceFilterMismatchesDuringBlock:(void(^)())block;

@end

NS_ASSUME_NONNULL_END
