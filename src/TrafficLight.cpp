#include "TrafficLight.h"

#include <chrono>
#include <future>
#include <iostream>
#include <random>

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive() {
  // FP.5a : The method receive should use std::unique_lock<std::mutex> and
  //_condition.wait() to wait for and receive new messages and pull them from
  // the queue using move semantics.
  // The received object should then be returned by the receive function.

  std::unique_lock<std::mutex> lock(_mutex);

  _condition.wait(lock, [this] { return !_queue.empty(); });

  auto msg = std::move(_queue.back());
  _queue.back();

  return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg) {
  // FP.4a : The method send should use the mechanisms
  // std::lock_guard<std::mutex>
  // as well as _condition.notify_one() to add a new message to the queue
  // and afterwards send a notification.

  std::lock_guard<std::mutex> lock(_mutex);

  _queue.push_back(std::move(msg));

  _condition.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight() {
  _currentPhase = TrafficLightPhase::red;
  _messageQueue = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

void TrafficLight::waitForGreen() {
  // FP.5b : add the implementation of the method waitForGreen, in which an
  // infinite while-loop
  // runs and repeatedly calls the receive function on the message queue.
  // Once it receives TrafficLightPhase::green, the method returns.
  while (true) {
    if (_messageQueue->receive() == TrafficLightPhase::green) {
      return;
    }
  }
}

TrafficLightPhase TrafficLight::getCurrentPhase() { return _currentPhase; }

void TrafficLight::simulate() {
  // FP.2b : Finally, the private method „cycleThroughPhases“ should be
  // started in a thread when the public method „simulate“ is called.To do
  // this,
  //  use the thread queue in the base class.
  threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

void TrafficLight::toogleTrafficLight() {
  _currentPhase = (getCurrentPhase() == TrafficLightPhase::red)
                      ? TrafficLightPhase::green
                      : TrafficLightPhase::red;
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases() {
  // FP.2a : Implement the function with an infinite loop that measures the
  // time between two loop cycles
  // and toggles the current phase of the traffic light between red and
  // green
  // and sends an update method
  // to the message queue using move semantics. The cycle
  // duration should be a
  // random value between 4 and 6 seconds
  // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms
  // between two cycles.

  std::chrono::steady_clock::time_point last_update =
      std::chrono::steady_clock::now();
  std::srand(0);

  // std::unique_lock<std::mutex> lock(_mutex);
  // lock.unlock();

  int cycle_time = 4 + (std::rand() % (6 - 4 + 1));
  std::cout << "random_threshold: " << cycle_time << std::endl;

  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // measure time between two iterations
    auto time_since_last_update =
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - last_update)
            .count();

    if (time_since_last_update >= cycle_time) {
      toogleTrafficLight();

      auto is_sent =
          std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send,
                     _messageQueue, std::move(_currentPhase));
      is_sent.wait();

      // set new cycle time randomly
      cycle_time = 4 + (std::rand() % (6 - 4 + 1));

      // reset stopwatch for the next cycle
      last_update = std::chrono::steady_clock::now();
    }
  }
}
