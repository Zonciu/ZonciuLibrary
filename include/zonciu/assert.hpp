#ifndef ZONCIU_ASSERT_HPP
#define ZONCIU_ASSERT_HPP
#include <assert.h>
#define ZONCIU_ASSERT(condition,message) assert((condition)&& message)
#endif
