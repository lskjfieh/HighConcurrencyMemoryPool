#include "ThreadCache.h"
#include "CentralCache.h"

void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
	// ����ʼ���������㷨
	// 1. �ʼ����һ����central cacheһ����������̫�࣬��ֹ̫���ò���
	// 2. ��������е�ǰsize��С����batchNum�ͻ᲻��������ֱ������
	// 3. sizeԽ��һ����central cacheҪ��batchNumԽС
	// 3. sizeԽС��һ����central cacheҪ��batchNumԽ��
	size_t batchNum = min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(size));
	
	if (_freeLists[index].MaxSize() == batchNum) 
	{
		_freeLists[index].MaxSize() += 1;
	}
	
	void* start = nullptr;
	void* end = nullptr;
	
	// ��ȡ����������һ��������������
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
// �������
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
// �ͷŶ���
void ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(ptr);
	assert(size <= MAX_BYTES);

	// �ҳ����ͷŶ���Ķ�Ӧ������Ͱ, �����ȥ
	size_t index = SizeClass::Index(size);
	_freeLists[index].Push(ptr);

	// �������ȴ���һ������������ڴ�ʱ�Ϳ�ʼ��һ��list��central cache

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