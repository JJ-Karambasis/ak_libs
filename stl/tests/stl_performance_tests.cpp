#include <stl.h>
#include <stl.cpp>

#include <stdlib.h>

#define KB(x) 1024*x
#define MB(x) 1024*KB(x)

inline size_t Random_Between(size_t Min, size_t Max)
{
    return rand()%(Max-Min+1)+Min;
}

#define System_Clock() __rdtsc()

#ifdef _WIN32
#include <Windows.h>
typedef uint64_t qpc;

qpc Query_Performance_Counter()
{
    qpc Result = {};
    QueryPerformanceCounter((LARGE_INTEGER*)&Result);
    return Result;
}

double To_Seconds(qpc QPC)
{
    LARGE_INTEGER Frequency;
    QueryPerformanceFrequency(&Frequency);
    return (double)QPC/(double)Frequency.QuadPart;
}

#else
#error Not Implemented
#endif

struct allocation_record 
{
    uint64_t ElapsedClock;
    qpc      ElapsedQPC;
};

#define RECORD_ALLOCATE(allocator, block, size) \
allocation_record AllocationRecord = {}; \
uint64_t StartClock = System_Clock(); \
qpc StartQPC        = Query_Performance_Counter(); \
/*block = (allocator)->Allocate(size); */ \
block = HeapAlloc(GetProcessHeap(), 0, size); \
AllocationRecord.ElapsedClock = System_Clock()-StartClock; \
AllocationRecord.ElapsedQPC = Query_Performance_Counter()-StartQPC; \
Allocations.push_back(AllocationRecord);

#define RECORD_FREE(allocator, block) \
allocation_record AllocationRecord = {}; \
uint64_t StartClock = System_Clock(); \
qpc StartQPC = Query_Performance_Counter(); \
/*(allocator)->Free(block); */\
HeapFree(GetProcessHeap(), 0, block); \
AllocationRecord.ElapsedClock = System_Clock()-StartClock; \
AllocationRecord.ElapsedQPC = Query_Performance_Counter()-StartQPC; \
Frees.push_back(AllocationRecord)

#include <vector>

struct allocator_entry
{
    ak_stl::allocator* Allocator;
    const char*        Name;
};

int main()
{
    ak_stl::default_allocator MallocAllocator;
    ak_stl::default_allocator MallocAllocator2;
    
    allocator_entry Allocators[] = 
    {
        {&MallocAllocator, "Malloc 0"},
        {&MallocAllocator2, "Malloc 2"}
    };
    
    for(allocator_entry& Allocator : Allocators)
    {
        std::vector<allocation_record> Allocations;
        std::vector<allocation_record> Frees;
        
        uint32_t Iterations = 1;
        for(uint32_t Index = 0; Index < Iterations; Index++)
        {
            size_t RandomSizes[250];
            for(uint32_t BlockIndex = 0; BlockIndex < 250; BlockIndex++)
            {
                RandomSizes[BlockIndex] = Random_Between(KB(1), MB(1));
            }
            
            void* FirstBlocks[250];
            for(uint32_t BlockIndex = 0; BlockIndex < 250; BlockIndex++)
            {
                RECORD_ALLOCATE(Allocator.Allocator, FirstBlocks[BlockIndex], RandomSizes[BlockIndex]);
            }
            
            for(uint32_t BlockIndex = 0; BlockIndex < 50; BlockIndex++)
            {
                uint32_t ActualBlockIndex = 100+BlockIndex;
                RECORD_FREE(Allocator.Allocator, FirstBlocks[ActualBlockIndex]);
            }
            
            void* SecondBlocks[500];
            for(uint32_t BlockIndex = 0; BlockIndex < 500; BlockIndex++)
            {
                RECORD_ALLOCATE(Allocator.Allocator, SecondBlocks[BlockIndex], RandomSizes[BlockIndex]);
            }
            
            for(uint32_t BlockIndex = 0; BlockIndex < 100; BlockIndex++)
            {
                uint32_t ActualBlockIndex = 150+BlockIndex;
                RECORD_FREE(Allocator.Allocator, SecondBlocks[ActualBlockIndex]);
            }
            
            for(uint32_t BlockIndex = 0; BlockIndex < 100; BlockIndex++)
            {
                RECORD_FREE(Allocator.Allocator, FirstBlocks[BlockIndex]);
            }
            
            for(uint32_t BlockIndex = 150; BlockIndex < 250; BlockIndex++)
            {
                RECORD_FREE(Allocator.Allocator, FirstBlocks[BlockIndex]);
            }
            
            for(uint32_t BlockIndex = 0; BlockIndex < 150; BlockIndex++)
            {
                RECORD_FREE(Allocator.Allocator, SecondBlocks[BlockIndex]);
            }
            
            for(uint32_t BlockIndex = 250; BlockIndex < 500; BlockIndex++)
            {
                RECORD_FREE(Allocator.Allocator, SecondBlocks[BlockIndex]);
            }
            
            printf("\r%s %d/%d", Allocator.Name, Index+1, Iterations);
        }
        printf("\n");
        
        uint64_t TotalClock = 0;
        double   TotalSeconds = 0.0;
        
        AK_STL_Assert(Allocations.size() == Frees.size());
        
        for(const allocation_record& Record : Allocations)
        {
            TotalClock += Record.ElapsedClock;
            TotalSeconds += To_Seconds(Record.ElapsedQPC);
        }
        
        printf("Allocation Total cycles:         %lluhz\n", TotalClock);
        printf("Allocation Average cycles:       %lluhz\n", TotalClock/Allocations.size());
        printf("Allocation Total milliseconds:   %fms\n", TotalSeconds*1000.0);
        printf("Allocation Average milliseconds: %fms\n", (TotalSeconds*1000.0)/Allocations.size());
        
        TotalClock = 0;
        TotalSeconds = 0.0;
        for(const allocation_record& Record : Frees)
        {
            TotalClock += Record.ElapsedClock;
            TotalSeconds += To_Seconds(Record.ElapsedQPC);
        }
        printf("Free Total cycles:               %lluhz\n", TotalClock);
        printf("Free Average cycles:             %lluhz\n", TotalClock/Frees.size());
        printf("Free Total milliseconds:         %fms\n", TotalSeconds*1000.0);
        printf("Free Average milliseconds:       %fms\n", (TotalSeconds*1000.0)/Frees.size());
        printf("\n");
    }
    
    return 0;
}