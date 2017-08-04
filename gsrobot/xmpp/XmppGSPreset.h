#pragma once
#include "../typedef.h"
#include "gloox/stanzaextension.h"
#include "gloox/tag.h"
#include "../PresetManager.h"

using namespace gloox;

class XmppGSPreset :
	public StanzaExtension
{
public:
	enum defPSMethod
	{
		defPSMethod_Unknown	= 0,
		defPSMethod_goto,			// ���� 1��Ԥ�õ㣬ֻ��Ҫ�ṩԤ�õ�ID
		defPSMethod_add,			// ��� 1��Ԥ�õ�
		defPSMethod_del,			// ɾ�� n��Ԥ�õ㣬ֻ��Ҫ�ṩԤ�õ�ID
		defPSMethod_edit,			// �޸� n��Ԥ�õ���Ƶ���Ϣ
		defPSMethod_setnew,			// ���� 1��Ԥ�õ��λ�ã�ֻ��Ҫ�ṩԤ�õ�ID
		defPSMethod_sort,			// ���� n����ֻ��Ҫ�ṩԤ�õ�ID���Ⱥ�˳���������
	};

public:
	XmppGSPreset( const struTagParam &TagParam, const std::string &srcmethod, IOTDeviceType device_type, int device_id, const defPresetQueue &PresetList, defGSReturn result=defGSReturn_Null );
	XmppGSPreset( const Tag* tag );
	~XmppGSPreset(void);

	// reimplemented from StanzaExtension
	virtual const std::string& filterString() const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* newInstance( const Tag* tag ) const
	{
		return new XmppGSPreset( tag );
	}

	void PrintTag( const Tag* tag, const Stanza *stanza ) const;

	// reimplemented from StanzaExtension
	virtual StanzaExtension* clone() const
	{
		XmppGSPreset *pnew = new XmppGSPreset(NULL);
		*pnew = *this;

		pnew->m_PresetList.clear();
		CPresetManager::ClonePresetQueue_Spec( pnew->m_PresetList, this->m_PresetList );

		return pnew;
	}

	// reimplemented from StanzaExt
	virtual Tag* tag() const;

	defPSMethod GetMethod() const
	{
		return this->m_method;
	}

	const std::string& GetSrcMethod() const
	{
		return this->m_srcmethod;
	}

	IOTDeviceType get_device_type() const
	{
		return m_device_type;
	}

	int get_device_id() const
	{
		return m_device_id;
	}

	const defPresetQueue& get_PresetList() const
	{
		return m_PresetList;
	}

	void swap_PresetList( defPresetQueue &other )
	{
		m_PresetList.swap(other);
	}

	CPresetObj* GetFristPreset() const
	{
		if( !m_PresetList.empty() )
		{
			return ( *m_PresetList.begin() );
		}

		return NULL;
	}

	void SetResult( defGSReturn result )
	{
		m_result = result;
	}

	defGSReturn GetResult() const
	{
		return m_result;
	}

private:
	struTagParam m_TagParam;
	defGSReturn m_result;

	defPSMethod m_method;
	std::string m_srcmethod;
	IOTDeviceType m_device_type;
	int m_device_id;
	defPresetQueue m_PresetList;
};
