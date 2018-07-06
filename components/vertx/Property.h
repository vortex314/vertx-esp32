#ifndef PROPERTY_H
#define PROPERTY_H
#include <LinkedList.hpp>
#include <vertx.h>
//#include <memory.h>
#include <ArduinoJson.h>

class Property : public LinkedList<Property>
{
public:
	uint64_t _timeout;
	uid_t _uid;
	uint32_t _interval;
	bool _trackChanges=true;
	Property(const char* name,int interval)
	{
		if ( interval < 0 ) {
			_trackChanges=false;
			_interval=- interval;
		} else {
			_trackChanges=true;
			_interval = interval;
		}
		_uid = UID.add(name);
		_timeout = Sys::millis()+_interval;
		add(this);
	}
	virtual void toJson(Str&)
	{
		ERROR("");
	};
	virtual void fromJson(Str&)
	{
		ERROR("");
	};
	static Property* findByUid(uid_t uid);
	void updated()
	{
		_timeout=Sys::millis()-1;
	}
	bool trackChanges()
	{
		return _trackChanges;
	}
	void trackChanges(bool t)
	{
		_trackChanges=t;
	}
	virtual bool hasChanged()
	{
		return false;
	}

	uid_t uid()
	{
		return _uid;
	};

};

template <typename T>
class PropertyReference : public Property
{
public:
	T& _var;
	T _oldVar;

	PropertyReference(const char* name,T& var,uint32_t interval) : Property(name,interval),_var(var),_oldVar(var)
	{
	}

	~PropertyReference()
	{
	}
	void toJson(Str& msg)
	{
		msg.append(_var);
		_oldVar=_var;
	}
	void fromJson(Str& msg)
	{
		msg.parse(_var);
	}
	bool hasChanged()
	{
		if ( !_trackChanges ) return false;
		if ( _var == _oldVar ) return false;
		return true;
	}

};

template <typename T>
class PropertyFunction : public Property
{
	T _oldVar;
public:
	std::function<T()> _fRead;
	PropertyFunction(const char* name,std::function<T ()> fRead,uint32_t interval) : Property(name,interval),_fRead(fRead)
	{
	}

	~PropertyFunction()
	{
	}
	void toJson(Str& msg)
	{
		msg.append(_fRead());
		_oldVar = _fRead();
	}
	bool hasChanged()
	{
		if ( !_trackChanges ) return false;
		if ( _fRead()==_oldVar) return false;
		return true;
	}
};

template <typename T>
class PropertyWriteFunction : public Property
{
	T _oldVar;
public:
	std::function<T()> _fRead;
	std::function<Erc(T&v)> _fWrite;


	PropertyWriteFunction(const char* name,std::function<T ()> fRead,std::function<Erc (T& input)> fWrite,uint32_t interval)
		: Property(name,interval),_fRead(fRead),_fWrite(fWrite)
	{
	}

	~PropertyWriteFunction()
	{
	}
	void toJson(Str& msg)
	{
		msg.append(_fRead());
		_oldVar = _fRead();
	}
	void fromJson(Str& msg)
	{
		T var;
		msg.parse(var);
		_fWrite(var);
	}
	bool hasChanged()
	{
		if ( !_trackChanges ) return false;
		if ( _fRead()==_oldVar) return false;
		return true;
	}

};



class PropertyVerticle : public VerticleCoRoutine
{
	Property* _currentProp;
	bool  _mqttConnected;
	Message _toMqttMsg;
	Str _topic;
	Str _message;
	void sendProp(Property* p);
public:
	PropertyVerticle(const char* name);
	void start();
	void run();
};

extern PropertyVerticle propertyVerticle;



#endif // PROPERTY_H
