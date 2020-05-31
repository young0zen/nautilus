/*
 * Copyright 2017 - 2019  Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * The ThreadSafeQueueI class (queue interface)
 * Provides the expected thread queue interface
 * Used as the base for both user and kernel implementations
 */
#pragma once

#include <atomic>
#include <utility>

namespace MARC {

  template <typename T>
  class ThreadSafeQueueI {
    public:

      /*
       * Attempt to get the first value in the queue.
       * Returns true if a value was successfully written to the out parameter, false otherwise.
       */
      virtual bool tryPop (T& out) = 0;

      /*
       * Get the first value in the queue.
       * Will block until a value is available unless clear is called or the instance is destructed.
       * Returns true if a value was successfully written to the out parameter, false otherwise.
       */
      virtual bool waitPop (T& out) = 0;
      virtual bool waitPop (void) = 0;

      /*
       * Push a new value onto the queue.
       */
      virtual void push (T value) = 0;

      /*
       * Push a new value onto the queue if the queue size is less than maxSize.
       * Otherwise, wait for it to happen and then push the new value.
       */
      virtual bool waitPush (T value, int64_t maxSize) = 0;

      /*
       * Clear all items from the queue.
       */
      virtual void clear (void) = 0;

      /*
       * Check whether or not the queue is empty.
       */
      virtual bool empty (void) const = 0;

      /*
       * Return the number of elements in the queue.
       */
      virtual int64_t size (void) const = 0;

      /*
       * Returns whether or not this queue is valid.
       */
      virtual bool isValid(void) const = 0;

      /*
       * Invalidate the queue.
       * Used to ensure no conditions are being waited on in waitPop when
       * a thread or the application is trying to exit.
       * The queue is invalid after calling this method and it is an error
       * to continue using a queue after this method has been called.
       */
      virtual void invalidate(void) = 0 ;

      /*
       * Default constructor.
       */
      ThreadSafeQueue (void) = 0 ;

      /*
       * Not copyable.
       */
      ThreadSafeQueue (const ThreadSafeQueue & other) = delete;
      ThreadSafeQueue & operator= (const ThreadSafeQueue & other) = delete;

      /*
       * Not assignable.
       */
      ThreadSafeQueue (const ThreadSafeQueue && other) = delete;
      ThreadSafeQueue & operator= (const ThreadSafeQueue && other) = delete;
  };
}

