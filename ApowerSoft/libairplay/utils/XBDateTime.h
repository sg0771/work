#pragma once
#include "../utils/utils.h"

#include <string>
#include <Windows.h>


class CDateTime;

class CDateTimeSpan
{
public:
  CDateTimeSpan();
  CDateTimeSpan(const CDateTimeSpan& span);
  CDateTimeSpan(int day, int hour, int minute, int second);

  bool operator >(const CDateTimeSpan& right) const;
  bool operator >=(const CDateTimeSpan& right) const;
  bool operator <(const CDateTimeSpan& right) const;
  bool operator <=(const CDateTimeSpan& right) const;
  bool operator ==(const CDateTimeSpan& right) const;
  bool operator !=(const CDateTimeSpan& right) const;

  CDateTimeSpan operator +(const CDateTimeSpan& right) const;
  CDateTimeSpan operator -(const CDateTimeSpan& right) const;

  const CDateTimeSpan& operator +=(const CDateTimeSpan& right);
  const CDateTimeSpan& operator -=(const CDateTimeSpan& right);

  void SetDateTimeSpan(int day, int hour, int minute, int second);
  void SetFromPeriod(const std::string &period);
  void SetFromTimeString(const std::string& time);

  int GetDays() const;
  int GetHours() const;
  int GetMinutes() const;
  int GetSeconds() const;
  int GetSecondsTotal() const;

private:
  void ToULargeInt(ULARGE_INTEGER& time) const;
  void FromULargeInt(const ULARGE_INTEGER& time);

private:
  FILETIME m_timeSpan;

  friend class CDateTime;
};

class CArchive;

class IArchivable
{
public:
    virtual void Archive(CArchive& ar) = 0;
    virtual ~IArchivable() {}
};

/// \brief DateTime class, which uses FILETIME as it's base.
class CDateTime : public IArchivable
{
public:
  CDateTime();
  CDateTime(const CDateTime& time);
  CDateTime(const SYSTEMTIME& time);
  CDateTime(const FILETIME& time);
  CDateTime(const time_t& time);
  CDateTime(const tm& time);
  CDateTime(int year, int month, int day, int hour, int minute, int second);
  virtual ~CDateTime() {}

  bool SetFromDateString(const std::string &date);

  static CDateTime GetCurrentDateTime();
  static CDateTime GetUTCDateTime();
  static int MonthStringToMonthNum(const std::string& month);

  const CDateTime& operator =(const SYSTEMTIME& right);
  const CDateTime& operator =(const FILETIME& right);
  const CDateTime& operator =(const time_t& right);
  const CDateTime& operator =(const tm& right);

  bool operator >(const CDateTime& right) const;
  bool operator >=(const CDateTime& right) const;
  bool operator <(const CDateTime& right) const;
  bool operator <=(const CDateTime& right) const;
  bool operator ==(const CDateTime& right) const;
  bool operator !=(const CDateTime& right) const;

  bool operator >(const FILETIME& right) const;
  bool operator >=(const FILETIME& right) const;
  bool operator <(const FILETIME& right) const;
  bool operator <=(const FILETIME& right) const;
  bool operator ==(const FILETIME& right) const;
  bool operator !=(const FILETIME& right) const;

  bool operator >(const SYSTEMTIME& right) const;
  bool operator >=(const SYSTEMTIME& right) const;
  bool operator <(const SYSTEMTIME& right) const;
  bool operator <=(const SYSTEMTIME& right) const;
  bool operator ==(const SYSTEMTIME& right) const;
  bool operator !=(const SYSTEMTIME& right) const;

  bool operator >(const time_t& right) const;
  bool operator >=(const time_t& right) const;
  bool operator <(const time_t& right) const;
  bool operator <=(const time_t& right) const;
  bool operator ==(const time_t& right) const;
  bool operator !=(const time_t& right) const;

  bool operator >(const tm& right) const;
  bool operator >=(const tm& right) const;
  bool operator <(const tm& right) const;
  bool operator <=(const tm& right) const;
  bool operator ==(const tm& right) const;
  bool operator !=(const tm& right) const;

  CDateTime operator +(const CDateTimeSpan& right) const;
  CDateTime operator -(const CDateTimeSpan& right) const;

  const CDateTime& operator +=(const CDateTimeSpan& right);
  const CDateTime& operator -=(const CDateTimeSpan& right);

  CDateTimeSpan operator -(const CDateTime& right) const;

  operator FILETIME() const;

  virtual void Archive(CArchive& ar);

  void Reset();

  int GetDay() const;
  int GetMonth() const;
  int GetYear() const;
  int GetHour() const;
  int GetMinute() const;
  int GetSecond() const;
  int GetDayOfWeek() const;
  int GetMinuteOfDay() const;

  bool SetDateTime(int year, int month, int day, int hour, int minute, int second);
  bool SetDate(int year, int month, int day);
  bool SetTime(int hour, int minute, int second);
  bool SetFromDBDate(const std::string &date);
  bool SetFromDBTime(const std::string &time);
  bool SetFromW3CDate(const std::string &date);
  bool SetFromW3CDateTime(const std::string &date, bool ignoreTimezone = false);
  bool SetFromUTCDateTime(const CDateTime &dateTime);
  bool SetFromUTCDateTime(const time_t &dateTime);
  bool SetFromRFC1123DateTime(const std::string &dateTime);

  /*! \brief set from a database datetime format YYYY-MM-DD HH:MM:SS
   \sa GetAsDBDateTime()
   */
  bool SetFromDBDateTime(const std::string &dateTime);

  void GetAsSystemTime(SYSTEMTIME& time) const;
  void GetAsTime(time_t& time) const;
  void GetAsTm(tm& time) const;
  void GetAsTimeStamp(FILETIME& time) const;

  CDateTime GetAsUTCDateTime() const;
  std::string GetAsSaveString() const;
  std::string GetAsDBDateTime() const;
  std::string GetAsDBDate() const;
  std::string GetAsRFC1123DateTime() const;
  std::string GetAsW3CDate() const;
  std::string GetAsW3CDateTime(bool asUtc = false) const;

  void SetValid(bool yesNo);
  bool IsValid() const;

  static void ResetTimezoneBias(void);
  static CDateTimeSpan GetTimezoneBias(void);

private:
  bool ToFileTime(const SYSTEMTIME& time, FILETIME& fileTime) const;
  bool ToFileTime(const time_t& time, FILETIME& fileTime) const;
  bool ToFileTime(const tm& time, FILETIME& fileTime) const;

  void ToULargeInt(ULARGE_INTEGER& time) const;
  void FromULargeInt(const ULARGE_INTEGER& time);

private:
  FILETIME m_time;

  typedef enum _STATE
  {
    invalid=0,
    valid
  } STATE;

  STATE m_state;
};
