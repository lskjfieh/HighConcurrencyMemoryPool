#pragma once
#include "Common.h"

// 单例(饿汉)模式
class CentralCache
{
public:
	static CentralCache* GetInstance()
	{
		return &_sInst;
	}

	//获取一个非空的span
	Span* GetOneSpan(SpanList& list, size_t size);

	// 从中心缓存获取一定数量的对象给thread cache
	//start end为获取到的起始与结束
	size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size);

	// 将一定数量的对象释放到span跨度
	void ReleseListToSpans(void* start, size_t size);
private:
	SpanList _spanLists[NFREELIST];
private:
	CentralCache()
	{}

	CentralCache(const CentralCache&) = delete;
	static CentralCache _sInst;
};
