#include "QueueManager.h"

namespace QueueManager
{
  typedef unsigned ChunkHandle;

  const unsigned CHUNK_SIZE       = MAX_DATA_SIZE / MAX_QUEUES;
  const unsigned MAX_CHUNKS       = MAX_DATA_SIZE / CHUNK_SIZE;
  const unsigned MAX_WASTED_BYTES = CHUNK_SIZE;
  
  const ChunkHandle LAST_CHUNK_HANDLE = 0xFFFFFFFE;
  
  const unsigned    NULL_HANDLE       = 0xFFFFFFFF;
  const ChunkHandle NULL_CHUNK_HANDLE = NULL_HANDLE;
  const QueueHandle NULL_QUEUE_HANDLE = NULL_HANDLE & 0xFF;
  
  struct Chunk;
  struct ChunkPtr;
  struct Queue;
  
  struct Chunk {
    ChunkHandle   chunk_next;
    unsigned char data[CHUNK_SIZE];
    
    Chunk() :
      chunk_next(NULL_CHUNK_HANDLE)
    {
    }
    
    inline Chunk * Next();
  };
  
  struct ChunkPtr {
    ChunkHandle chunk;
    int         offset; // this needs to be signed! see createQueue().
    
    ChunkPtr() :
      chunk(NULL_CHUNK_HANDLE),
      offset(0)
    {
    }
    
    ChunkPtr(ChunkHandle _chunk, unsigned _offset) :
      chunk(_chunk),
      offset(_offset)
    {
    }
    
    inline unsigned char * Ptr();
    inline void Add(unsigned value);
  };
  
  struct Queue {
    ChunkHandle chunk_first;
    ChunkPtr    ptr_read;
    ChunkPtr    ptr_write;
    unsigned    size;
    unsigned    bytes_free;
    unsigned    bytes_wasted;
    
    Queue() :
      chunk_first(NULL_CHUNK_HANDLE),
      ptr_read(),
      ptr_write(),
      size(0),
      bytes_free(0),
      bytes_wasted(0)
    {
    }
    
    inline ChunkHandle   GetChunk();
    inline void          Grow(ChunkHandle after);
    inline void          Shrink();
    inline Chunk *       Head();
    inline unsigned char Pop();
    inline void          Push(unsigned char value);
  };

  // Queue table
  static Queue       s_queues[MAX_QUEUES];
  // Index of an unused queue for quick creation
  static QueueHandle s_unusedQueue;
  
  // Storage chunk table
  static Chunk       s_chunks[MAX_CHUNKS];
  // Index of an unused chunk for quick creation
  static ChunkHandle s_unusedChunk;
  
  inline void _assert(bool expression, const char * expressionStr, const char * file, unsigned line) {
    if (!expression) {
      __asm int 3;
      onIllegalOperation("Assertion failed: %s (%s:%d)", expressionStr, file, line);
    }
  }
  
  #ifdef _DEBUG
    #define assert(expr) \
      _assert(!!(expr), #expr, __FILE__, __LINE__)
  #else
    #define assert(expr)
  #endif    

  void initializeQueueManager()
  {
    // Null out all the queues
    for (unsigned i = 0; i < MAX_QUEUES; i++) {
      s_queues[i] = Queue();
    }
    
    // Null out all the chunks
    for (unsigned i = 0; i < MAX_CHUNKS; i++) {
      s_chunks[i] = Chunk();
    }
    
    s_unusedQueue = 0;
    s_unusedChunk = 0;
  }

  QueueHandle createQueue()
  {
    // Attempt to grab the unused queue
    QueueHandle result = s_unusedQueue;
    s_unusedQueue = NULL_QUEUE_HANDLE;
    
    // If there was no unused queue available, do a linear search for one
    if (result == NULL_QUEUE_HANDLE) {
      for (unsigned i = 0; i < MAX_QUEUES; i++) {
        if (s_queues[i].Head() == 0) {
          result = i;
          break;
        }
      }
    }
    
    // Out of available queues
    if (result == NULL_QUEUE_HANDLE)
      onOutOfMemory();
      
    // Speed up the next allocation if the following queue is free
    if (result < MAX_QUEUES - 1) {
      if (s_queues[result + 1].Head() == 0)
        s_unusedQueue = result + 1;
    }
      
    // Initialize the queue
    Queue & queue = s_queues[result];
    queue.chunk_first = queue.GetChunk();
    queue.bytes_free = CHUNK_SIZE;
    queue.ptr_read = ChunkPtr(queue.chunk_first, 0);
    // We initially place the write pointer before the beginning of the queue
    //  so that the 'add before write' behavior works correctly.
    queue.ptr_write = ChunkPtr(queue.chunk_first, -1);
    s_chunks[queue.chunk_first].chunk_next = LAST_CHUNK_HANDLE;
    
    return result;
  }

  void destroyQueue (QueueHandle queueHandle)
  {
    assert(queueHandle < MAX_QUEUES);
    Queue & queue = s_queues[queueHandle];
    Chunk * chunk = queue.Head();
    assert(chunk);
    
    // Clean up the linked list
    while (chunk) {
      Chunk * current = chunk;
      chunk = current->Next();
      current->chunk_next = NULL_CHUNK_HANDLE;
    }
    
    // Null out the queue
    queue = Queue();
    s_unusedQueue = queueHandle;
  }

  void enQueue (QueueHandle queueHandle, unsigned char value)
  {
    assert(queueHandle < MAX_QUEUES);
    Queue & queue = s_queues[queueHandle];
    
    queue.Push(value);
  }

  unsigned char deQueue (QueueHandle queueHandle)
  {
    assert(queueHandle < MAX_QUEUES);
    Queue & queue = s_queues[queueHandle];
    
    return queue.Pop();
  }
  
  inline unsigned char * ChunkPtr::Ptr() {
    assert(chunk != NULL_CHUNK_HANDLE);
    
    while (offset >= CHUNK_SIZE) {
      Chunk & _chunk = s_chunks[chunk];
      offset -= CHUNK_SIZE;
      chunk = _chunk.chunk_next;
      assert(chunk != NULL_CHUNK_HANDLE);
      assert(chunk != LAST_CHUNK_HANDLE);
    }
    
    return (s_chunks[chunk].data + offset);
  }

  inline void ChunkPtr::Add(unsigned value) {
    assert(chunk != NULL_CHUNK_HANDLE);
    
    offset += value;
  }
  
  inline Chunk * Chunk::Next() {
    if ((chunk_next == NULL_CHUNK_HANDLE) || (chunk_next == LAST_CHUNK_HANDLE))
      return 0;
    else
      return &(s_chunks[chunk_next]);
  }

  // Note: this function doesn't initialize the chunk for you  
  inline ChunkHandle Queue::GetChunk() {
    ChunkHandle result = s_unusedChunk;
    s_unusedChunk = NULL_CHUNK_HANDLE;
    
    // If there was no unused chunk available, do a linear search for one
    if (result == NULL_CHUNK_HANDLE) {
      for (unsigned i = 0; i < MAX_CHUNKS; i++) {
        if (s_chunks[i].chunk_next == NULL_CHUNK_HANDLE) {
          result = i;
          break;
        }
      }
    }
    
    // Out of available chunks
    if (result == NULL_CHUNK_HANDLE)
      onOutOfMemory();
      
    // Speed up the next allocation if the following chunk is free
    if (result < MAX_CHUNKS - 1) {
      if (s_chunks[result + 1].chunk_next == NULL_CHUNK_HANDLE)
        s_unusedChunk = result + 1;
    }
    
    return result;
  }
  
  inline Chunk * Queue::Head() {
    if (chunk_first == NULL_CHUNK_HANDLE)
      return 0;
    else
      return &(s_chunks[chunk_first]);
  }
  
  inline unsigned char Queue::Pop() {
    assert(size > 0);
    
    unsigned char result = *(ptr_read.Ptr());
    ptr_read.Add(1);
    bytes_wasted += 1;
    size -= 1;
    
    if (bytes_wasted >= MAX_WASTED_BYTES)
      Shrink();
    
    return result;
  }
  
  inline void Queue::Push(unsigned char value) {
    if (bytes_free == 0)
      Grow(ptr_write.chunk);
      
    assert(bytes_free != 0);
    
    ptr_write.Add(1);
    *(ptr_write.Ptr()) = value;
    bytes_free -= 1;
    size += 1;
  }

  inline void Queue::Grow(ChunkHandle after) {
    assert(after != NULL_CHUNK_HANDLE);
    
    if (bytes_wasted >= MAX_WASTED_BYTES)
      Shrink();

    ChunkHandle newHandle = GetChunk();
    Chunk & chunk = s_chunks[after];
    Chunk & newChunk = s_chunks[newHandle];
    
    newChunk.chunk_next = chunk.chunk_next;
    chunk.chunk_next = newHandle;
    bytes_free += CHUNK_SIZE;
  }
  
  // Note! This pulls the first chunk out of the queue. This means that until
  //  the first chunk is completely empty, the queue will never shrink, only
  //  grow.
  inline void Queue::Shrink() {
    assert(bytes_wasted >= CHUNK_SIZE);
        
    Chunk * head = Head();
    // Queues never shrink below 1 chunk
    if (head->chunk_next == LAST_CHUNK_HANDLE) {
      return;
    }

    // We have to resolve the read pointer before removing chunks, because
    //  it could be hanging off the end of the one we're removing
    ChunkPtr old = ptr_read;
    ptr_read.Ptr();
    assert(ptr_read.chunk != chunk_first);
      
    s_unusedChunk = chunk_first;
    chunk_first = head->chunk_next;
    head->chunk_next = NULL_CHUNK_HANDLE;
      
    bytes_wasted -= CHUNK_SIZE;
  }
};
