#pragma once
#include "Common.h"

// ����(����)ģʽ
class CentralCache
{
public:
	static CentralCache* GetInstance()
	{
		return &_sInst;
	}

	//��ȡһ���ǿյ�span
	Span* GetOneSpan(SpanList& list, size_t size);

	// �����Ļ����ȡһ�������Ķ����thread cache
	//start endΪ��ȡ������ʼ�����
	size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size);

	// ��һ�������Ķ����ͷŵ�span���
	void ReleseListToSpans(void* start, size_t size);
private:
	SpanList _spanLists[NFREELIST];
private:
	CentralCache()
	{}

	CentralCache(const CentralCache&) = delete;
	static CentralCache _sInst;
};
