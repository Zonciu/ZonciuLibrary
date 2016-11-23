#include "zonciu/timer.hpp"
#include "zonciu/clock.hpp"
#include "zonciu/util.hpp"
#include "zonciu/lock.hpp"
#include "zonciu/random.hpp"
#include "zonciu/security.hpp"
#include "zonciu/singleton.hpp"
#include "zonciu/semaphor.hpp"
#include "zonciu/format.hpp"
#include "zonciu/json.hpp"
#include <iostream>
#include <stdio.h>
#include <gtest/gtest.h>
using namespace std;

TEST(Random_Num, random_num_test)
{
    EXPECT_LE(10, zonciu::Random::Num(10, 9999));
    EXPECT_GE(9999, zonciu::Random::Num(10, 9999));
}
int test(int argc,char*argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
int main(int argc, char *argv[])
{
    zonciu::Json js;
    cout<<as.dump();
    
}