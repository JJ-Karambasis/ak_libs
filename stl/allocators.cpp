namespace ak_stl
{
    inline uint32_t Find_Num_Leading_Zeros(uint32_t V)
    {
#ifdef _MSC_VER
        uint32_t RetVal;
        _BitScanReverse((unsigned long*)&RetVal, V);
        return 31-RetVal;
#else
#error Not Implemented
#endif
    }
    
    inline uint32_t Find_First_Set_Bit(uint32_t V)
    {
#ifdef _MSC_VER
        uint32_t RetVal;
        _BitScanForward((unsigned long*)&RetVal, V);
        return RetVal;
#else
#error Not Implemented
#endif
    }
    
    uint32_t Find_Lowest_Set_Bit_After(uint32_t BitMask, uint32_t StartBitIndex)
    {
        uint32_t MaskBeforeStartIndex = (1 << StartBitIndex) - 1;
        uint32_t MaskAfterStartIndex = ~MaskBeforeStartIndex;
        uint32_t BitsAfter = BitMask & MaskAfterStartIndex;
        if (BitsAfter == 0) return (uint32_t)-1;
        return Find_First_Set_Bit(BitsAfter);
    }
    
    class allocator_float_helper
    {
        static const uint32_t MANTISSA_BITS  = 3;
        static const uint32_t MANTISSA_VALUE = 1 << MANTISSA_BITS;
        static const uint32_t MANTISSA_MASK  = MANTISSA_VALUE-1;
        
        public:
        static uint32_t Round_Up_F32_Dist(uint32_t Size)
        {
            uint32_t Exp = 0;
            uint32_t Mantissa = 0;
            
            if(Size < MANTISSA_VALUE) Mantissa = Size;
            else
            {
                uint32_t LeadingZeros = Find_Num_Leading_Zeros(Size);
                uint32_t HighestSetBit = 31-LeadingZeros;
                
                uint32_t MantissaStartBit = HighestSetBit - MANTISSA_BITS;
                Exp = MantissaStartBit + 1;
                Mantissa = (Size >> MantissaStartBit) & MANTISSA_MASK;
                
                uint32_t LowBitsMask = (1 << MantissaStartBit) - 1;
                
                //Round to highest index
                if(Size & LowBitsMask) Mantissa++;
            }
            
            return (Exp << MANTISSA_BITS) + Mantissa;
        }
        
        static uint32_t Round_Down_F32_Dist(uint32_t Size)
        {
            uint32_t Exp = 0;
            uint32_t Mantissa = 0;
            
            if(Size < MANTISSA_VALUE) Mantissa = Size;
            else
            {
                uint32_t LeadingZeros = Find_Num_Leading_Zeros(Size);
                uint32_t HighestSetBit = 31-LeadingZeros;
                
                uint32_t MantissaStartBit = HighestSetBit - MANTISSA_BITS;
                Exp = MantissaStartBit + 1;
                Mantissa = (Size >> MantissaStartBit) & MANTISSA_MASK;
            }
            
            return (Exp << MANTISSA_BITS) | Mantissa;
        }
        
        static uint32_t F32_To_U32(uint32_t Float)
        {
            uint32_t Exp = Float >> MANTISSA_BITS;
            uint32_t Mantissa = Float & MANTISSA_MASK;
            return (Exp == 0) ? Mantissa : ((Mantissa | MANTISSA_VALUE) << (Exp - 1));
        }
    };
    
    void* allocator::Allocate(size_t Size, clear_flag Flag)
    {
        void* Result = Allocate_Internal(Size);
        if((Flag == clear_flag::Clear) && Result)
            AK_STL_Memset(Result, 0, Size);
        return Result;
    }
    
    void* allocator::Reallocate(void* Original, size_t NewSize)
    {
        void* Result = Reallocate_Internal(Original, NewSize);
        return Result;
    }
    
    void allocator::Free(void* Memory)
    {
        return Free_Internal(Memory);
    }
    
    void* allocator::Allocate_Aligned(size_t Size, size_t Alignment, clear_flag Flag)
    {
        AK_STL_Assert(Alignment > 0 && Is_Pow2(Alignment));
        
        void*  P1; // original block
        void** P2; // aligned block
        size_t Offset = Alignment - 1 + sizeof(void*);
        if ((P1 = Allocate_Internal(Size + Offset)) == NULL)
            return NULL;
        
        P2= (void**)(((size_t)(P1) + Offset) & ~(Alignment - 1));
        P2[-1] = P1;
        
        if(Flag == clear_flag::Clear) AK_STL_Memset(P2, 0, Size);
        
        return P2;
    }
    
    void* allocator::Reallocate_Aligned(void* Original, size_t NewSize, size_t Alignment)
    {
        AK_STL_Assert(Alignment > 0 && Is_Pow2(Alignment));
        
        void* OriginalUnaligned = ((void**)Original)[-1];
        
        void*  P1; // original block
        void** P2; // aligned block
        size_t Offset = Alignment - 1 + sizeof(void*);
        if ((P1 = Reallocate_Internal(OriginalUnaligned, NewSize + Offset)) == NULL)
            return NULL;
        
        P2= (void**)(((size_t)(P1) + Offset) & ~(Alignment - 1));
        P2[-1] = P1;
        
        return P2;
    }
    
    void allocator::Free_Aligned(void* Memory)
    {
        Free(((void**)Memory)[-1]);
    }
    
    void* default_allocator::Allocate_Internal(size_t Size)
    {
        return AK_STL_Malloc(Size);
    }
    
    void* default_allocator::Reallocate_Internal(void* Original, size_t NewSize)
    {
        return AK_STL_Realloc(Original, NewSize);
    }
    
    void default_allocator::Free_Internal(void* Memory)
    {
        return AK_STL_Free(Memory);
    }
    
    offset_allocation tlsf_offset_allocator::Allocate(size_t _Size)
    {
        AK_STL_Assert(_Size <= 0xFFFFFFFF);
        uint32_t Size = (uint32_t)_Size;
        
        // Round up to bin index to ensure that alloc >= bin
        // Gives us min bin index that fits the size
        uint32_t MinBinIndex = allocator_float_helper::Round_Up_F32_Dist(Size);
        
        uint32_t MinTopBinIndex  = MinBinIndex >> TOP_BINS_INDEX_SHIFT;
        uint32_t MinLeafBinIndex = MinBinIndex &  LEAF_BINS_INDEX_MASK;
        
        uint32_t TopBinIndex  = MinTopBinIndex;
        uint32_t LeafBinIndex = offset_allocation::Invalid;
        
        // If top bin exists, scan its leaf bin. This can fail (NO_SPACE).
        if (m_UsedBinsTop & (1 << TopBinIndex))
            LeafBinIndex = Find_Lowest_Set_Bit_After(m_UsedBins[TopBinIndex], MinLeafBinIndex);
        
        // If we didn't find space in top bin, we search top bin from +1
        if (LeafBinIndex == offset_allocation::Invalid)
        {
            TopBinIndex = Find_Lowest_Set_Bit_After(m_UsedBinsTop, MinTopBinIndex + 1);
            
            // Out of space?
            if (TopBinIndex == offset_allocation::Invalid)
                return offset_allocator::Null;
            
            // All leaf bins here fit the alloc, since the top bin was rounded up. Start leaf search from bit 0.
            // NOTE: This search can't fail since at least one leaf bit was set because the top bit was set.
            LeafBinIndex = Find_First_Set_Bit(m_UsedBins[TopBinIndex]);
        }
        
        uint32_t BinIndex = (TopBinIndex << TOP_BINS_INDEX_SHIFT) | LeafBinIndex;
        
        // Pop the top node of the bin. Bin top = node.next.
        uint32_t NodeIndex = m_BinIndices[BinIndex];
        node* Node = m_Nodes + NodeIndex;
        uint32_t NodeTotalSize = Node->Size;
        Node->Size = Size;
        Node->Used = true;
        m_BinIndices[BinIndex] = Node->BinListNext;
        if (node.binListNext != Node::unused) m_nodes[node.binListNext].binListPrev = Node::unused;
        m_freeStorage -= nodeTotalSize;
#ifdef DEBUG_VERBOSE
        printf("Free storage: %u (-%u) (allocate)\n", m_freeStorage, nodeTotalSize);
#endif
        
        // Bin empty?
        if (m_binIndices[binIndex] == Node::unused)
        {
            // Remove a leaf bin mask bit
            m_usedBins[topBinIndex] &= ~(1 << leafBinIndex);
            
            // All leaf bins empty?
            if (m_usedBins[topBinIndex] == 0)
            {
                // Remove a top bin mask bit
                m_usedBinsTop &= ~(1 << topBinIndex);
            }
        }
        
        // Push back reminder N elements to a lower bin
        uint32 reminderSize = nodeTotalSize - size;
        if (reminderSize > 0)
        {
            uint32 newNodeIndex = insertNodeIntoBin(reminderSize, node.dataOffset + size);
            
            // Link nodes next to each other so that we can merge them later if both are free
            // And update the old next neighbor to point to the new node (in middle)
            if (node.neighborNext != Node::unused) m_nodes[node.neighborNext].neighborPrev = newNodeIndex;
            m_nodes[newNodeIndex].neighborPrev = nodeIndex;
            m_nodes[newNodeIndex].neighborNext = node.neighborNext;
            node.neighborNext = newNodeIndex;
        }
        
        return {.offset = node.dataOffset, .metadata = nodeIndex};
    }
    
    void tlsf_offset_allocator::Free(const offset_allocation& Allocation)
    {
    }
    
    void* tlsf_allocator::Allocate_Internal(size_t Size)
    {
        AK_STL_Assert(Size <= m_BlockSize);
        offset_allocation Allocation = tlsf_offset_allocator::Allocate(Size);
        if(Allocation == offset_allocation::Null) return NULL;
        
        //NOTE(EVERYONE): Since we have fixed size blocks, we cannot allocate things between block boundaries 
        size_t StartBlockIndex = Allocation.Offset % m_BlockSize;
        size_t EndBlockIndex = (StartBlockIndex+Size) % m_BlockSize;
        if(StartBlockIndex != EndBlockIndex)
        {
            //NOTE(EVERYONE): We check to see if we overlap block boundaries here. If we do, we split the boundaries into
            //two free blocks and insert them into the freenode list. 
            AK_STL_Assert((EndBlockIndex-1) == StartBlockIndex);
            
            size_t StartBlockOffset = Allocation.Offset;
            size_t StartBlockSize = ((StartBlockIndex*m_BlockSize)+m_BlockSize) - Allocation.Offset;
            size_t EndBlockOffset = StartBlockOffset+StartBlockSize;
            size_t EndBlockSize = Size-StartBlockSize;
            
            //NOTE(EVERYONE): Change the currently allocated node to the first block
            m_Nodes[Allocation.Metadata].Size = StartBlockSize;
            
            //NOTE(EVERYONE): Free the first block node
            tlsf_offset_allocator::Free(Allocation);
            
            //NOTE(EVERYONE): Insert the remaining size into a new bin
            Insert_Node_Into_Bin(EndBlockOffset, EndBlockSize);
            
            //NOTE(EVERYONE): Allocate new block
            return Allocate_Internal(Size);
        }
        
        if(!m_Blocks[StartBlockIndex]) m_Blocks[StartBlockIndex] = (uint8_t*)Allocate_Block();
        if(!m_Blocks[EndBlockIndex]) m_Blocks[EndBlockIndex] = (uint8_t*)Allocate_Block();
        
        uint8_t* Result = m_Blocks[StartBlockIndex] + Allocation.Offset;
        return Result;
    }
}