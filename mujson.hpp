#pragma once

// MuJSON interface similar to libJsonCpp interface. For optimal usage, use the C99 interface.

#ifndef MUJSON_USE_CPP_INTERFACE
#error Please globally define MUJSON_USE_CPP_INTERFACE in order to use it. 
#endif

#include <iosfwd>
#include <string>
#include <vector>

extern "C"
{
	
typedef struct
{
	std::istream *file;
} muj_source;

extern bool muj_read_byte(muj_source source, char* byte);
extern bool muj_peek_byte(muj_source source, char* byte);
	
#include <mujson.h>

} // extern "C"

namespace Json
{
	
class Value;
	
class DocumentContainer
{
public:
	muj_document getDocument() const {return document;}
protected:
	muj_document document;
};

class Reader
	: public DocumentContainer
{
public:
	Reader();
	~Reader();
	/// Parses directly from an istream. Mind that this will malloc the size of the remainder of the stream (+1 byte)
	bool parse(std::istream& inStream, Value& root);
};

class Value
{
public:
	Value() : document(0), index(0) {}

	Value operator[](const char* name) const;
	Value operator[](size_t atIndex) const;
	
	bool empty() const;
	
	float asFloat() const;
	double asDouble() const;
	bool asBool() const;
	int asInt() const;
	long asLong() const;
	std::string asString() const;
	
	bool isArray() const;
	bool isObject() const;
	bool isString() const;
	bool isNumeric() const;
	bool isConstant() const;
	bool isBoolean() const;
	bool isTrue() const;
	bool isFalse() const;
	bool isNull() const;
	private:

	DocumentContainer* document;
	MUJ_INDEX index;
	
	friend class Object;
	friend class Array;
	friend class Reader;
	
	Value(DocumentContainer& _document, MUJ_INDEX _index) : document(&_document), index(_index) {}
};

struct KeyValuePair
{
	std::string key;
	Value value;
};

class Object // Optimization. Preloads all key/value pairs.
{
public:
	explicit Object(const Value& value);
	
	Value operator[](const char* name) const;
	KeyValuePair operator[](int index) const;
	
	bool empty() const;
	
	size_t size() const {return keyValuePairs.size();}
private:
	std::vector<muj_key_value_pair> keyValuePairs;
	std::vector<std::string> keys;
	MUJ_INDEX index;
	DocumentContainer* document;
};

class Array // Optimization. Preloads all array elements.
{
public:
	explicit Array(const Value& value);
	
	Value operator[](size_t index) const;
	
	bool empty() const;
	
	size_t size() const {return elements.size();}
private:
	std::vector<MUJ_INDEX> elements;
	MUJ_INDEX index;
	DocumentContainer* document;
};

}
