#include "ThreadCache.h"

void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
	return nullptr;
}
// �������
void* ThreadCache::Allocate(size_t size)
{
	assert(size <= MAX_BYTES);
	size_t alignSize = SizeClass::RoundUp(size);
	size_t index = SizeClass::Index(size);

	if (!_freeList[index].Empty())
	{
		return _freeList[index].Pop();
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
	_freeList[index].Push(ptr);

}