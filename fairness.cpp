// Copied from https://trac.webkit.org/browser/trunk/Source/WTF/benchmarks/LockFairnessTest.cpp?rev=200444
// but with less giant framework required.

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include <atomic>
#include <memory>

struct GlobalState {
  GlobalState() : mKeepGoing(true) {
    pthread_mutex_init(&mLock, nullptr);
  }

  pthread_mutex_t mLock;
  std::atomic<bool> mKeepGoing;
};

struct PerThreadState {
  PerThreadState(GlobalState* aGlobalState, unsigned aIndex)
    : mGlobalState(aGlobalState)
    , mThreadIndex(aIndex)
    , mAcquisitions(0)
  {}

  GlobalState* mGlobalState;
  pthread_t mThreadHandle;
  unsigned mThreadIndex;
  unsigned mAcquisitions;
};

void*
thread_func(void* arg)
{
  auto* state = static_cast<PerThreadState*>(arg);
  GlobalState* global = state->mGlobalState;

  while (global->mKeepGoing.load(std::memory_order_relaxed)) {
    pthread_mutex_lock(&global->mLock);
    state->mAcquisitions++;
    pthread_mutex_unlock(&global->mLock);
  }

  return nullptr;
}

int
main(int argc, char** argv)
{
  GlobalState state;

  const unsigned nThreads = 10;
  auto threadStates = std::make_unique<PerThreadState*[]>(nThreads);

  pthread_mutex_lock(&state.mLock);

  for (unsigned i = 0; i < nThreads; ++i) {
    threadStates[i] = new PerThreadState(&state, i);
    pthread_create(&threadStates[i]->mThreadHandle, nullptr,
		   thread_func, threadStates[i]);
  }

  // Let all the threads start.
  usleep(1000 * 1000);

  // Release the Kraken!
  pthread_mutex_unlock(&state.mLock);

  const useconds_t secondsPerTest = 100 * 1000;

  usleep(secondsPerTest);

  pthread_mutex_lock(&state.mLock);
  state.mKeepGoing.store(false, std::memory_order_relaxed);

  for (unsigned i = 0; i < nThreads; ++i) {
    printf("%u\n", threadStates[i]->mAcquisitions);
  }

  pthread_mutex_unlock(&state.mLock);

  for (unsigned i = 0; i < nThreads; ++i) {
    pthread_join(threadStates[i]->mThreadHandle, nullptr);
  }

  return 0;
}
