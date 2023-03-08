#include <stl.h>
#include <stl.cpp>
#include <utest.h>

UTEST(allocator_float_helper, Tests)
{
    
    // Denorms, exp=1 and exp=2 + mantissa = 0 are all precise.
    // NOTE: Assuming 8 value (3 bit) mantissa.
    // If this test fails, please change this assumption!
    uint32_t PreciseNumberCount = 17;
    for (uint32_t Index = 0; Index < PreciseNumberCount; Index++)
    {
        uint32_t RoundUp = ak_stl::allocator_float_helper::Round_Up_F32_Dist(Index);
        uint32_t RoundDown = ak_stl::allocator_float_helper::Round_Down_F32_Dist(Index);
        ASSERT_EQ(Index, RoundUp);
        ASSERT_EQ(Index, RoundDown);
    }
    
    // Test some random picked numbers
    struct number_float_up_down
    {
        uint32_t Number;
        uint32_t Up;
        uint32_t Down;
    };
    
    number_float_up_down TestData[] = {
        {17, 17, 16},
        {118, 39, 38},
        {1024, 64, 64},
        {65536, 112, 112},
        {529445, 137, 136},
        {1048575, 144, 143},
    };
    
    for (uint32_t i = 0; i < sizeof(TestData) / sizeof(number_float_up_down); i++)
    {
        number_float_up_down V = TestData[i];
        uint32_t RoundUp       = ak_stl::allocator_float_helper::Round_Up_F32_Dist(V.Number);
        uint32_t RoundDown     = ak_stl::allocator_float_helper::Round_Down_F32_Dist(V.Number);
        ASSERT_EQ(RoundUp, V.Up);
        ASSERT_EQ(RoundDown, V.Down);
    }
    
    // Denorms, exp=1 and exp=2 + mantissa = 0 are all precise.
    // NOTE: Assuming 8 value (3 bit) mantissa.
    // If this test fails, please change this assumption!
    PreciseNumberCount = 17;
    for (uint32_t Index = 0; Index < PreciseNumberCount; Index++)
    {
        uint32_t V = ak_stl::allocator_float_helper::F32_To_U32(Index);
        ASSERT_EQ(Index, V);
    }
    
    // Test that float->uint->float conversion is precise for all numbers
    // NOTE: Test values < 240. 240->4G = overflows 32 bit integer
    for (uint32_t Index = 0; Index < 240; Index++)
    {
        uint32_t V = ak_stl::allocator_float_helper::F32_To_U32(Index);
        uint32_t RoundUp = ak_stl::allocator_float_helper::Round_Up_F32_Dist(V);
        uint32_t RoundDown = ak_stl::allocator_float_helper::Round_Down_F32_Dist(V);
        
        ASSERT_EQ(Index, RoundUp);
        ASSERT_EQ(Index, RoundDown);
    }
}

UTEST_MAIN();