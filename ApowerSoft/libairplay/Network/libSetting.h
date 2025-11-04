#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
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

#include <map>
#include <set>
#include <string>
#include <vector>

#include <memory>


#include <string>
#include <vector>



#include <set>
#include <string>



#include <map>
#include <set>
#include <string>



#include <string>
#include <vector>

#define SETTING_XML_ROOT              "settings"

#define SETTING_XML_ELM_SECTION       "section"
#define SETTING_XML_ELM_CATEGORY      "category"
#define SETTING_XML_ELM_GROUP         "group"
#define SETTING_XML_ELM_SETTING       "setting"
#define SETTING_XML_ELM_VISIBLE       "visible"
#define SETTING_XML_ELM_REQUIREMENT   "requirement"
#define SETTING_XML_ELM_CONDITION     "condition"
#define SETTING_XML_ELM_LEVEL         "level"
#define SETTING_XML_ELM_DEFAULT       "default"
#define SETTING_XML_ELM_VALUE         "value"
#define SETTING_XML_ELM_CONTROL       "control"
#define SETTING_XML_ELM_CONSTRAINTS   "constraints"
#define SETTING_XML_ELM_OPTIONS       "options"
#define SETTING_XML_ELM_OPTION        "option"
#define SETTING_XML_ELM_MINIMUM       "minimum"
#define SETTING_XML_ELM_STEP          "step"
#define SETTING_XML_ELM_MAXIMUM       "maximum"
#define SETTING_XML_ELM_ALLOWEMPTY    "allowempty"
#define SETTING_XML_ELM_DEPENDENCIES  "dependencies"
#define SETTING_XML_ELM_DEPENDENCY    "dependency"
#define SETTING_XML_ELM_UPDATES       "updates"
#define SETTING_XML_ELM_UPDATE        "update"
#define SETTING_XML_ELM_ACCESS        "access"
#define SETTING_XML_ELM_DELIMITER     "delimiter"
#define SETTING_XML_ELM_MINIMUM_ITEMS "minimumitems"
#define SETTING_XML_ELM_MAXIMUM_ITEMS "maximumitems"

#define SETTING_XML_ATTR_ID           "id"
#define SETTING_XML_ATTR_LABEL        "label"
#define SETTING_XML_ATTR_HELP         "help"
#define SETTING_XML_ATTR_TYPE         "type"
#define SETTING_XML_ATTR_PARENT       "parent"
#define SETTING_XML_ATTR_FORMAT       "format"
#define SETTING_XML_ATTR_DELAYED      "delayed"
#define SETTING_XML_ATTR_ON           "on"
#define SETTING_XML_ATTR_OPERATOR     "operator"
#define SETTING_XML_ATTR_NAME         "name"
#define SETTING_XML_ATTR_SETTING      "setting"
#define SETTING_XML_ATTR_BEFORE       "before"
#define SETTING_XML_ATTR_AFTER        "after"

typedef std::pair<int, int> StaticIntegerSettingOption;
typedef std::vector<StaticIntegerSettingOption> StaticIntegerSettingOptions;
typedef std::pair<std::string, int> DynamicIntegerSettingOption;
typedef std::vector<DynamicIntegerSettingOption> DynamicIntegerSettingOptions;
typedef std::pair<std::string, std::string> DynamicStringSettingOption;
typedef std::vector<DynamicStringSettingOption> DynamicStringSettingOptions;

class CSetting;
typedef void (*IntegerSettingOptionsFiller)(const CSetting* setting, std::vector< std::pair<std::string, int> >& list, int& current, void* data);
typedef void (*StringSettingOptionsFiller)(const CSetting* setting, std::vector< std::pair<std::string, std::string> >& list, std::string& current, void* data);



#include <string>
#include <vector>

#include <memory>

class IXmlDeserializable
{
public:
    virtual ~IXmlDeserializable() { }
};

typedef enum {
    BooleanLogicOperationOr = 0,
    BooleanLogicOperationAnd
} BooleanLogicOperation;

class CBooleanLogicValue : public IXmlDeserializable
{
public:
    CBooleanLogicValue(const std::string& value = "", bool negated = false)
        : m_value(value), m_negated(negated)
    { }
    virtual ~CBooleanLogicValue() { }

    virtual const std::string& GetValue() const { return m_value; }
    virtual bool IsNegated() const { return m_negated; }
    virtual const char* GetTag() const { return "value"; }

    virtual void SetValue(const std::string& value) { m_value = value; }
    virtual void SetNegated(bool negated) { m_negated = negated; }

protected:
    std::string m_value;
    bool m_negated;
};

typedef std::shared_ptr<CBooleanLogicValue> CBooleanLogicValuePtr;
typedef std::vector<CBooleanLogicValuePtr> CBooleanLogicValues;

class CBooleanLogicOperation;
typedef std::shared_ptr<CBooleanLogicOperation> CBooleanLogicOperationPtr;
typedef std::vector<CBooleanLogicOperationPtr> CBooleanLogicOperations;

class CBooleanLogicOperation : public IXmlDeserializable
{
public:
    CBooleanLogicOperation(BooleanLogicOperation op = BooleanLogicOperationAnd)
        : m_operation(op)
    { }
    virtual ~CBooleanLogicOperation();


    virtual BooleanLogicOperation GetOperation() const { return m_operation; }
    virtual const CBooleanLogicOperations& GetOperations() const { return m_operations; }
    virtual const CBooleanLogicValues& GetValues() const { return m_values; }

    virtual void SetOperation(BooleanLogicOperation op) { m_operation = op; }

protected:
    virtual CBooleanLogicOperation* newOperation() { return new CBooleanLogicOperation(); }
    virtual CBooleanLogicValue* newValue() { return new CBooleanLogicValue(); }

    BooleanLogicOperation m_operation;
    CBooleanLogicOperations m_operations;
    CBooleanLogicValues m_values;
};

class CBooleanLogic : public IXmlDeserializable
{
public:
    CBooleanLogic() { }
    virtual ~CBooleanLogic() { }
    virtual const CBooleanLogicOperationPtr& Get() const { return m_operation; }
    virtual CBooleanLogicOperationPtr Get() { return m_operation; }

protected:
    CBooleanLogicOperationPtr m_operation;
};


class CSettingsManager;
class CSetting;

typedef bool (*SettingConditionCheck)(const std::string& condition, const std::string& value, const CSetting* setting);

class ISettingCondition
{
public:
    ISettingCondition(CSettingsManager* settingsManager)
        : m_settingsManager(settingsManager)
    { }
    virtual ~ISettingCondition() { }

    virtual bool Check() const = 0;

protected:
    CSettingsManager* m_settingsManager;
};

class CSettingConditionItem : public CBooleanLogicValue, public ISettingCondition
{
public:
    CSettingConditionItem(CSettingsManager* settingsManager = NULL)
        : ISettingCondition(settingsManager)
    { }
    virtual ~CSettingConditionItem() { }

    virtual const char* GetTag() const { return SETTING_XML_ELM_CONDITION; }
    virtual bool Check() const;

protected:
    std::string m_name;
    std::string m_setting;
};

class CSettingConditionCombination : public CBooleanLogicOperation, public ISettingCondition
{
public:
    CSettingConditionCombination(CSettingsManager* settingsManager = NULL)
        : ISettingCondition(settingsManager)
    { }
    virtual ~CSettingConditionCombination() { }

    virtual bool Check() const;

private:
    virtual CBooleanLogicOperation* newOperation() { return new CSettingConditionCombination(m_settingsManager); }
    virtual CBooleanLogicValue* newValue() { return new CSettingConditionItem(m_settingsManager); }
};

class CSettingCondition : public CBooleanLogic, public ISettingCondition
{
public:
    CSettingCondition(CSettingsManager* settingsManager = NULL);
    virtual ~CSettingCondition() { }

    virtual bool Check() const;
};

class CSettingConditionsManager
{
public:
    CSettingConditionsManager();
    virtual ~CSettingConditionsManager();

    void AddCondition(const std::string& condition);
    void AddCondition(const std::string& identifier, SettingConditionCheck condition);

    bool Check(const std::string& condition, const std::string& value = "", const CSetting* setting = NULL) const;

private:
    CSettingConditionsManager(const CSettingConditionsManager&);
    CSettingConditionsManager const& operator=(CSettingConditionsManager const&);

    typedef std::pair<std::string, SettingConditionCheck> SettingConditionPair;
    typedef std::map<std::string, SettingConditionCheck> SettingConditionMap;

    SettingConditionMap m_conditions;
    std::set<std::string> m_defines;
};


class CSettingRequirementCondition : public CSettingConditionItem
{
public:
    CSettingRequirementCondition(CSettingsManager* settingsManager = NULL)
        : CSettingConditionItem(settingsManager)
    { }
    virtual ~CSettingRequirementCondition() { }

    virtual bool Check() const;
};

class CSettingRequirementConditionCombination : public CSettingConditionCombination
{
public:
    CSettingRequirementConditionCombination(CSettingsManager* settingsManager = NULL)
        : CSettingConditionCombination(settingsManager)
    { }
    virtual ~CSettingRequirementConditionCombination() { }

    virtual bool Check() const;

private:
    virtual CBooleanLogicOperation* newOperation() { return new CSettingRequirementConditionCombination(m_settingsManager); }
    virtual CBooleanLogicValue* newValue() { return new CSettingRequirementCondition(m_settingsManager); }
};

class CSettingRequirement : public CSettingCondition
{
public:
    CSettingRequirement(CSettingsManager* settingsManager = NULL);
    virtual ~CSettingRequirement() { }
};


class CSettingsManager;

class ISetting
{
public:
    /*!
     \brief Creates a new setting object with the given identifier.

     \param id Identifier of the setting object
     \param settingsManager Reference to the settings manager
     */
    ISetting(const std::string& id, CSettingsManager* settingsManager = NULL);
    virtual ~ISetting() { }

    const std::string& GetId() const { return m_id; }
    /*!
     \brief Whether the setting object is visible or hidden.

     \return True if the setting object is visible, false otherwise
     */
    virtual bool IsVisible() const { return m_visible; }
    /*!
     \brief Sets the visibility state of the setting object.

     \param visible Whether the setting object shall be visible or not
     */
    virtual void SetVisible(bool visible) { m_visible = visible; }
    /*!
    \brief Gets the localizeable label ID of the setting group.

    \return Localizeable label ID of the setting group
    */
    const int GetLabel() const { return m_label; }
    /*!
     \brief Sets the localizeable label ID of the setting group.

     \param label Localizeable label ID of the setting group
     */
    void SetLabel(int label) { m_label = label; }
    /*!
     \brief Gets the localizeable help ID of the setting group.

     \return Localizeable help ID of the setting group
     */
    const int GetHelp() const { return m_help; }
    /*!
     \brief Sets the localizeable help ID of the setting group.

     \param label Localizeable help ID of the setting group
     */
    void SetHelp(int help) { m_help = help; }
    /*!
     \brief Whether the setting object meets all necessary requirements.

     \return True if the setting object meets all necessary requirements, false otherwise
     */
    virtual bool MeetsRequirements() const { return m_meetsRequirements; }
    /*!
     \brief Checks if the setting object meets all necessary requirements.
     */
    virtual void CheckRequirements();
    /*!
     \brief Sets whether the setting object meets all necessary requirements.

     \param visible Whether the setting object meets all necessary requirements or not
     */
    virtual void SetRequirementsMet(bool requirementsMet) { m_meetsRequirements = requirementsMet; }

protected:
    std::string m_id;
    CSettingsManager* m_settingsManager;

private:
    bool m_visible;
    int m_label;
    int m_help;
    bool m_meetsRequirements;
    CSettingRequirement m_requirementCondition;
};


class CSetting;

class ISettingCallback
{
public:
    virtual ~ISettingCallback() { }
    virtual bool OnSettingChanging(const CSetting* setting) { return true; }
    virtual void OnSettingChanged(const CSetting* setting) { }
    virtual void OnSettingAction(const CSetting* setting) { }
    virtual bool OnSettingUpdate(CSetting*& setting, const char* oldSettingId) { return false; }
    virtual void OnSettingPropertyChanged(const CSetting* setting, const char* propertyName) { }
};


#include <string>



class ISettingControl
{
public:
    ISettingControl()
        : m_delayed(false)
    { }
    virtual ~ISettingControl() { }

    virtual std::string GetType() const = 0;
    const std::string& GetFormat() const { return m_format; }
    bool GetDelayed() const { return m_delayed; }
    void SetDelayed(bool delayed) { m_delayed = delayed; }

    virtual bool SetFormat(const std::string& format) { return true; }

protected:
    bool m_delayed;
    std::string m_format;
};



#include <list>
#include <set>
#include <string>

#include <set>
#include <string>

class CSettingConditions
{
public:
    static void Initialize();

    static const std::set<std::string>& GetSimpleConditions() { return m_simpleConditions; }
    static const std::map<std::string, SettingConditionCheck>& GetComplexConditions() { return m_complexConditions; }

    static bool Check(const std::string& condition, const std::string& value = "", const CSetting* setting = NULL);

private:
    static std::set<std::string> m_simpleConditions;
    static std::map<std::string, SettingConditionCheck> m_complexConditions;
};


typedef enum {
    SettingDependencyTypeNone = 0,
    SettingDependencyTypeEnable,
    SettingDependencyTypeUpdate,
    SettingDependencyTypeVisible
} SettingDependencyType;

typedef enum {
    SettingDependencyOperatorNone = 0,
    SettingDependencyOperatorEquals,
    SettingDependencyOperatorContains
} SettingDependencyOperator;

typedef enum {
    SettingDependencyTargetNone = 0,
    SettingDependencyTargetSetting,
    SettingDependencyTargetProperty
} SettingDependencyTarget;

class CSettingDependencyCondition : public CSettingConditionItem
{
public:
    explicit CSettingDependencyCondition(CSettingsManager* settingsManager = NULL);
    CSettingDependencyCondition(const std::string& setting, const std::string& value,
        SettingDependencyOperator op, bool negated = false,
        CSettingsManager* settingsManager = NULL);
    CSettingDependencyCondition(const std::string& strProperty, const std::string& value,
        const std::string& setting = "", bool negated = false,
        CSettingsManager* settingsManager = NULL);
    virtual ~CSettingDependencyCondition() { }

    virtual bool Check() const;

    const std::string& GetName() const { return m_name; }
    const std::string& GetSetting() const { return m_setting; }
    const SettingDependencyTarget GetTarget() const { return m_target; }
    const SettingDependencyOperator GetOperator() const { return m_operator; }

private:
    bool setTarget(const std::string& target);
    bool setOperator(const std::string& op);

    SettingDependencyTarget m_target;
    SettingDependencyOperator m_operator;
};

typedef std::shared_ptr<CSettingDependencyCondition> CSettingDependencyConditionPtr;

class CSettingDependencyConditionCombination;
typedef std::shared_ptr<CSettingDependencyConditionCombination> CSettingDependencyConditionCombinationPtr;

class CSettingDependencyConditionCombination : public CSettingConditionCombination
{
public:
    explicit CSettingDependencyConditionCombination(CSettingsManager* settingsManager = NULL)
        : CSettingConditionCombination(settingsManager)
    { }
    CSettingDependencyConditionCombination(BooleanLogicOperation op, CSettingsManager* settingsManager = NULL)
        : CSettingConditionCombination(settingsManager)
    {
        SetOperation(op);
    }
    virtual ~CSettingDependencyConditionCombination() { }


    const std::set<std::string>& GetSettings() const { return m_settings; }

    CSettingDependencyConditionCombination* Add(CSettingDependencyConditionPtr condition);
    CSettingDependencyConditionCombination* Add(CSettingDependencyConditionCombinationPtr operation);

private:
    virtual CBooleanLogicOperation* newOperation() { return new CSettingDependencyConditionCombination(m_settingsManager); }
    virtual CBooleanLogicValue* newValue() { return new CSettingDependencyCondition(m_settingsManager); }

    std::set<std::string> m_settings;
};

class CSettingDependency : public CSettingCondition
{
public:
    explicit CSettingDependency(CSettingsManager* settingsManager = NULL);
    CSettingDependency(SettingDependencyType type, CSettingsManager* settingsManager = NULL);
    virtual ~CSettingDependency() { }


    SettingDependencyType GetType() const { return m_type; }
    std::set<std::string> GetSettings() const;

    CSettingDependencyConditionCombinationPtr And();
    CSettingDependencyConditionCombinationPtr Or();

private:
    bool setType(const std::string& type);

    SettingDependencyType m_type;
};

typedef std::list<CSettingDependency> SettingDependencies;
typedef std::map<std::string, SettingDependencies> SettingDependencyMap;



typedef enum {
    SettingUpdateTypeNone = 0,
    SettingUpdateTypeRename,
    SettingUpdateTypeChange
} SettingUpdateType;

class CSettingUpdate
{
public:
    CSettingUpdate();
    virtual ~CSettingUpdate() { }

    bool operator<(const CSettingUpdate& rhs) const;



    SettingUpdateType GetType() const { return m_type; }
    const std::string& GetValue() const { return m_value; }

private:
    bool setType(const std::string& type);

    SettingUpdateType m_type;
    std::string m_value;
};


#include "../utils/thread.h"

/*!
 \ingroup settings
 \brief Basic setting types available in the settings system.
 */
typedef enum {
  SettingTypeNone = 0,
  SettingTypeBool,
  SettingTypeInteger,
  SettingTypeNumber,
  SettingTypeString,
  SettingTypeAction,
  SettingTypeList
} SettingType;

/*!
 \ingroup settings
 \brief Levels which every setting is assigned to.
 */
typedef enum {
  SettingLevelBasic  = 0,
  SettingLevelStandard,
  SettingLevelAdvanced,
  SettingLevelExpert,
  SettingLevelInternal
} SettingLevel;

typedef enum {
  SettingOptionsTypeNone = 0,
  SettingOptionsTypeStatic,
  SettingOptionsTypeDynamic
} SettingOptionsType;

/*!
 \ingroup settings
 \brief Setting base class containing all the properties which are common to
 all settings independent of the setting type.
 */
class CSetting : public ISetting,
                 protected ISettingCallback
{
public:
  CSetting(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSetting(const std::string &id, const CSetting &setting);
  virtual ~CSetting();

  virtual CSetting* Clone(const std::string &id) const = 0;

  virtual int GetType() const = 0;
  virtual bool FromString(const std::string &value) = 0;
  virtual std::string ToString() const = 0;
  virtual bool Equals(const std::string &value) const = 0;
  virtual bool CheckValidity(const std::string &value) const = 0;
  virtual void Reset() = 0;

  int GetLabel() const { return m_label; }
  void SetLabel(int label) { m_label = label; }
  int GetHelp() const { return m_help; }
  void SetHelp(int help) { m_help = help; }
  bool IsEnabled() const;
  void SetEnabled(bool enabled);
  bool IsDefault() const { return !m_changed; }
  const std::string& GetParent() const { return m_parentSetting; }
  void SetParent(const std::string& parentSetting) { m_parentSetting = parentSetting; }
  SettingLevel GetLevel() const { return m_level; }
  void SetLevel(SettingLevel level) { m_level = level; }
  const ISettingControl* GetControl() const { return m_control; }
  ISettingControl* GetControl() { return m_control; }
  void SetControl(ISettingControl* control) { m_control = control; }
  const SettingDependencies& GetDependencies() const { return m_dependencies; }
  void SetDependencies(const SettingDependencies &dependencies) { m_dependencies = dependencies; }
  const std::set<CSettingUpdate>& GetUpdates() const { return m_updates; }

  void SetCallback(ISettingCallback *callback) { 
      m_callback = callback; 
  }

  // overrides of ISetting
  virtual bool IsVisible() const;

protected:
  // implementation of ISettingCallback
  virtual bool OnSettingChanging(const CSetting *setting);
  virtual void OnSettingChanged(const CSetting *setting);
  virtual void OnSettingAction(const CSetting *setting);
  virtual bool OnSettingUpdate(CSetting* &setting, const char *oldSettingId);
  virtual void OnSettingPropertyChanged(const CSetting *setting, const char *propertyName);

  void Copy(const CSetting &setting);

  ISettingCallback *m_callback;
  int m_label;
  int m_help;
  bool m_enabled;
  std::string m_parentSetting;
  SettingLevel m_level;
  ISettingControl *m_control;
  SettingDependencies m_dependencies;
  std::set<CSettingUpdate> m_updates;
  bool m_changed;
  CSharedSection m_critical;
};

typedef std::shared_ptr<CSetting> SettingPtr;

typedef std::vector<CSetting *> SettingList;
typedef std::vector<SettingPtr> SettingPtrList;

/*!
 \ingroup settings
 \brief List setting implementation
 \sa CSetting
 */
class CSettingList : public CSetting
{
public:
  CSettingList(const std::string &id, CSetting *settingDefinition, CSettingsManager *settingsManager = NULL);
  CSettingList(const std::string &id, CSetting *settingDefinition, int label, CSettingsManager *settingsManager = NULL);
  CSettingList(const std::string &id, const CSettingList &setting);
  virtual ~CSettingList();

  virtual CSetting* Clone(const std::string &id) const;

 
  virtual int GetType() const { return SettingTypeList; }
  virtual bool FromString(const std::string &value);
  virtual std::string ToString() const;
  virtual bool Equals(const std::string &value) const;
  virtual bool CheckValidity(const std::string &value) const;
  virtual void Reset();
  
  int GetElementType() const;
  const CSetting* GetDefinition() const { return m_definition; }
  void SetDefinition(CSetting *definition) { m_definition = definition; }

  const std::string& GetDelimiter() const { return m_delimiter; }
  void SetDelimiter(const std::string &delimiter) { m_delimiter = delimiter; }
  int GetMinimumItems() const { return m_minimumItems; }
  void SetMinimumItems(int minimumItems) { m_minimumItems = minimumItems; }
  int GetMaximumItems() const { return m_maximumItems; }
  void SetMaximumItems(int maximumItems) { m_maximumItems = maximumItems; }
  
  bool FromString(const std::vector<std::string> &value);

  const SettingPtrList& GetValue() const { return m_values; }
  bool SetValue(const SettingPtrList &values);
  const SettingPtrList& GetDefault() const { return m_defaults; }
  void SetDefault(const SettingPtrList &values);

protected:
  void copy(const CSettingList &setting);
  static void copy(const SettingPtrList &srcValues, SettingPtrList &dstValues);
  bool fromString(const std::string &strValue, SettingPtrList &values) const;
  bool fromValues(const std::vector<std::string> &strValues, SettingPtrList &values) const;
  std::string toString(const SettingPtrList &values) const;

  SettingPtrList m_values;
  SettingPtrList m_defaults;
  CSetting *m_definition;
  std::string m_delimiter;
  int m_minimumItems;
  int m_maximumItems;
};

/*!
 \ingroup settings
 \brief Boolean setting implementation.
 \sa CSetting
 */
class CSettingBool : public CSetting
{
public:
  CSettingBool(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingBool(const std::string &id, const CSettingBool &setting);
  CSettingBool(const std::string &id, int label, bool value, CSettingsManager *settingsManager = NULL);
  virtual ~CSettingBool() { }

  virtual CSetting* Clone(const std::string &id) const;

  virtual int GetType() const { return SettingTypeBool; }
  virtual bool FromString(const std::string &value);
  virtual std::string ToString() const;
  virtual bool Equals(const std::string &value) const;
  virtual bool CheckValidity(const std::string &value) const;
  virtual void Reset() { SetValue(m_default); }

  bool GetValue() const { CSharedLock lock(m_critical); return m_value; }
  bool SetValue(bool value);
  bool GetDefault() const { return m_default; }
  void SetDefault(bool value);

private:
  void copy(const CSettingBool &setting);
  bool fromString(const std::string &strValue, bool &value) const;

  bool m_value;
  bool m_default;
};

/*!
 \ingroup settings
 \brief Integer setting implementation
 \sa CSetting
 */
class CSettingInt : public CSetting
{
public:
  CSettingInt(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingInt(const std::string &id, const CSettingInt &setting);
  CSettingInt(const std::string &id, int label, int value, CSettingsManager *settingsManager = NULL);
  CSettingInt(const std::string &id, int label, int value, int minimum, int step, int maximum, CSettingsManager *settingsManager = NULL);
  CSettingInt(const std::string &id, int label, int value, const StaticIntegerSettingOptions &options, CSettingsManager *settingsManager = NULL);
  virtual ~CSettingInt() { }

  virtual CSetting* Clone(const std::string &id) const;

  virtual int GetType() const { return SettingTypeInteger; }
  virtual bool FromString(const std::string &value);
  virtual std::string ToString() const;
  virtual bool Equals(const std::string &value) const;
  virtual bool CheckValidity(const std::string &value) const;
  virtual bool CheckValidity(int value) const;
  virtual void Reset() { SetValue(m_default); }

  int GetValue() const { CSharedLock lock(m_critical); return m_value; }
  bool SetValue(int value);
  int GetDefault() const { return m_default; }
  void SetDefault(int value);

  int GetMinimum() const { return m_min; }
  void SetMinimum(int minimum) { m_min = minimum; }
  int GetStep() const { return m_step; }
  void SetStep(int step) { m_step = step; }
  int GetMaximum() const { return m_max; }
  void SetMaximum(int maximum) { m_max = maximum; }

  SettingOptionsType GetOptionsType() const;
  const StaticIntegerSettingOptions& GetOptions() const { return m_options; }
  void SetOptions(const StaticIntegerSettingOptions &options) { m_options = options; }
  const std::string& GetOptionsFillerName() const { return m_optionsFillerName; }
  void SetOptionsFillerName(const std::string &optionsFillerName, void *data = NULL)
  {
    m_optionsFillerName = optionsFillerName;
    m_optionsFillerData = data;
  }
  void SetOptionsFiller(IntegerSettingOptionsFiller optionsFiller, void *data = NULL)
  {
    m_optionsFiller = optionsFiller;
    m_optionsFillerData = data;
  }
  DynamicIntegerSettingOptions UpdateDynamicOptions();

private:
  void copy(const CSettingInt &setting);
  static bool fromString(const std::string &strValue, int &value);

  int m_value;
  int m_default;
  int m_min;
  int m_step;
  int m_max;
  StaticIntegerSettingOptions m_options;
  std::string m_optionsFillerName;
  IntegerSettingOptionsFiller m_optionsFiller;
  void *m_optionsFillerData;
  DynamicIntegerSettingOptions m_dynamicOptions;
};

