#ifndef ALLOCATORS_H
#define ALLOCATORS_H

namespace ak_stl
{
    enum class clear_flag
    {
        Clear,
        NoClear
    };
    
    class allocator
    {
        public:
        void* Allocate(size_t Size, clear_flag Flag = AK_STL_DEFAULT_CLEAR_FLAG);
        void* Reallocate(void* Original, size_t NewSize);
        void  Free(void* Memory);
        
        void* Allocate_Aligned(size_t Size, size_t Alignment, clear_flag Flag = AK_STL_DEFAULT_CLEAR_FLAG);
        void* Reallocate_Aligned(void* Original, size_t NewSize, size_t Alignment);
        void  Free_Aligned(void* Memory);
        
        protected:
        virtual void* Allocate_Internal(size_t Size) = 0;
        virtual void* Reallocate_Internal(void* Original, size_t NewSize) = 0;
        virtual void  Free_Internal(void* Memory)    = 0;
    };
    
    class async_allocator : allocator
    {
        protected:
        virtual void* Allocate_Internal(size_t Size) = 0;
        virtual void* Reallocate_Internal(void* Original, size_t NewSize) = 0;
        virtual void  Free_Internal(void* Memory)    = 0;
    };
    
    class default_allocator : public allocator
    {
        protected:
        void* Allocate_Internal(size_t Size) override;
        void* Reallocate_Internal(void* Original, size_t NewSize) override;
        void  Free_Internal(void* Memory) override;
    };
    
    template <typename type, class lock = mutex> class async_allocator_wrapper : public async_allocator
    {
        type  m_Allocator;
        lock  m_Lock;
        
        protected:
        virtual void* Allocate_Internal(size_t Size) = 0;
        virtual void* Reallocate_Internal(void* Original, size_t NewSize) = 0;
        virtual void  Free_Internal(void* Memory)    = 0;
    };
    
    struct offset_allocation
    {
        size_t Offset;
        size_t Metadata;
        
        static const size_t Invalid = (size_t)-1;
    };
    
    class offset_allocator
    {
        public:
        virtual offset_allocation Allocate(size_t Size) = 0;
        virtual void Free(const offset_allocation& Allocation) = 0;
        
        offset_allocation Allocate_Aligned(size_t Size, size_t Alignment);
        void Free_Aligned(const offset_allocation& Allocation);
        
        static constexpr offset_allocation Null = {offset_allocation::Invalid, offset_allocation::Invalid};
    };
    
    class tlsf_offset_allocator : public offset_allocator
    {
        protected:
        static const uint32_t NUM_TOP_BINS  = 32;
        static const uint32_t BINS_PER_LEAF = 8;
        static const uint32_t TOP_BINS_INDEX_SHIFT = 3;
        static const uint32_t LEAF_BINS_INDEX_MASK = 0x7;
        static const uint32_t NUM_LEAF_BINS = NUM_TOP_BINS*BINS_PER_LEAF;
        
        struct node
        {
            size_t   Offset = 0;
            size_t   Size = 0;
            uint32_t BinListPrev = offset_allocation::Invalid;
            uint32_t BinListNext = offset_allocation::Invalid;
            uint32_t NeighborPrev = offset_allocation::Invalid;
            uint32_t NeighborNext = offset_allocation::Invalid;
            bool   Used;
        };
        
        uint32_t m_UsedBinsTop;
        uint8_t  m_UsedBins[NUM_TOP_BINS];
        uint32_t m_BinIndices[NUM_LEAF_BINS];
        
        node* m_Nodes;
        
        public:
        offset_allocation Allocate(size_t Size) override;
        void Free(const offset_allocation& Allocation) override;
        
        protected:
        uint32_t Insert_Node_Into_Bin(size_t Offset, size_t Size);
    };
    
    class tlsf_allocator : public allocator, tlsf_offset_allocator
    {
        size_t    m_BlockSize;
        uint8_t** m_Blocks;
        
        protected:
        void* Allocate_Internal(size_t Size) override;
        void* Reallocate_Internal(void* Original, size_t Size) override;
        void  Free_Internal(void* Memory) override;
        
        void* Allocate_Block();
    };
}

#endif
