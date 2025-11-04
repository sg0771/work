/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "../utils/thread.h"
#include "CacheStrategy.h"
#include "IFile.h"
#include "../utils/Win32Utils.h"
#include "SpecialProtocol.h"
#include "../utils/URL.h"
#if defined(TARGET_POSIX)
#include "posix/PosixFile.h"
#elif defined(_WIN32)
#include "Win32File.h"
#endif // _WIN32

#include <cassert>
#include <algorithm>

using namespace XFILE;

CCacheStrategy::CCacheStrategy() : m_bEndOfInput(false)
{
}


CCacheStrategy::~CCacheStrategy()
{
}

void CCacheStrategy::EndOfInput() {
  m_bEndOfInput = true;
}

bool CCacheStrategy::IsEndOfInput()
{
  return m_bEndOfInput;
}

void CCacheStrategy::ClearEndOfInput()
{
  m_bEndOfInput = false;
}



CDoubleCache::CDoubleCache(CCacheStrategy *impl)
{
  assert(NULL != impl);
  m_pCache = impl;
  m_pCacheOld = NULL;
}

CDoubleCache::~CDoubleCache()
{
  delete m_pCache;
  delete m_pCacheOld;
}

int CDoubleCache::Open()
{
  return m_pCache->Open();
}

void CDoubleCache::Close()
{
  m_pCache->Close();
  if (m_pCacheOld)
  {
    delete m_pCacheOld;
    m_pCacheOld = NULL;
  }
}

size_t CDoubleCache::GetMaxWriteSize(const size_t& iRequestSize)
{
  return m_pCache->GetMaxWriteSize(iRequestSize); // NOTE: Check the active cache only
}

int CDoubleCache::WriteToCache(const char *pBuffer, size_t iSize)
{
  return m_pCache->WriteToCache(pBuffer, iSize);
}

int CDoubleCache::ReadFromCache(char *pBuffer, size_t iMaxSize)
{
  return m_pCache->ReadFromCache(pBuffer, iMaxSize);
}

int64_t CDoubleCache::WaitForData(unsigned int iMinAvail, unsigned int iMillis)
{
  return m_pCache->WaitForData(iMinAvail, iMillis);
}

int64_t CDoubleCache::Seek(int64_t iFilePosition)
{
  /* Check whether position is NOT in our current cache but IS in our old cache.
   * This is faster/more efficient than having to possibly wait for data in the
   * Seek() call below
   */
  if (!m_pCache->IsCachedPosition(iFilePosition) &&
       m_pCacheOld && m_pCacheOld->IsCachedPosition(iFilePosition))
  {
    return CACHE_RC_ERROR; // Request seek event, so caches are swapped
  }

  return m_pCache->Seek(iFilePosition); // Normal seek
}

bool CDoubleCache::Reset(int64_t iSourcePosition, bool clearAnyway)
{
  if (!clearAnyway && m_pCache->IsCachedPosition(iSourcePosition)
      && (!m_pCacheOld || !m_pCacheOld->IsCachedPosition(iSourcePosition)
          || m_pCache->CachedDataEndPos() >= m_pCacheOld->CachedDataEndPos()))
  {
    return m_pCache->Reset(iSourcePosition, clearAnyway);
  }
  if (!m_pCacheOld)
  {
    CCacheStrategy *pCacheNew = m_pCache->CreateNew();
    if (pCacheNew->Open() != CACHE_RC_OK)
    {
      delete pCacheNew;
      return m_pCache->Reset(iSourcePosition, clearAnyway);
    }
    bool bRes = pCacheNew->Reset(iSourcePosition, clearAnyway);
    m_pCacheOld = m_pCache;
    m_pCache = pCacheNew;
    return bRes;
  }
  bool bRes = m_pCacheOld->Reset(iSourcePosition, clearAnyway);
  CCacheStrategy *tmp = m_pCacheOld;
  m_pCacheOld = m_pCache;
  m_pCache = tmp;
  return bRes;
}

void CDoubleCache::EndOfInput()
{
  m_pCache->EndOfInput();
}

bool CDoubleCache::IsEndOfInput()
{
  return m_pCache->IsEndOfInput();
}

void CDoubleCache::ClearEndOfInput()
{
  m_pCache->ClearEndOfInput();
}

int64_t CDoubleCache::CachedDataEndPos()
{
  return m_pCache->CachedDataEndPos();
}

int64_t CDoubleCache::CachedDataEndPosIfSeekTo(int64_t iFilePosition)
{
  int64_t ret = m_pCache->CachedDataEndPosIfSeekTo(iFilePosition);
  if (m_pCacheOld)
    return std::max(ret, m_pCacheOld->CachedDataEndPosIfSeekTo(iFilePosition));
  return ret;
}

bool CDoubleCache::IsCachedPosition(int64_t iFilePosition)
{
  return m_pCache->IsCachedPosition(iFilePosition) || (m_pCacheOld && m_pCacheOld->IsCachedPosition(iFilePosition));
}

CCacheStrategy *CDoubleCache::CreateNew()
{
  return new CDoubleCache(m_pCache->CreateNew());
}

