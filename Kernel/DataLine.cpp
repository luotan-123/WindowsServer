#pragma once

#include "pch.h"
#include "DataLine.h"
#include "Function.h"
#include "KernelDefine.h"
#include "log.h"

CDataLine::CDataLine()
{
}

CDataLine::~CDataLine()
{
	ListItemData *pListItem = NULL;
	while (m_DataList.size() > 0)
	{
		pListItem = m_DataList.front();
		m_DataList.pop_front();
		delete pListItem->pData;
		delete pListItem;
	}
}

//加入消息队列
/*
Function		:AddData
Memo			:将数据压入到队列当中
Author			:Fred Huang
Add Data		:2008-3-4
Modify Data		:none
Parameter		:
[IN]		pDataInfo	:要压入队列的数据指针
[IN]		uAddSize	:数据大小
[IN]		uDataKind	:数据类型
[IN]		pAppendData	:附加数据，可能是空的
[IN]		pAppendAddSize	:附加数据大小，可以为0，此时实体数据为空
Return			:指压入队列的大小
*/
UINT CDataLine::AddData(DataLineHead * pDataInfo, UINT uAddSize, UINT uDataKind, void * pAppendData, UINT uAppendAddSize)
{
	if (!m_hCompletionPort)
	{
		return 0;
	}

	if (!pDataInfo)
	{
		return 0;
	}

	CSignedLockObject LockObject(&m_csLock, true);

	ListItemData *pListItem = new ListItemData;			//创建一个队列项

	pListItem->pData = NULL;							//先设为0，以保证后续不出错
	pListItem->stDataHead.uSize = uAddSize;				//数据大小
	pListItem->stDataHead.uDataKind = uDataKind;		//数据类型
	if (pAppendData)									//如果有附加数据
	{
		pListItem->stDataHead.uSize += uAppendAddSize;
	}

	pListItem->pData = new BYTE[pListItem->stDataHead.uSize + 1];	//申请数据项内存
	ZeroMemory(pListItem->pData, pListItem->stDataHead.uSize + 1);	//清空内存

	pDataInfo->uDataKind = uDataKind;
	pDataInfo->uSize = pListItem->stDataHead.uSize;

	memcpy(pListItem->pData, pDataInfo, uAddSize);					//复制实体数据
	if (pAppendData != NULL)										//如果有附加数据，复制在实体数据后面
	{
		memcpy(pListItem->pData + uAddSize, pAppendData, uAppendAddSize);
	}

	m_DataList.push_back(pListItem);								//加到队列尾部

	BOOL ret = PostQueuedCompletionStatus(m_hCompletionPort, pListItem->stDataHead.uSize, NULL, NULL);	//通知完成端口
	if (ret == FALSE)
	{
		//ERROR_LOG("CDataLine::AddData PostQueuedCompletionStatus failed err=%d", GetLastError());
	}

	return pListItem->stDataHead.uSize;		//返回大小
}

//提取消息数据
/*
Function		:GetData
Memo			:从队列中取出数据
Author			:Fred Huang
Add Data		:2008-3-4
Modify Data		:none
Parameter		:
[OUT]		pDataBuffer	:取出数据的缓存
[IN]		uBufferSize	:缓存大小，缺省为 LD_MAX_PART = 3096
Return			:取出数据的实际大小
*/
UINT CDataLine::GetData(DataLineHead * pDataBuffer, UINT uBufferSize)
{
	CSignedLockObject LockObject(&m_csLock, true);

	memset(pDataBuffer, 0, uBufferSize);
	//如果队列是空的，直接返回
	if (m_DataList.size() <= 0)
	{
		return 0;
	}

	//取数据
	ListItemData *pListItem = m_DataList.front();
	m_DataList.pop_front();

	UINT uDataSize = pListItem->stDataHead.uSize;

	if (uDataSize <= LD_MAX_PART)
	{
		//投递数据
		memcpy((void*)pDataBuffer, pListItem->pData, uDataSize);
	}
	else
	{
		ERROR_LOG("### DataLine GetData fail uDataSize=%d max=%d ###", uDataSize, LD_MAX_PART);
		uDataSize = 0;
	}

	//删除队列中的数据
	delete[] pListItem->pData;
	delete pListItem;

	return uDataSize;
}

//清理所有数据
bool CDataLine::CleanLineData()
{
	CSignedLockObject LockObject(&m_csLock, true);

	ListItemData *pListItem = NULL;

	while (m_DataList.size() > 0)
	{
		pListItem = m_DataList.front();
		m_DataList.pop_front();
		delete pListItem->pData;
		delete pListItem;
	}

	return true;
}

// 获取队列数据数量
INT_PTR CDataLine::GetDataCount(void)
{
	CSignedLockObject LockObject(&m_csLock, true);
	return m_DataList.size();
}
