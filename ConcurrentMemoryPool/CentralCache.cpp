#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_sInst;

//获取一个非空的span
Span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{
	 
	// 查看当前的spanlist中是否有未分配对象的span
	Span* it = list.Begin();
	while (it != list.End())
	{
		if (it->_freeList != nullptr)
		{
			return it;
		}
		else {
			it = it->_next;
		}
	}
	// 先解掉central cache的桶锁,防止其他线程释放内存对象回来时阻塞
	list._mtx.unlock();

	// 走到这里，说明没有空闲span，只能找page cache申请
	PageCache::GetInstance()->_pageMtx.lock();
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
	span->_isUse = true;
	span->_objSize = size;
	PageCache::GetInstance()->_pageMtx.unlock();

	// 计算span的大块内存的起始地址和大块内存的大小(字节数)
	char* start = (char*)(span->_pageId << PAGE_SHIFT);
	size_t bytes = span->_n << PAGE_SHIFT;
	char* end = start + bytes;  // char为一字节此处可以进行运算

	// 把大块内存切自由链表连接起来
	// 1. 先切一块下来，方便尾插
	span->_freeList = start;
	start += size;
	void* tail = span->_freeList;
	int i = 1;
	while (start < end)
	{
		++i;
		NextObj(tail) = start;
		tail = NextObj(tail);
		start += size;
	}
	NextObj(tail) = nullptr;

	// 当span切分好,需要把span挂到桶里时,需要加锁
	list._mtx.lock();
	list.PushFront(span);
	return span;
}

// 从中心缓存获取一定数量的对象给thread cache
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size)
{
	size_t index = SizeClass::Index(size);
	_spanLists[index]._mtx.lock();

	Span* span = GetOneSpan(_spanLists[index], size);
	assert(span);
	assert(span->_freeList);

	// 从span中获取batchNum个对象
	// 如果不够batchNum个，有多少拿多少
	start = span->_freeList;
	end = start;
	size_t i = 0;
	size_t actualNum = 1;
	while ( i < batchNum - 1 && NextObj(end) != nullptr) 
	{
		end = NextObj(end);
		++i;
		++actualNum;
	}
	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;
	span->_useCount += actualNum;

	_spanLists[index]._mtx.unlock();

	return actualNum;
}

// CentralCache回收
void CentralCache::ReleseListToSpans(void* start, size_t size)
{
	// 找到对应桶
	size_t index = SizeClass::Index(size);
	_spanLists[index]._mtx.lock();
	while (start)
	{
		void* next = NextObj(start);
		Span* span = PageCache::GetInstance()->MapObjectToSpan(start);
		NextObj(start) = span->_freeList;
		span->_freeList = start;
		span->_useCount--;
		// 说明span切分出去的所有小块内存都回来了
		// 这个span可以再回收给page cache, page cache进行前后页的合并
		if (span->_useCount == 0)
		{
			_spanLists[index].Erase(span); // 从桶里拿掉该span
			span->_freeList = nullptr;
			span->_next = nullptr;
			span->_prev = nullptr;
			
			_spanLists[index]._mtx.unlock();

			// 释放span给pagecache时，使用pagecache的锁
			// 解掉桶锁
			PageCache::GetInstance()->_pageMtx.lock();
			PageCache::GetInstance()->ReleaseSpanToPageCache(span);
			PageCache::GetInstance()->_pageMtx.unlock();

			_spanLists[index]._mtx.lock();

		}

		start = next;
	}// start为空，则表示都还走了
	
	_spanLists[index]._mtx.unlock();
}
