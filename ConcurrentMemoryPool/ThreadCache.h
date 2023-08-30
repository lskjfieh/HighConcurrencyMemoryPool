#pragma once
#include "Common.h"

class ThreadCache
{
public:
	// ������ͷŶ���
	void* Allocate(size_t size);
	void Deallocate(void* ptr, size_t size);

	// �����Ļ����ȡ����
	void* FetchFromCentralCache(size_t index, size_t size);

	// �ͷŶ���ʱ���������ʱ�������ڴ�ص����Ļ���
	void ListTooLong(FreeList& lsit, size_t size);
private:
	FreeList _freeLists[NFREELIST];
};

static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;