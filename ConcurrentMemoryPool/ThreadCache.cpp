#include "ThreadCache.h"
#include "CentralCache.h"

void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
	// 慢开始反馈调节算法
	// 1. 最开始不会一次向central cache一次批量申请太多，防止太多用不完
	// 2. 如果不断有当前size大小需求，batchNum就会不断增长，直到上限
	// 3. size越大，一次向central cache要的batchNum越小
	// 3. size越小，一次向central cache要的batchNum越大
	size_t batchNum = min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(size));
	
	if (_freeLists[index].MaxSize() == batchNum) 
	{
		_freeLists[index].MaxSize() += 1;
	}
	
	void* start = nullptr;
	void* end = nullptr;
	
	// 获取到的数量不一定等于期望数量
	size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, size);
	assert(actualNum > 0);

	if (actualNum == 1) {
		assert(start == end);
		return start;
	}
	else
	{
		_freeLists[index].PushRange(NextObj(start), end, actualNum-1);
		return start;
	}
}
// 申请对象
void* ThreadCache::Allocate(size_t size)
{
	assert(size <= MAX_BYTES);
	size_t alignSize = SizeClass::RoundUp(size);
	size_t index = SizeClass::Index(size);

	if (!_freeLists[index].Empty())
	{
		return _freeLists[index].Pop();
	}
	else
	{
		return FetchFromCentralCache(index, alignSize);
	}
}
// 释放对象
void ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(ptr);
	assert(size <= MAX_BYTES);

	// 找出待释放对象的对应的所在桶, 插入进去
	size_t index = SizeClass::Index(size);
	_freeLists[index].Push(ptr);

	// 当链表长度大于一次批量申请的内存时就开始还一段list给central cache

	if (_freeLists[index].Size() >= _freeLists[index].MaxSize())
	{
		ListTooLong(_freeLists[index], size);
	}
}
void ThreadCache::ListTooLong(FreeList& list, size_t size)
{
	void* start = nullptr;
	void* end = nullptr;
	list.PopRange(start, end, list.MaxSize());

	CentralCache::GetInstance()->ReleseListToSpans(start, size);
}