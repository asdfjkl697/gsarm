#pragma once
#include "GSIOTObjBase.h"
#include "gloox/tag.h"

using namespace gloox;


struct struDBSavePreset
{
	int id;
	int device_type;
	int device_id;
	int sort_no;
	int preset_index;
	int enable;
	int param1;
	int param2;
	std::string name;
};

//
// 预置点对象
class CPresetObj : public GSIOTObjBase
{
public:
	CPresetObj( const std::string &name );
	CPresetObj( const struDBSavePreset &obj );
	CPresetObj( const Tag* tag );
	~CPresetObj(void)
	{
	}

	void Reset();

	Tag* tag(const struTagParam &TagParam);

	virtual int GetId() const
	{
		return m_id;
	}

	void SetId( int id )
	{
		m_id = id;
	}

	int GetIndex() const
	{
		return m_index;
	}

	void SetIndex( int index )
	{
		m_index = index;
	}

	int GetSortNo() const
	{
		return m_sort_no;
	}

	void SetSortNo( int sort_no )
	{
		m_sort_no = sort_no;
	}

	uint32_t GetEnable() const 
	{
		return m_enable;
	}

	void SetEnable( uint32_t enable )
	{
		m_enable = enable;
	}

	void SwapInfo( CPresetObj &PresetOther );

	void SetName( const std::string& name )
	{
		this->m_name = name;
	}

	virtual const std::string GetObjName() const
	{
		return m_name;
	}

	virtual GSIOTObjType GetObjType() const
	{
		return GSIOTObjType_PresetObj;
	}

	virtual CPresetObj* clone() const
	{
		return new CPresetObj(*this);
	}

	virtual GSIOTObjBase* cloneObj() const
	{
		return clone();
	}

private:
	int m_id;		// 对应数据库id，全局唯一
	int m_index;	// 预置点索引号，设备范围内唯一。允许添加 index 相同的预置位配置，比如别名
	int m_sort_no;	// 排序号
	uint32_t m_enable;
	std::string m_name;
};

typedef std::vector<struDBSavePreset> defDBSavePresetQueue;
typedef std::list<CPresetObj*> defPresetQueue;


//
// 预置点对象集合管理类
class CPresetManager
{
public:
	CPresetManager(void);
	CPresetManager( const CPresetManager& RightSides );
	CPresetManager& operator= ( const CPresetManager &RightSides );
	~CPresetManager(void);

public:
	static void ClonePresetQueue_Spec( defPresetQueue &quedest, const defPresetQueue &quesrc );
	static void DeletePresetQueue_Spec( defPresetQueue &que );
	static bool DeletePreset_Spec( defPresetQueue &que, uint32_t PresetID );
	static void PresetQueueChangeToOne_Spec( defPresetQueue &que, uint32_t PresetID=0 );
	static CPresetObj* GetPresetForID_Spec( const defPresetQueue &que, uint32_t PresetID );
	static CPresetObj* GetPresetForIndex_Spec( const defPresetQueue &que, uint32_t PresetIndex );
	static int GetUnusedIndex_Spec( const defPresetQueue &que );

public:
	void tagAllPreset( Tag* iDest, const struTagParam &TagParam ) const;

	int GetUnusedIndex() const;

	void AddPreset( CPresetObj *pPreset )
	{
		m_PresetQueue.push_back(pPreset);
	}

	CPresetObj* GetPreset( uint32_t PresetID ) const;
	//CPresetObj* GetPresetForIndex( uint32_t PresetIndex ) const; // index不一定唯一，不能获取

	const defPresetQueue& GetPresetList() const
	{
		return this->m_PresetQueue;
	}

	defGSReturn CheckExist( const int PresetID, const std::string &newname, std::string *strerr );
	void DeletePresetQueue();
	bool DeletePreset( uint32_t PresetID );
	void PresetQueueChangeToOne( uint32_t PresetID=0 );

	CPresetObj* GetFristPreset() const;

	CPresetObj* UpdatePreset( const CPresetObj *pPreset );

	void Sort( std::map<int,int> &sortlist ); //<PresetID,sort_no>
	void SortByPresetList( const defPresetQueue &PresetSortlist );

protected:
	defPresetQueue m_PresetQueue;
};
