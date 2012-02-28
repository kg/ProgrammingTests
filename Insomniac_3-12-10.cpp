#include "MemoryManager.h"

#include <cassert>
#include <limits.h>
#include <string.h>

namespace MemoryManager
{
  const int MM_POOL_SIZE = 65536;
  char MM_pool[MM_POOL_SIZE];

  // Reducing the page size will increase the space efficiency of allocations
  //  but decrease the performance of allocate()/deallocate() and increase
  //  the amount of space used by the page table
  // Note that given a page table that stores 8 bits per page, the maximum
  //  size of a single allocation is (255 * MM_PAGE_SIZE). For larger allocations,
  //  either increase the page size or the size of each page table entry.
  const unsigned MM_PAGE_SIZE = 128;
  // Computes the maximum number of pages that can fit into the pool
  //  based on the size of each page and the page table
  const unsigned MM_NUM_PAGES = MM_POOL_SIZE / (MM_PAGE_SIZE + 1);

  // Convenience constants for bounds checks
  unsigned char * const MM_firstPagePtr =
    reinterpret_cast<unsigned char *>(MM_pool);
  // For some reason VC++2008 stores this constant in the 
  //  executable's data segment instead of folding it into expressions,
  //  so it's technically using extra memory. :/
  unsigned char * const MM_lastPagePtr =
    MM_firstPagePtr +
    (MM_NUM_PAGES - 1) * MM_PAGE_SIZE;

  // Place page table at the end of the pool
  unsigned char * const MM_pageTable = 
    MM_firstPagePtr +
    (MM_POOL_SIZE - MM_NUM_PAGES);

  // Starts searching for an empty page at the provided page ID.
  // If an empty page is found, the page ID provided is replaced with
  //  the ID of the empty page, and true is returned. Otherwise,
  //  false is returned.
  // VC++2008 appears to inline this function appropriately, so there's
  //  no performance overhead from pulling this logic out into a function.
  inline bool findEmptyPage (unsigned & pageId) {
    while (pageId < MM_NUM_PAGES) {
      unsigned char occupancy = MM_pageTable[pageId];
      
      // If a page is not empty, it contains an occupancy value that
      //  tells us how many pages we need to skip to find the end of that
      //  allocation.
      if (occupancy == 0)
        return true;
      else
        pageId += occupancy;
    }
    
    return false;
  }

  // Converts a page ID to a pointer. No bounds checks
  //  are performed.
  inline void * pageIdToPtr (const unsigned pageId) {
    return MM_firstPagePtr + (pageId * MM_PAGE_SIZE);
  }

  // Converts a pointer to a page ID, if possible. If the provided
  //  pointer is not part of the memory pool, false is returned.
  //  If the provided pointer is part of the memory pool, pageId
  //  is updated with the ID of the page and true is returned.
  inline bool ptrToPageId (const void * ptr, unsigned & pageId) {
    // Do simple bounds checks on the pointer
    if (ptr < MM_firstPagePtr)
      return false;
    else if (ptr > MM_lastPagePtr)
      return false;

    unsigned relativeAddress = 
      reinterpret_cast<unsigned>(ptr) - 
      reinterpret_cast<unsigned>(MM_firstPagePtr);

    // Make sure the pointer is properly page-aligned, since if
    //  it isn't, that means it wasn't returned by allocate()
    if (relativeAddress % MM_PAGE_SIZE != 0)
      return false;

    pageId = relativeAddress / MM_PAGE_SIZE;

    return true;
  }

  // Converts a size in bytes to the number of pages required to hold
  //  that many bytes.
  inline unsigned sizeToPageCount (const unsigned sizeInBytes) {
    // Force the allocation of at least one page so allocate(0) works
    if (sizeInBytes == 0)
      return 1;

    // Round partial page allocations up to an entire page
    return (sizeInBytes + MM_PAGE_SIZE - 1) / MM_PAGE_SIZE;
  }

  // Initialize set up any data needed to manage the memory pool
  void initializeMemoryManager(void)
  {
    // Mark every page as empty
    memset(MM_pageTable, 0, MM_NUM_PAGES);
  }

  // return a pointer inside the memory pool
  // If no chunk can accommodate aSize call onOutOfMemory()
  void* allocate(int aSize)
  {
    // If we wanted to detect buffer underruns or overruns, we could
    //  allocate an extra page on both sides of our allocation and fill
    //  both of them with a fill pattern.
    unsigned pageCount = sizeToPageCount(aSize);
    unsigned pageId = 0;

    // Given the use of unsigned char for the page table, a small page size
    //  may prevent the allocation of the entire pool in a single block.
    if (pageCount > 255) {
      onIllegalOperation("Requested allocation (%d bytes -> %d pages) too large based on page size of %d", aSize, pageCount, MM_PAGE_SIZE);
      return (void*)0;
    }
    
    // If we expected frequent temporary allocations we could make use of a simple
    //  free-list structure here to avoid having to do a page table search to
    //  satisfy allocations. This is unnecessary complication in the general case,
    //  though, since performance sensitive algorithms usually shouldn't use malloc
    //  anyway, and as a result it often isn't a bottleneck.
    
    // If tiny allocations are frequent, instead of using a single page table we would
    //  probably want to maintain multiple page tables with smaller or larger page
    //  sizes, to reduce overhead and fragmentation.
    
    // If we also wanted thread safety we might want to keep a free-list for each
    //  thread in thread-local storage so that threads would be able to allocate
    //  without needing to compete for the lock and scan the page table every time.
    
    // The TCMalloc webpage describes some of these optimization techniques in
    //  detail: http://goog-perftools.sourceforge.net/doc/tcmalloc.html
    
    // Scan for a set of sequential unoccupied pages that can accomodate our allocation.
    //  The layout of the page table ensures that the worst case performance of this scan
    //  is O(pages) and in the ideal case it can skip past most of the page table.
    while (findEmptyPage(pageId)) {
      // We start scanning backwards from the last page needed to satisfy this allocation,
      //  already knowing that the first page (pageId) is unoccupied.
      unsigned lastPageId = pageId + pageCount - 1;
      
      bool obstructed = false;
      while (lastPageId > pageId) {
        // As soon as we find an occupied page, we know that this candidate location
        //  [pageId, lastPageId] is not going to satisfy our allocation, so we can 
        //  bail out and set lastPageId as the start location for our next call to
        //  findEmptyPage, skipping past the entire allocation that currently
        //  occupies this page.
        if (MM_pageTable[lastPageId] != 0) {
          pageId = lastPageId;
          obstructed = true;
          break;
        } else {
          lastPageId--;
        }
      }
      
      // We found an occupied page during our search, so skip back up to the top
      //  of the loop and find another empty page to start searching from
      if (obstructed)
        continue;
      
      // Mark all the pages as occupied with the correct occupancy value. The
      //  occupancy values allow allocate() to skip past an entire allocation
      //  in one operation when searching for empty pages, so that it won't
      //  have to scan the entire page table.

      // If we wanted to provide thread safety, we'd need to acquire a lock
      //  here and do a sanity check to make sure that none of these pages
      //  became occupied before we acquired the lock. If they got occupied,
      //  we'd need to bail out and restart our search (or continue from the
      //  current location), since we clearly just lost a race. After marking
      //  all the pages as occupied we could release the lock.
      unsigned i = pageId;
      for (unsigned char occupancy = pageCount; occupancy > 0; occupancy--, i++)
        MM_pageTable[i] = occupancy;
      
      // To aid debugging, we might want to fill the allocated pages with a
      //  fill pattern to detect use of uninitialized memory.
      
      return pageIdToPtr(pageId);
    }

    // We never found enough sequential unoccupied pages to hold this allocation
    onOutOfMemory();
    return (void*)0;
  }

  // Free up a chunk previously allocated
  void deallocate(void* aPointer)
  {
    // Figure out whether this points to an allocated page (note that
    //  if we are passed a pointer to the middle of an allocation, the
    //  occupancy table data structure will result in us deallocating
    //  pages starting from the pointer to the end of the allocation.
    unsigned pageId;
    if (!ptrToPageId(aPointer, pageId)) {
      onIllegalOperation("Invalid pointer passed to deallocate().");
      return;
    }

    // Make sure the page we're being asked to free isn't already empty
    //  since that means someone screwed up.
    unsigned occupancy = MM_pageTable[pageId];
    if (occupancy == 0) {
      onIllegalOperation("deallocate() was passed a pointer to an already-freed page.");
      return;
    }

    // The occupancy value from the first page tells us how many pages to mark as empty
    memset(MM_pageTable + pageId, 0, occupancy);
    
    // We could fill the pages with a fill pattern here to aid debugging.
    
    // If using a free-list to optimize temporary allocations we'd add the returned
    //  allocation to the list here.
  }

  // Will scan the memory pool and return the total free space remaining
  int freeRemaining(void)
  {
    int resultPages = 0;
    unsigned pageId = 0;

    // Just scan through and count empty pages. Worst-case complexity
    //  should be O(pages) and in the best case we skip over occupied
    //  pages cheaply, so this actually gets faster as you allocate more.
    while (findEmptyPage(pageId)) {
      while (MM_pageTable[pageId] == 0) {
        resultPages++;
        pageId++;
      }
    }

    return resultPages * MM_PAGE_SIZE;
  }

  // Will scan the memory pool and return the largest free space remaining
  int largestFree(void)
  {
    int resultPages = 0;
    unsigned pageId = 0;

    // Second verse, same as the first. Like the above function, but we 
    //  count sequential empty pages instead of all empty pages.
    while (findEmptyPage(pageId)) {
      int currentPages = 0;
      while (MM_pageTable[pageId] == 0) {
        currentPages++;
        pageId++;
      }

      // Only store the largest number of sequential empty pages.
      if (currentPages > resultPages)
        resultPages = currentPages;
    }

    return resultPages * MM_PAGE_SIZE;
  }

  // will scan the memory pool and return the smallest free space remaining
  int smallestFree(void)
  {
    // Yuck :(
    int resultPages = INT_MAX;
    unsigned pageId = 0;

    // Same as above.
    while (findEmptyPage(pageId)) {
      int currentPages = 0;
      while (MM_pageTable[pageId] == 0) {
        currentPages++;
        pageId++;
      }

      // Only store the smallest number of sequential empty pages.
      if (currentPages < resultPages)
        resultPages = currentPages;
    }

    // Simple hack to return correct result when no pages are free
    if (resultPages == INT_MAX)
      return 0;
    else
      return resultPages * MM_PAGE_SIZE;
  }

  // Added these for my testing purposes

  unsigned getPageSize () {
    return MM_PAGE_SIZE;
  }

  unsigned getNumPages () {
    return MM_NUM_PAGES;
  }

  unsigned getMaxAllocationSize () {
    unsigned result = MM_PAGE_SIZE * 255;
    if (sizeToPageCount(result) > MM_NUM_PAGES)
      result = MM_NUM_PAGES * MM_PAGE_SIZE;

    return result;
  }
}