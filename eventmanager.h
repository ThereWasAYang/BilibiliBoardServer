#ifndef _DreamIsland_EventManager_hpp__
#define _DreamIsland_EventManager_hpp__



#include <map>

class CFunctionSlotBase
{
public:
	virtual ~CFunctionSlotBase(){}
	virtual bool operator()( void * pdata ){ return true;}
	virtual long getThisAddr() { return 0;}
	virtual long getFunAddr() { return 0;}
	virtual void emptyThisAddr(){}
};



template<typename T , typename T2>
class CFunctionSlot : public CFunctionSlotBase
{
	typedef void (T::*f)( T2* );
public:
	CFunctionSlot( void (T::*f)( T2* ) , T * obj )
		:m_funPtr( f ),m_this( obj )
	{

	}
	virtual ~CFunctionSlot(){}

	virtual bool operator() ( void* pdata )
	{
		if( m_this != NULL && m_funPtr != NULL ) 
		{
			(m_this->*m_funPtr)( reinterpret_cast<T2*>( pdata ) );
			return true;
		}
		else
			return false;
	}

	virtual long getThisAddr()
	{
		return ( *reinterpret_cast<long*>( &m_this ) );
	}
	virtual void emptyThisAddr()
	{
		m_this = NULL;
	}

	virtual long getFunAddr()
	{
		//提取类成员函数地址，&m_funPtr 取出成员函数指针地址，将其转为指向long型的指针，再用*取出地址值
		return ( *reinterpret_cast<long*>( &m_funPtr ) );
	}

private:
	f m_funPtr;
	T * m_this;
};




class CEventManager
{
public:
	~CEventManager()
	{
		for( MSGMAP::iterator it = m_msgMap.begin() ; it != m_msgMap.end() ; it++ )
		{
			std::vector<CFunctionSlotBase* > * p = it->second ;
			for( std::vector<CFunctionSlotBase* >::iterator it2 = p->begin() ; it2 != p->end() ; it2++ )
			{
				delete *it2;
			}
			p->clear();
			delete p;
		}
		m_msgMap.clear();
	}
	/*
	*注册服务器回发事件处理例程的函数
	*参数一：消息类型
	*参数二：处理例程的地址 例程函数定义为 void f( char* )
	*参数三：例程对象的地址
	*/
	template<typename T , typename T2>
	bool registerMessageHandle( uint32 msgType , void (T::*f)( T2* ) , T * obj )
	{
		std::vector<CFunctionSlotBase* > * vec = NULL;
		MSGMAP::iterator it = m_msgMap.find( msgType );
		if( it == m_msgMap.end() )
			vec = new  std::vector<CFunctionSlotBase* >;
		else
			vec = it->second;

		CFunctionSlotBase * pslot = new CFunctionSlot<T,T2>( f , obj );
		if( !pslot ) return false;

		vec->push_back( pslot );
		m_msgMap.insert( std::make_pair( msgType , vec ) );
		return true;
	}

	template<typename T>
	bool unregisterMessageHandle( uint32 msgType , T* obj )
	{
		std::vector<CFunctionSlotBase* > * vec = NULL;
		MSGMAP::iterator it = m_msgMap.find( msgType );
		if( it == m_msgMap.end() )
			return true ;
		else
			vec = it->second;

		long thisAddr = *reinterpret_cast<long *>( &obj );

		for( std::vector<CFunctionSlotBase* >::iterator itor =  vec->begin() ; itor != vec->end() ; ++itor)
		{
			if( (*itor)->getThisAddr() == thisAddr )
			{
				//反注册并不真实删除，只是将this指针置空
				(*itor)->emptyThisAddr();
			}

		}
		return true;
	}
	template<typename T>
	bool fireMessage( uint32 type , T * pdata)
	{

		//判断该类型事件是否在系统中注册
		MSGMAP::iterator typeIt = m_msgMap.find( type );
		if( typeIt == m_msgMap.end() ) return false;

		//发送数据到对应的注册函数中
		for( std::vector<CFunctionSlotBase* >::iterator itor =  typeIt->second->begin() ; itor != typeIt->second->end() ; itor++ )
		{
			//判断该成员函数指针对应的对象是否为0，防止对象意外被删除
			if( (*itor)->getThisAddr() != 0 )
				(**itor)( pdata );
		}
		return true;
	}


	


protected:
	//用于保存注册的函数地址
	std::map< uint32 , std::vector<CFunctionSlotBase* >* > m_msgMap;
	typedef std::map< uint32, std::vector<CFunctionSlotBase* >* > MSGMAP;
};













#endif // _DreamIsland_EventManager_hpp__
